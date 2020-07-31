#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  sera{*this, 's', "sera", desc{"comma or space separated list of serum indexes (0-based)"}};
    option<bool> dry_run{*this, "dry-run", desc{"report antigens, do not produce output"}};
    option<bool> remove_projections{*this, "remove-projections"};

    argument<str> source{*this, arg_name{"source-chart"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::chart::PointIndexList sera_titrated_against{opt.sera.empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(*opt.sera)};
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(*opt.source)};
        auto titers = chart.titers();
        acmacs::ReverseSortedIndexes antigens_to_keep;
        for (auto serum_index : sera_titrated_against) {
            antigens_to_keep.add(*titers->having_titers_with(serum_index + chart.number_of_antigens()));
        }
        AD_INFO("antigens_to_keep:  {} {}", antigens_to_keep.size(), antigens_to_keep);
        acmacs::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
        antigens_to_remove.remove(antigens_to_keep);
        AD_INFO("antigens_to_remove:  {} {}", antigens_to_remove.size(), antigens_to_remove);
        if (!opt.dry_run) {
            if (!antigens_to_remove.empty())
                chart.remove_antigens(antigens_to_remove);
            acmacs::chart::export_factory(chart, *opt.output, opt.program_name());
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
