#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-strip.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> fields{*this, "fields", desc{"report names with fields"}};
    option<str>  format{*this, 'f', "format", dflt{"{ag_sr} {no0} {full_name_with_passage} {serum_species} [{date}] {lab_ids} {ref}"}, desc{"\n          run chart-name-format-help to list available formats"}};
                        // desc{"\n          supported fields:\n            {ag_sr} {no0} {<no0} {no1} {<no1}\n            {name} {full_name_with_passage} {full_name_with_fields} {abbreviated_name}\n            {abbreviated_name_with_passage_type} {abbreviated_location_with_passage_type}\n            {abbreviated_name_with_serum_id} {designation} {name_abbreviated} {name_without_subtype}\n            {abbreviated_location_year} {location_abbreviated}\n            {location} {country} {continent} {latitude} {longitude}\n            {serum_id} {serum_species} {sera_with_titrations}\n            {ref} {date} {lab_ids} {reassortant} {passage} {passage_type} {annotations} {lineage}"}};
    option<str>  antigens{*this, 'a', "antigens", desc{"comma or space separated list of antigens (zero based indexes) to report for them only"}};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool> verbose{*this, 'v', "verbose"};
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::string pattern{opt.format};
        if (opt.fields)
            pattern = "{ag_sr} {no0} {full_name_with_fields}";
        for (const auto& chart_filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(chart_filename, acmacs::chart::Verify::None, do_report_time(opt.report_time));
            chart->sera()->set_homologous(acmacs::chart::find_homologous::all, *chart->antigens(), acmacs::debug::no);
            if (opt.antigens) {
                for (const auto ag_no : acmacs::string::split_into_size_t(*opt.antigens))
                    fmt::print("{}\n", acmacs::string::strip(acmacs::chart::format_antigen(pattern, *chart, ag_no)));
            }
            else {
                for (const auto ag_no : acmacs::range(chart->number_of_antigens()))
                    fmt::print("{}\n", acmacs::string::strip(format_antigen(pattern, *chart, ag_no)));
                for (const auto sr_no : acmacs::range(chart->number_of_sera()))
                    fmt::print("{}\n", acmacs::string::strip(format_serum(pattern, *chart, sr_no)));
            }
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
