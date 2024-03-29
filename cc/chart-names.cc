#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-strip.hh"
#include "acmacs-base/regex.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::raise) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> fields{*this, "fields", desc{"report names with fields"}};
    option<str> format{*this, 'f', "format", dflt{"{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n"}, desc{"\n          run chart-name-format-help to list available formats"}};
    // desc{"\n          supported fields:\n            {ag_sr} {no0} {<no0} {no1} {<no1}\n            {name} {name_full_passage} {full_name_with_fields} {abbreviated_name}\n
    // {abbreviated_name_with_passage_type} {abbreviated_location_with_passage_type}\n            {abbreviated_name_with_serum_id} {designation} {name_abbreviated} {name_without_subtype}\n
    // {abbreviated_location_year} {location_abbreviated}\n            {location} {country} {continent} {latitude} {longitude}\n            {serum_id} {serum_species} {sera_with_titrations}\n {ref}
    // {date} {lab_ids} {reassortant} {passage} {passage_type} {annotations} {lineage}"}};
    option<str_array>  antigens{*this, 'a', "antigens", desc{"comma or space separated list of antigens (zero based indexes) to report for them only, or regex to match antigen name"}};
    option<str_array>  sera{*this, 's', "sera", desc{"comma or space separated list of sera (zero based indexes) to report for them only, or regex to match serum name"}};
    option<bool> antigens_only{*this, "antigens-only", desc{"report just antigens"}};
    option<bool> sera_only{*this, "sera-only", desc{"report just sera"}};
    option<bool> egg_only{*this, "egg-only", desc{"report just egg antigens/sera"}};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool> verbose{*this, 'v', "verbose"};
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const std::regex numbers{"^[0-9, ]+$"};
        std::string pattern{opt.format};
        if (opt.fields)
            pattern = "{ag_sr} {no0} {fields}";
        for (const auto& chart_filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(chart_filename, acmacs::chart::Verify::None, do_report_time(opt.report_time));
            chart->sera()->set_homologous(acmacs::chart::find_homologous::all, *chart->antigens(), acmacs::debug::no);
            bool reported{false};
            auto ag_indexes_to_report = chart->antigens()->all_indexes();
            auto sr_indexes_to_report = chart->sera()->all_indexes();
            if (opt.egg_only) {
                chart->antigens()->filter_egg(ag_indexes_to_report);
                chart->sera()->filter_egg(sr_indexes_to_report);
            }
            for (const auto& ag_data : *opt.antigens) {
                if (std::regex_match(std::begin(ag_data), std::end(ag_data), numbers)) {
                    for (const auto ag_no : acmacs::string::split_into_size_t(ag_data)) {
                        if (ag_indexes_to_report.contains(ag_no))
                            fmt::print("{}", acmacs::chart::format_antigen(pattern, *chart, ag_no, acmacs::chart::collapse_spaces_t::yes));
                    }
                }
                else {
                    for (const auto ag_no : chart->antigens()->find_by_name(std::regex{std::begin(ag_data), std::end(ag_data), acmacs::regex::icase})) {
                        if (ag_indexes_to_report.contains(ag_no))
                            fmt::print("{}", acmacs::chart::format_antigen(pattern, *chart, ag_no, acmacs::chart::collapse_spaces_t::yes));
                    }
                }
                reported = true;
            }
            for (const auto& sr_data : *opt.sera) {
                if (std::regex_match(std::begin(sr_data), std::end(sr_data), numbers)) {
                    for (const auto sr_no : acmacs::string::split_into_size_t(sr_data)) {
                        if (sr_indexes_to_report.contains(sr_no))
                            fmt::print("{}", acmacs::chart::format_serum(pattern, *chart, sr_no, acmacs::chart::collapse_spaces_t::yes));
                    }
                }
                else {
                    for (const auto sr_no : chart->sera()->find_by_name(std::regex{std::begin(sr_data), std::end(sr_data), acmacs::regex::icase})) {
                        if (sr_indexes_to_report.contains(sr_no))
                            fmt::print("{}", acmacs::chart::format_serum(pattern, *chart, sr_no, acmacs::chart::collapse_spaces_t::yes));
                    }
                }
                reported = true;
            }
            if (!reported) {
                const bool antigens_and_sera = !opt.antigens_only && !opt.sera_only;
                if (antigens_and_sera || opt.antigens_only) {
                    for (const auto ag_no : ag_indexes_to_report)
                        fmt::print("{}\n", acmacs::string::strip(format_antigen(pattern, *chart, ag_no, acmacs::chart::collapse_spaces_t::yes)));
                }
                if (antigens_and_sera || opt.sera_only) {
                    for (const auto sr_no : sr_indexes_to_report)
                        fmt::print("{}\n", acmacs::string::strip(format_serum(pattern, *chart, sr_no, acmacs::chart::collapse_spaces_t::yes)));
                }
            }
        }
    }
    catch (show_help& err) {
        fmt::print(stderr, "{}\n\nchart-name-format-help\n\n{}", err, acmacs::chart::format_help());
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
