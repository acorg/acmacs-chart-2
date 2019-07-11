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

static void contents(std::ostream& output, const acmacs::chart::Chart& chart, const groups_t& groups, const groups_t& serum_groups, bool all_fields, bool rest_group, bool only_with_titers);
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
    option<str_array> serum_groups{*this, "serum-group", desc{"comma separated indexes of sera in the group"}};
    option<str_array> serum_group_names{*this, "serum-group-name"};
    option<str>       sera{*this, "sera", desc{"just show the specified sera (comma separated indexes) in the table"}};
    option<bool>      no_rest_group{*this, "no-rest-group", desc{"do not show bottom group with antigens not found in any of the specified groups"}};
    option<bool>      only_with_titers{*this, "only-with-titers", desc{"do not show antigens that have no titers against listed sera"}};
    option<bool>      all_fields{*this, "all-fields"};
    option<bool>      open{*this, "open"};
    option<bool>      report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str>     chart{*this, arg_name{"chart"}, mandatory};
    argument<str>     output{*this, arg_name{"output.html"}};
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
            groups.emplace_back(group_name, acmacs::string::split_into_size_t(opt.groups->at(group_no), ","));
        }

        groups_t serum_groups;
        for (size_t group_no = 0; group_no < opt.serum_groups->size(); ++group_no) {
            const std::string serum_group_name = group_no < opt.serum_group_names->size() ? static_cast<std::string>(opt.serum_group_names->at(group_no)) : std::string{};
            serum_groups.emplace_back(serum_group_name, acmacs::string::split_into_size_t(opt.serum_groups->at(group_no), ","));
        }
        if (opt.sera.has_value() && serum_groups.empty())
            serum_groups.emplace_back("", acmacs::string::split_into_size_t(*opt.sera, ","));

        header(output, chart->make_name());
        contents(output, *chart, groups, serum_groups, opt.all_fields, !opt.no_rest_group, opt.only_with_titers);
        footer(output);
        output.close();

        acmacs::open_or_quicklook(opt.open || !opt.output.has_value(), false, output_filename);
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

static std::vector<size_t> serum_rows(std::ostream& output, const acmacs::chart::Chart& chart, const groups_t& serum_groups, bool all_fields, const AntigenFields& antigen_fields, const acmacs::chart::PointIndexList& having_too_few_numeric_titers, bool rest_group);

inline bool field_present(const std::vector<std::string>& src)
{
    return std::find_if(src.begin(), src.end(), [](const auto& s) -> bool { return !s.empty(); }) != src.end();
};

inline const char* make_titer_class(const acmacs::chart::Titer& titer)
{
    switch (titer.type()) {
        case acmacs::chart::Titer::Invalid:
            return "titer-invalid";
        case acmacs::chart::Titer::Regular:
            return "titer-regular";
        case acmacs::chart::Titer::DontCare:
            return "titer-dont-care";
        case acmacs::chart::Titer::LessThan:
            return "titer-less-than";
        case acmacs::chart::Titer::MoreThan:
            return "titer-more-than";
        case acmacs::chart::Titer::Dodgy:
            return "titer-dodgy";
    }
    return "titer-invalid";
}

// ----------------------------------------------------------------------

