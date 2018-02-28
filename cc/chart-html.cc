#include <iostream>
#include <fstream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

static void contents(std::ostream& output, const acmacs::chart::Chart& chart, bool all_fields);
static void header(std::ostream& output, std::string table_name);
static void footer(std::ostream& output);
static std::string html_escape(std::string source);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    using namespace std::string_literals;

    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--all-fields", false},
                {"--open", false},
                {"--verbose", false},
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> [<output.html>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::file::temp temp_file(".html");
            const std::string output_filename = args.number_of_arguments() > 1 ? std::string{args[1]} : static_cast<std::string>(temp_file);
            std::ofstream output{output_filename};

            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            header(output, chart->make_name());
            contents(output, *chart, args["--all-fields"]);
            footer(output);
            output.close();

            if (args["--open"] || args.number_of_arguments() < 2)
                acmacs::open(output_filename);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

struct AntigenFields
{
    bool annotations;
    bool reassortants;
    bool passages;
    bool dates;
    bool lab_ids;

    size_t skip_left() const { return size_t(annotations) + size_t(reassortants) + size_t(passages); }
    size_t skip_right() const { return size_t(dates) + size_t(lab_ids); }
};

static void serum_rows(std::ostream& output, const acmacs::chart::Chart& chart, bool all_fields, const AntigenFields& antigen_fields);

inline bool field_present(const std::vector<std::string>& src)
{
    return std::find_if(src.begin(), src.end(), [](const auto& s) -> bool { return !s.empty(); }) != src.end();
};

// ----------------------------------------------------------------------

void contents(std::ostream& output, const acmacs::chart::Chart& chart, bool all_fields)
{
    output << "<h3>" << chart.make_name() << "</h3>\n";
    auto antigens = chart.antigens();
    auto sera = chart.sera();

    std::vector<std::string> annotations(antigens->size()), reassortants(antigens->size()), passages(antigens->size()), dates(antigens->size()), lab_ids(antigens->size());
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        if (all_fields) {
            annotations[ag_no] = string::join(" ", antigen->annotations());
            reassortants[ag_no] = antigen->reassortant();
            passages[ag_no] = antigen->passage();
        }
        dates[ag_no] = antigen->date();
        lab_ids[ag_no] = string::join(" ", antigen->lab_ids());
    }
    const AntigenFields antigen_fields{field_present(annotations), field_present(reassortants), field_present(passages), field_present(dates), field_present(lab_ids)};

    output << "<table>\n";
    serum_rows(output, chart, all_fields, antigen_fields);

    auto titers = chart.titers();
      // antigens and titers
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        output << "<tr class=\"" << ((ag_no % 2) == 0 ? "even" : "odd") << ' ' << ("ag-" + std::to_string(ag_no)) << "\"><td class=\"ag-no\">" << (ag_no + 1) << "</td>";
        const auto passage_type = antigen->passage_type();
        if (all_fields) {
            output << "<td class=\"ag-name ag-" << ag_no << " passage-" << passage_type << "\">" << html_escape(antigen->name()) << "</td>";
            if (antigen_fields.annotations)
                output << "<td class=\"auto ag-annotations\">" << html_escape(annotations[ag_no]) << "</td>";
            if (antigen_fields.reassortants)
                output << "<td class=\"ag-reassortant\">" << html_escape(reassortants[ag_no]) << "</td>";
            if (antigen_fields.passages)
                output << "<td class=\"ag-passage\">" << html_escape(passages[ag_no]) << "</td>";
        }
        else {
            std::string name = antigen->name_abbreviated();
            // if (name.size() > 8) {
            //     std::vector<std::string> fields;
            //     for (auto field : acmacs::string::split(name, "/", acmacs::string::Split::RemoveEmpty))
            //         fields.emplace_back(field.substr(0, 2));
            //     name = string::join("/", fields);
            // }
            output << "<td class=\"ag-name ag-" << ag_no << " passage-" << passage_type << "\"><div class=\"tooltip\">" << html_escape(name)
                   << "<span class=\"tooltiptext\">" << html_escape(antigen->full_name()) << "</span></div></td>";
        }

          // titers
        for (auto sr_no : acmacs::range(sera->size())) {
            const auto titer = titers->titer(ag_no, sr_no);
            const char* titer_class = "regular";
            switch (titer.type()) {
              case acmacs::chart::Titer::Invalid: titer_class = "titer-invalid"; break;
              case acmacs::chart::Titer::Regular: titer_class = "titer-regular"; break;
              case acmacs::chart::Titer::DontCare: titer_class = "titer-dont-care"; break;
              case acmacs::chart::Titer::LessThan: titer_class = "titer-less-than"; break;
              case acmacs::chart::Titer::MoreThan: titer_class = "titer-more-than"; break;
              case acmacs::chart::Titer::Dodgy: titer_class = "titer-dodgy"; break;
            }
            const char* titer_pos_class = sr_no == 0 ? "titer-pos-left" : (sr_no == (sera->size() - 1) ? "titer-pos-right" : "titer-pos-middle");
            const auto sr_no_class = "sr-" + std::to_string(sr_no);
            output << "<td class=\"titer " << titer_class << ' ' << titer_pos_class << ' ' << sr_no_class << "\">" << html_escape(titer) << "</td>";
        }

        if (antigen_fields.dates)
            output << "<td class=\"ag-date\">" << dates[ag_no] << "</td>";
        if (antigen_fields.lab_ids)
            output << "<td class=\"ag-lab-id\">" << lab_ids[ag_no] << "</td>";
        output << "</tr>\n";
    }

    output << "</table>\n";

            // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
            // for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
            //     std::cout << "AG " << std::setw(num_digits) << ag_no << " " << string::join(" ", {antigen->name(), string::join(" ", antigen->annotations()), antigen->reassortant(), antigen->passage(), "[" + static_cast<std::string>(antigen->date()) + "]", string::join(" ", antigen->lab_ids())}) << (antigen->reference() ? " Ref" : "") << '\n';
            // }
            // for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
            //     std::cout << "SR " << std::setw(num_digits) << sr_no << " " << string::join(" ", {serum->name(), string::join(" ", serum->annotations()), serum->reassortant(), serum->passage(), serum->serum_id(), serum->serum_species()}) << '\n';
            // }

} // contents

