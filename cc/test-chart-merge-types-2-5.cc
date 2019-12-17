#include <cassert>

#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void test_merge_2(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> ignore_passages{*this, "ignore-passages"};

    argument<str> chart1{*this, arg_name{"chart-file"}, mandatory};
    argument<str> chart2{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto match_level{opt.ignore_passages ? acmacs::chart::CommonAntigensSera::match_level_t::ignored : acmacs::chart::CommonAntigensSera::match_level_t::strict};
        auto chart1 = acmacs::chart::import_from_file(opt.chart1);
        auto chart2 = acmacs::chart::import_from_file(opt.chart2);

        test_merge_2(*chart1, *chart2, match_level);

    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void test_merge_2(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    using namespace acmacs::chart;
    const MergeSettings settings(match_level, projection_merge_t::type2);
    auto [merged_chart, merge_report] = merge(chart1, chart2, settings);

    assert(merged_chart->number_of_projections() == 1);

    auto merge_layout = merged_chart->projection(0)->layout();
    auto layout1 = chart1.projection(0)->layout();
    // primary layout must be copied
    for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target) {
        assert((*layout1)[index1] == (*merge_layout)[index_merge_common.index]);
    }

} // test_merge_2

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