void contents(std::ostream& output, const acmacs::chart::Chart& chart, const groups_t& groups, const groups_t& serum_groups, bool all_fields, bool rest_group, bool only_with_titers)
{
    output << "<h3>" << chart.make_name() << "</h3>\n";
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    const auto number_of_sera = sera->size();
    const auto indexes_having_too_few_numeric_titers = chart.titers()->having_too_few_numeric_titers();
    const auto number_of_sera_to_show =
        rest_group ? number_of_sera : std::accumulate(std::begin(serum_groups), std::end(serum_groups), 0UL, [](size_t sum, const auto& group) { return sum + group.second.size(); });

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
    const auto sera_rest = serum_rows(output, chart, serum_groups, all_fields, antigen_fields, indexes_having_too_few_numeric_titers, rest_group);

    auto titers = chart.titers();

    const auto make_antigen = [&](size_t ag_no, std::string group_name, const std::vector<acmacs::chart::Titer>& titers_of_antigen) {
        auto antigen = antigens->at(ag_no);
        output << "<tr class=\"" << ((ag_no % 2) == 0 ? "even" : "odd") << ' ' << ("ag-" + std::to_string(ag_no)) << "\">";
        output << "<td class=\"group-name\">" << group_name << "</td>";
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
            output << "<td class=\"ag-name ag-" << ag_no << " passage-" << passage_type << has_too_few_numeric_titers_class << "\"><div class=\"tooltip\">" << html_escape(name)
                   << "<span class=\"tooltiptext\">" << html_escape(antigen->full_name()) << "</span></div></td>";
        }

        // titers
        for (const auto& group : serum_groups) {
            bool group_begin = true;
            for (auto sr_no : group.second) {
                output << "<td class=\"titer titer-pos-middle sr-" << sr_no << ' ' << make_titer_class(titers_of_antigen[sr_no]) << (group_begin ? " sr-group-begin" : "") << "\">"
                       << html_escape(titers_of_antigen[sr_no]) << "</td>";
                group_begin = false;
            }
        }
        if (!sera_rest.empty()) {
            bool group_begin = true;
            for (auto sr_no : sera_rest) {
                output << "<td class=\"titer titer-pos-middle sr-" << sr_no << ' ' << make_titer_class(titers_of_antigen[sr_no]) << (group_begin ? " sr-group-begin" : "") << "\">"
                       << html_escape(titers_of_antigen[sr_no]) << "</td>";
                group_begin = false;
            }
        }

        if (antigen_fields.dates)
            output << "<td class=\"ag-date\">" << dates[ag_no] << "</td>";
        if (antigen_fields.lab_ids)
            output << "<td class=\"ag-lab-id\">" << lab_ids[ag_no] << "</td>";
        output << "</tr>\n";
    };

    const auto make_group_sepeartor = [&]() {
        output << "<tr class=\"group-separator\"><td colspan=" << (antigen_fields.skip_left() + 2 + number_of_sera_to_show + antigen_fields.skip_right()) << ">A</td></tr>";
    };

    std::vector<bool> antigens_written(antigens->size());

    // antigens and titers
    bool first_group = true;
    for (const auto& group : groups) {
        if (!first_group)
            make_group_sepeartor();
        for (auto ag_no : group.second) {
            std::vector<acmacs::chart::Titer> titers_of_antigen(number_of_sera);
            std::transform(acmacs::index_iterator(0UL), acmacs::index_iterator(number_of_sera), titers_of_antigen.begin(), [ag_no, &titers](size_t sr_no) { return titers->titer(ag_no, sr_no); });
            if (!only_with_titers || std::any_of(std::begin(titers_of_antigen), std::end(titers_of_antigen), [](const auto& titer) { return !titer.is_dont_care(); }))
                make_antigen(ag_no, group.first, titers_of_antigen);
            antigens_written[ag_no] = true;
        }
        first_group = false;
    }
    if (rest_group) {
        bool rest_antigens = false;
        for (size_t ag_no = 0; ag_no < antigens_written.size(); ++ag_no) {
            if (!antigens_written[ag_no]) {
                std::vector<acmacs::chart::Titer> titers_of_antigen(number_of_sera);
                std::transform(acmacs::index_iterator(0UL), acmacs::index_iterator(number_of_sera), titers_of_antigen.begin(), [ag_no, &titers](size_t sr_no) { return titers->titer(ag_no, sr_no); });
                if (!only_with_titers || std::any_of(std::begin(titers_of_antigen), std::end(titers_of_antigen), [](const auto& titer) { return !titer.is_dont_care(); })) {
                    if (!rest_antigens && !first_group)
                        make_group_sepeartor();
                    make_antigen(ag_no, std::string{}, titers_of_antigen);
                    rest_antigens = true;
                }
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

std::vector<size_t> serum_rows(std::ostream& output, const acmacs::chart::Chart& chart, const groups_t& serum_groups, bool all_fields, const AntigenFields& antigen_fields,
                const acmacs::chart::PointIndexList& having_too_few_numeric_titers, bool rest_group)
{
    auto sera = chart.sera();
    const auto number_of_sera = sera->size();
    const auto number_of_antigens = chart.number_of_antigens();
    auto make_skip = [&output](size_t count) { output << "<td colspan=\"" + std::to_string(count) + "\"></td>"; };

    // groups
    output << "<tr>\n";
    make_skip(antigen_fields.skip_left() + 2);
    size_t sera_in_groups = 0;
    for (const auto& group : serum_groups) {
        output << "<td class=\"sr-group sr-group-begin\" colspan=\"" << group.second.size() << "\">" << group.first << "</td>";
        sera_in_groups += group.second.size();
    }
    if (rest_group && sera_in_groups < number_of_sera)
        output << "<td class=\"sr-group sr-group-begin\" colspan=\"" << (number_of_sera - sera_in_groups) << "\"></td>";
    make_skip(antigen_fields.skip_right());
    output << "</tr>\n";

    std::vector<bool> sera_written(number_of_sera);

    // serum no
    output << "<tr>\n";
    make_skip(antigen_fields.skip_left() + 2);
    for (const auto& group : serum_groups) {
        bool group_begin = true;
        for (auto sr_no : group.second) {
            output << "<td class=\"sr-no " << ("sr-" + std::to_string(sr_no)) << (group_begin ? " sr-group-begin" : "") << "\">" << (sr_no + 1) << "</td>";
            sera_written[sr_no] = true;
            group_begin = false;
        }
    }
    std::vector<size_t> sera_rest;
    if (rest_group && sera_in_groups < number_of_sera) {
        bool group_begin = true;
        for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
            if (!sera_written[sr_no]) {
                output << "<td class=\"sr-no " << ("sr-" + std::to_string(sr_no)) << (group_begin ? " sr-group-begin" : "") << "\">" << (sr_no + 1) << "</td>";
                sera_rest.push_back(sr_no);
                group_begin = false;
            }
        }
    }
    make_skip(antigen_fields.skip_right());
    output << "</tr>\n";

    auto show_row = [&output, &make_skip, &antigen_fields, &serum_groups, &sera_rest, &sera](auto show_func) {
        output << "<tr>\n";
        make_skip(antigen_fields.skip_left() + 2);
        for (const auto& group : serum_groups) {
            bool group_begin = true;
            for (auto sr_no : group.second) {
                show_func(sr_no, *sera->at(sr_no), group_begin);
                group_begin = false;
            }
        }
        if (!sera_rest.empty()) {
            bool group_begin = true;
            for (auto sr_no : sera_rest) {
                show_func(sr_no, *sera->at(sr_no), group_begin);
                group_begin = false;
            }
        }
        make_skip(antigen_fields.skip_right());
        output << "</tr>\n";
    };

    if (all_fields) {
        const auto show_serum_name = [&having_too_few_numeric_titers, number_of_antigens, &output](size_t sr_no, const auto& serum, bool group_begin) {
            const char* has_too_few_numeric_titers_class = having_too_few_numeric_titers.contains(sr_no + number_of_antigens) ? " too-few-numeric-titers" : "";
            output << "<td class=\"sr-name sr-" << sr_no << " passage-" << serum.passage().passage_type() << has_too_few_numeric_titers_class << (group_begin ? " sr-group-begin" : "") << "\">"
                   << html_escape(serum.name_abbreviated()) << "</td>";
        };

        show_row(show_serum_name);

        // serum annotations (e.g. CONC)
        std::vector<std::string> serum_annotations(number_of_sera);
        for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
            serum_annotations[sr_no] = string::join(" ", sera->at(sr_no)->annotations());
        if (field_present(serum_annotations)) {
            auto show_annotations = [&output, &serum_annotations](size_t sr_no, const auto& /*serum*/, bool group_begin) {
                output << "<td class=\"sr-annotations " << ("sr-" + std::to_string(sr_no)) << (group_begin ? " sr-group-begin" : "") << "\">" << html_escape(serum_annotations[sr_no]) << "</td>";
            };

            show_row(show_annotations);
        }

        // serum_ids
        const auto show_serum_ids = [&output](size_t sr_no, const auto& serum, bool group_begin) {
            output << "<td class=\"sr-id " << "sr-" << sr_no << (group_begin ? " sr-group-begin" : "") << "\">" << html_escape(serum.serum_id()) << "</td>";
        };

        show_row(show_serum_ids);
    }
    else {
        auto show_serum = [&having_too_few_numeric_titers, number_of_antigens, &output](size_t sr_no, const auto& serum, bool group_begin) {
            std::string name = serum.name_abbreviated();
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
            output << "<td class=\"sr-name sr-" << sr_no << " passage-" << serum.passage().passage_type() << has_too_few_numeric_titers_class << (group_begin ? " sr-group-begin" : "")
                   << "\"><div class=\"tooltip\">" << html_escape(name) << "<span class=\"tooltiptext\">" << html_escape(serum.full_name_with_passage()) << "</span></div></td>";
        };

        show_row(show_serum);
    }

    return sera_rest;

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
.sr-group-begin { border-left: 1px solid black; }
td.sr-group { padding: 0 0.5em; text-align: center; }
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