// ----------------------------------------------------------------------

void serum_rows(std::ostream& output, const acmacs::chart::Chart& chart, bool all_fields, const AntigenFields& antigen_fields)
{
    auto sera = chart.sera();
    auto make_skip = [&output] (size_t count) { output << "<td colspan=\"" + std::to_string(count) + "\"></td>"; };

      // serum no
    output << "<tr>\n";
    make_skip(antigen_fields.skip_left() + 2);
    for (auto sr_no : acmacs::range(sera->size()))
        output << "<td class=\"sr-no " << ("sr-" + std::to_string(sr_no)) << "\">" << (sr_no + 1) << "</td>";
    make_skip(antigen_fields.skip_right());
    output << "</tr>\n";

    if (all_fields) {
          // serum names
        output << "<tr>\n";
        make_skip(antigen_fields.skip_left() + 2);
        for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
            output << "<td class=\"sr-name sr-" << sr_no << "\">" << html_escape(serum->name_abbreviated()) << "</td>";
        }
        make_skip(antigen_fields.skip_right());
        output << "</tr>\n";

          // serum annotations (e.g. CONC)
        std::vector<std::string> serum_annotations(sera->size());
        for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
            serum_annotations[sr_no] = string::join(" ", serum->annotations());
        }
        if (field_present(serum_annotations)) {
            output << "<tr>";
            make_skip(antigen_fields.skip_left() + 2);
            for (auto [sr_no, serum] : acmacs::enumerate(*sera))
                output << "<td class=\"sr-annotations " << ("sr-" + std::to_string(sr_no)) << "\">" << html_escape(serum_annotations[sr_no]) << "</td>";
            make_skip(antigen_fields.skip_right());
            output << "</tr>\n";
        }

          // serum_ids
        output << "<tr>";
        make_skip(antigen_fields.skip_left() + 2);
        for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
            const auto sr_no_class = "sr-" + std::to_string(sr_no);
            output << "<td class=\"sr-id " << sr_no_class << "\">" << html_escape(serum->serum_id()) << "</td>";
        }
        make_skip(antigen_fields.skip_right());
        output << "</tr>\n";
    }
    else {
        output << "<tr>\n";
        make_skip(antigen_fields.skip_left() + 2);
        for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
            std::string name = serum->name_abbreviated();
            if (name.size() > 8) {
                std::vector<std::string> fields;
                for (auto field : acmacs::string::split(name, "/", acmacs::string::Split::RemoveEmpty))
                    fields.emplace_back(field.substr(0, 2));
                name = string::join("/", fields);
            }
            output << "<td class=\"sr-name sr-" << sr_no << "\"><div class=\"tooltip\">" << html_escape(name)
                   << "<span class=\"tooltiptext\">" << html_escape(serum->full_name()) << "</span></div></td>";
        }
        make_skip(antigen_fields.skip_right());
        output << "</tr>\n";
    }

} // sera

