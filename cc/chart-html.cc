#include <iostream>
#include <fstream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

using group_t = std::vector<size_t>;
using groups_t = std::vector<std::pair<std::string, group_t>>;

static void contents(std::ostream& output, const acmacs::chart::Chart& chart, const groups_t& groups, const group_t& sera_to_show, bool all_fields, bool rest_group);
static void header(std::ostream& output, std::string table_name);
static void footer(std::ostream& output);
static std::string html_escape(std::string source);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> groups{*this, "group", desc{"comma separated indexes of antigens in the group"}};
    option<str_array> group_names{*this, "group-name"};
    option<str>       sera{*this, "sera", desc{"just show the specified sera (comma separated indexes) in the table"}};
    option<bool>      no_rest_group{*this, "no-rest-group", desc{"do not show bottom group with antigens not found in any of the specified groups"}};
    option<bool> all_fields{*this, "all-fields"};
    option<bool> open{*this, "open"};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str> chart{*this, arg_name{"chart"}, mandatory};
    argument<str> output{*this, arg_name{"output.html"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        auto chart = acmacs::chart::import_from_file(opt.chart, acmacs::chart::Verify::None, report);

        acmacs::file::temp temp_file(".html");
        const std::string output_filename = opt.output.has_value() ? static_cast<std::string>(opt.output) : static_cast<std::string>(temp_file);
        std::ofstream output{output_filename};

        groups_t groups;
        for (size_t group_no = 0; group_no < opt.groups->size(); ++group_no) {
            const std::string group_name = group_no < opt.group_names->size() ? static_cast<std::string>(opt.group_names->at(group_no)) : std::string{};
            groups.emplace_back(group_name, acmacs::string::split_into_uint(opt.groups->at(group_no), ","));
        }

        group_t sera_to_show;
        if (opt.sera.has_value())
            sera_to_show = acmacs::string::split_into_uint(*opt.sera, ",");
        else
            acmacs::fill_with_indexes(sera_to_show, chart->number_of_sera());

        header(output, chart->make_name());
        contents(output, *chart, groups, sera_to_show, opt.all_fields, !opt.no_rest_group);
        footer(output);
        output.close();

        if (opt.open || !opt.output.has_value())
            acmacs::open(output_filename);
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

    size_t skip_left() const { return size_t(annotations) + size_t(reassortants) + size_t(passages) + 1; } // +1 for group name
    size_t skip_right() const { return size_t(dates) + size_t(lab_ids); }
};

static void serum_rows(std::ostream& output, const acmacs::chart::Chart& chart, const group_t& sera_to_show, bool all_fields, const AntigenFields& antigen_fields, const acmacs::chart::PointIndexList& having_too_few_numeric_titers);

inline bool field_present(const std::vector<std::string>& src)
{
    return std::find_if(src.begin(), src.end(), [](const auto& s) -> bool { return !s.empty(); }) != src.end();
};

// ----------------------------------------------------------------------

void contents(std::ostream& output, const acmacs::chart::Chart& chart, const groups_t& groups, const group_t& sera_to_show, bool all_fields, bool rest_group)
{
    output << "<h3>" << chart.make_name() << "</h3>\n";
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    const auto indexes_having_too_few_numeric_titers = chart.titers()->having_too_few_numeric_titers();

    std::vector<std::string> annotations(antigens->size()), reassortants(antigens->size()), passages(antigens->size()), dates(antigens->size()), lab_ids(antigens->size());
    for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
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
    serum_rows(output, chart, sera_to_show, all_fields, antigen_fields, indexes_having_too_few_numeric_titers);

    auto titers = chart.titers();

    const auto make_antigen = [&](size_t ag_no, size_t group_size, std::string group_name) {
        auto antigen = antigens->at(ag_no);
        output << "<tr class=\"" << ((ag_no % 2) == 0 ? "even" : "odd") << ' ' << ("ag-" + std::to_string(ag_no)) << "\">";
        if (group_size)
            output << "<td class=\"group-name\" rowspan=" << group_size << '>' << group_name << "</td>";
        output << "<td class=\"ag-no\">" << (ag_no + 1) << "</td>";
        const auto passage_type = antigen->passage_type();
        const char* has_too_few_numeric_titers_class = indexes_having_too_few_numeric_titers.contains(ag_no) ? " too-few-numeric-titers" : "";
        if (all_fields) {
            output << "<td class=\"ag-name ag-" << ag_no << " passage-" << passage_type << has_too_few_numeric_titers_class << "\">" << html_escape(antigen->name()) << "</td>";
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
            output << "<td class=\"ag-name ag-" << ag_no << " passage-" << passage_type << has_too_few_numeric_titers_class
                   << "\"><div class=\"tooltip\">" << html_escape(name) << "<span class=\"tooltiptext\">"
                   << html_escape(antigen->full_name()) << "</span></div></td>";
        }

        // titers
        for (auto sr_no : sera_to_show) {
            const auto titer = titers->titer(ag_no, sr_no);
            const char* titer_class = "regular";
            switch (titer.type()) {
                case acmacs::chart::Titer::Invalid:
                    titer_class = "titer-invalid";
                    break;
                case acmacs::chart::Titer::Regular:
                    titer_class = "titer-regular";
                    break;
                case acmacs::chart::Titer::DontCare:
                    titer_class = "titer-dont-care";
                    break;
                case acmacs::chart::Titer::LessThan:
                    titer_class = "titer-less-than";
                    break;
                case acmacs::chart::Titer::MoreThan:
                    titer_class = "titer-more-than";
                    break;
                case acmacs::chart::Titer::Dodgy:
                    titer_class = "titer-dodgy";
                    break;
            }
            const char* titer_pos_class = sr_no == sera_to_show.front() ? "titer-pos-left" : (sr_no == sera_to_show.back() ? "titer-pos-right" : "titer-pos-middle");
            const auto sr_no_class = "sr-" + std::to_string(sr_no);
            output << "<td class=\"titer " << titer_class << ' ' << titer_pos_class << ' ' << sr_no_class << "\">" << html_escape(titer) << "</td>";
        }

        if (antigen_fields.dates)
            output << "<td class=\"ag-date\">" << dates[ag_no] << "</td>";
        if (antigen_fields.lab_ids)
            output << "<td class=\"ag-lab-id\">" << lab_ids[ag_no] << "</td>";
        output << "</tr>\n";
    };

    const auto make_group_sepeartor = [&]() {
        output << "<tr class=\"group-separator\"><td colspan=" << (antigen_fields.skip_left() + 2 + sera_to_show.size() + antigen_fields.skip_right()) << ">A</td></tr>";
    };

    std::vector<bool> antigens_written(antigens->size());

    // antigens and titers
    bool first_group = true;
    for (const auto& group : groups) {
        if (!first_group)
            make_group_sepeartor();
        auto group_size = group.second.size();
        for (auto ag_no : group.second) {
            make_antigen(ag_no, group_size, group.first);
            group_size = 0;
            antigens_written[ag_no] = true;
        }
        first_group = false;
    }
    if (rest_group) {
        bool rest_antigens = false;
        for (size_t ag_no = 0; ag_no < antigens_written.size(); ++ag_no) {
            if (!antigens_written[ag_no]) {
                if (!rest_antigens && !first_group)
                    make_group_sepeartor();
                make_antigen(ag_no, 1, std::string{});
                rest_antigens = true;
            }
        }
    }

    output << "</table>\n";

    // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
    // for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
    //     std::cout << "AG " << std::setw(num_digits) << ag_no << " " << string::join(" ", {antigen->name(), string::join(" ", antigen->annotations()), antigen->reassortant(), antigen->passage(), "["
    //     + static_cast<std::string>(antigen->date()) + "]", string::join(" ", antigen->lab_ids())}) << (antigen->reference() ? " Ref" : "") << '\n';
    // }
    // for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
    //     std::cout << "SR " << std::setw(num_digits) << sr_no << " " << string::join(" ", {serum->name(), string::join(" ", serum->annotations()), serum->reassortant(), serum->passage(),
    //     serum->serum_id(), serum->serum_species()}) << '\n';
    // }

} // contents

// ----------------------------------------------------------------------

void serum_rows(std::ostream& output, const acmacs::chart::Chart& chart, const group_t& sera_to_show, bool all_fields, const AntigenFields& antigen_fields, const acmacs::chart::PointIndexList& having_too_few_numeric_titers)
{
    auto sera = chart.sera();
    const auto number_of_antigens = chart.number_of_antigens();
    auto make_skip = [&output] (size_t count) { output << "<td colspan=\"" + std::to_string(count) + "\"></td>"; };

      // serum no
    output << "<tr>\n";
    make_skip(antigen_fields.skip_left() + 2);
    for (auto sr_no : sera_to_show)
        output << "<td class=\"sr-no " << ("sr-" + std::to_string(sr_no)) << "\">" << (sr_no + 1) << "</td>";
    make_skip(antigen_fields.skip_right());
    output << "</tr>\n";

    if (all_fields) {
          // serum names
        output << "<tr>\n";
        make_skip(antigen_fields.skip_left() + 2);
        for (auto sr_no : sera_to_show) {
            auto serum = sera->at(sr_no);
            const char* has_too_few_numeric_titers_class = having_too_few_numeric_titers.contains(sr_no + number_of_antigens) ? " too-few-numeric-titers" : "";
            output << "<td class=\"sr-name sr-" << sr_no << " passage-" << serum->passage().passage_type() << has_too_few_numeric_titers_class
                   << "\">" << html_escape(serum->name_abbreviated()) << "</td>";
        }
        make_skip(antigen_fields.skip_right());
        output << "</tr>\n";

          // serum annotations (e.g. CONC)
        std::vector<std::string> serum_annotations(sera->size());
        for (auto sr_no : sera_to_show) {
            auto serum = sera->at(sr_no);
            serum_annotations[sr_no] = string::join(" ", serum->annotations());
        }
        if (field_present(serum_annotations)) {
            output << "<tr>";
            make_skip(antigen_fields.skip_left() + 2);
            for (auto sr_no : sera_to_show)
                output << "<td class=\"sr-annotations " << ("sr-" + std::to_string(sr_no)) << "\">" << html_escape(serum_annotations[sr_no]) << "</td>";
            make_skip(antigen_fields.skip_right());
            output << "</tr>\n";
        }

          // serum_ids
        output << "<tr>";
        make_skip(antigen_fields.skip_left() + 2);
        for (auto sr_no : sera_to_show) {
            auto serum = sera->at(sr_no);
            const auto sr_no_class = "sr-" + std::to_string(sr_no);
            output << "<td class=\"sr-id " << sr_no_class << "\">" << html_escape(serum->serum_id()) << "</td>";
        }
        make_skip(antigen_fields.skip_right());
        output << "</tr>\n";
    }
    else {
        output << "<tr>\n";
        make_skip(antigen_fields.skip_left() + 2);
        for (auto sr_no : sera_to_show) {
            auto serum = sera->at(sr_no);
            std::string name = serum->name_abbreviated();
            if (name.size() > 10) {
                std::vector<std::string> fields;
                for (auto field : acmacs::string::split(name, "/", acmacs::string::Split::RemoveEmpty)) {
                    if (field.size() > 3)
                        fields.push_back(string::concat(field.substr(0, 3), "..."));
                    else
                        fields.emplace_back(field);
                }
                name = string::join("/", fields);
            }
            const char* has_too_few_numeric_titers_class = having_too_few_numeric_titers.contains(sr_no + number_of_antigens) ? " too-few-numeric-titers" : "";
            output << "<td class=\"sr-name sr-" << sr_no << " passage-" << serum->passage().passage_type() << has_too_few_numeric_titers_class
                   << "\"><div class=\"tooltip\">" << html_escape(name)
                   << "<span class=\"tooltiptext\">" << html_escape(serum->full_name_with_passage()) << "</span></div></td>";
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
.passage-egg.too-few-numeric-titers { color: #E0E0C0; }
.passage-cell.too-few-numeric-titers { color: #C0C0E0; }
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
/* tr.even td { background-color: white; } */
/* tr.odd td { background-color: #F8F8F8; } */
td.group-name { padding: 0.3em 0.1em 0 0.3em; font-weight: bold; vertical-align: top; }
tr.group-separator td { height: 1em; border: 1px solid grey; color: transparent; }

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