// ----------------------------------------------------------------------

void header(std::ostream& output, std::string table_name)
{
    const char* h1 = R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
table { border-collapse: collapse; border: 1px solid grey; border-spacing: 0; }
table.td { border: 1px solid grey; }
.sr-no { text-align: center; font-size: 0.8em; }
.sr-name { text-align: center; padding: 0 0.5em; white-space: nowrap; font-weight: bold; font-size: 1em; }
.sr-name .tooltip .tooltiptext { top: -3em; left: -10em; }
.sr-id { text-align: center; white-space: nowrap; font-size: 0.5em; border-bottom: 1px solid grey; }
.sr-annotations { text-align: center; white-space: nowrap; font-size: 0.5em; }
.ag-no { text-align: right; font-size: 0.8em; }
.ag-name { text-align: left; padding: 0 0.5em; white-space: nowrap; font-weight: bold; font-size: 1em; }
.ag-name .tooltip .tooltiptext { top: -2em; left: 0; }
.passage-egg { color: #606000; }
.passage-cell { color: #000060; }
.ag-annotations { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-reassortant { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-passage { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-date { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.ag-lab-id { text-align: left; padding: 0 0.5em; white-space: nowrap; font-size: 0.8em; }
.titer { text-align: right; white-space: nowrap; padding-right: 1em; font-size: 1em; }
.titer-pos-left { border-left: 1px solid grey; }
.titer-pos-right { border-right: 1px solid grey; }
.titer-invalid   { color: red; }
.titer-regular   { color: black; }
.titer-dont-care { color: grey; }
.titer-less-than { color: green; }
.titer-more-than { color: blue; }
.titer-dodgy     { color: brown; }
tr.even td { background-color: white; }
tr.odd td { background-color: #F8F8F8; }

.tooltip {
    position: relative;
    display: inline-block;
    // border-bottom: 1px dotted black;
}

.tooltip .tooltiptext {
    visibility: hidden;
    background-color: #FF8;
    color: blue;
    text-align: left;
    padding: 5px 3px;
    border-radius: 6px;
    border: 1px solid grey;
    position: absolute;
    top: 1em;
    left: 0;
    z-index: 1;
}

.tooltip:hover .tooltiptext {
    visibility: visible;
}

tr:hover td {
  background: #FFF0E0;
}

</style>
<title>)";
    const char* h2 = R"(</title>
</head>
<body>
)";
    output << h1 << table_name << h2;

} // header

// ----------------------------------------------------------------------

void footer(std::ostream& output)
{
    const char* footer_data = R"(
</body>
</html>
)";
    output << footer_data;

} // footer

// ----------------------------------------------------------------------

std::string html_escape(std::string source)
{
    source = string::replace(source, "&", "&amp;");
    source = string::replace(source, "<", "&lt;");
    source = string::replace(source, ">", "&gt;");
    return source;

} // html_escape

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
