#include <cassert>
#include <cmath>

#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void test_merge_2(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);
static void test_merge_3(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);
static void test_merge_4(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);
static void test_merge_5(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);

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
        test_merge_3(*chart1, *chart2, match_level);
        test_merge_4(*chart1, *chart2, match_level);
        test_merge_5(*chart1, *chart2, match_level);
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
    // fmt::print(stderr, "DEBUG: test_merge_2\n");

    using namespace acmacs::chart;
    const MergeSettings settings(match_level, projection_merge_t::type2);
    auto [merged_chart, merge_report] = merge(chart1, chart2, settings);

    assert(merged_chart->number_of_projections() == 1);

    auto merge_layout = merged_chart->projection(0)->layout();
    const auto merge_num_antigens = merged_chart->number_of_antigens();
    auto layout1 = chart1.projection(0)->layout();
    const auto chart1_num_antigens = chart1.number_of_antigens();

    // fmt::print(stderr, "DEBUG: test_merge_2 chart1 layout:\n{:.8f}\n", *layout1);
    // fmt::print(stderr, "DEBUG: test_merge_2 merge layout:\n{}\n", *merge_layout);

    // primary layout must be copied
    for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target)
        assert((*layout1)[index1] == (*merge_layout)[index_merge_common.index]);
    // fmt::print(stderr, "DEBUG: test_merge_2 merge: number_of_antigens:{} number_of_sera:{}\n", merge_num_antigens, merged_chart->number_of_sera());
    for (const auto& [index1, index_merge_common] : merge_report.sera_primary_target) {
        if (!((*layout1)[index1 + chart1_num_antigens] == merge_layout->at(index_merge_common.index + merge_num_antigens)))
            fmt::print(stderr, "DEBUG: test_merge_2 sera: index1:{} index_merge:{} common:{} v1:{} v2:{}\n", index1, index_merge_common.index, index_merge_common.common, (*layout1)[index1 + chart1_num_antigens], (*merge_layout)[index_merge_common.index + merge_num_antigens]);
        assert((*layout1)[index1 + chart1_num_antigens] == merge_layout->at(index_merge_common.index + merge_num_antigens));
    }

    // non-common points of the second layout must be NaN
    for (const auto& [index2, index_merge_common] : merge_report.antigens_secondary_target) {
        if (!index_merge_common.common) {
            for (acmacs::number_of_dimensions_t dim{0}; dim < merge_layout->number_of_dimensions(); ++dim)
                assert(std::isnan(merge_layout->coordinate(index_merge_common.index, dim)));
        }
    }
    for (const auto& [index2, index_merge_common] : merge_report.sera_secondary_target) {
        if (!index_merge_common.common) {
            for (acmacs::number_of_dimensions_t dim{0}; dim < merge_layout->number_of_dimensions(); ++dim)
                assert(std::isnan(merge_layout->coordinate(index_merge_common.index + merge_num_antigens, dim)));
        }
    }

} // test_merge_2

// ----------------------------------------------------------------------

void test_merge_3(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    // fmt::print(stderr, "DEBUG: test_merge_3\n");

    using namespace acmacs::chart;
    const MergeSettings settings(match_level, projection_merge_t::type3);
    auto [merged_chart, merge_report] = merge(chart1, chart2, settings);

    // export_factory(*merged_chart, "/d/m3-3.ace", "test-chart-merge-types-2-5");

    assert(merged_chart->number_of_projections() == 1);

    auto merge_projection = merged_chart->projection(0);
    auto merge_layout = merge_projection->layout();
    const auto merge_num_antigens = merged_chart->number_of_antigens();
    auto layout1 = chart1.projection(0)->layout();
    const auto chart1_num_antigens = chart1.number_of_antigens();

    // fmt::print(stderr, "DEBUG: test_merge_3 common: {}\n", merge_report.common.points());
    // for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target) {
    //     fmt::print(stderr, "DEBUG: antigen {} {} {}\n", index1, index_merge_common.index, index_merge_common.common);
    // }
    // fmt::print(stderr, "DEBUG: test_merge_3 chart1 {:.8f}\n", *layout1);
    // fmt::print(stderr, "DEBUG: test_merge_3 merge {:.8f}\n", *merge_layout);

    // non-common points of the first layout must be copied
    for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target) {
        if (!index_merge_common.common) {
            // fmt::print(stderr, "DEBUG: {} {} {} {} {} {}\n", index_merge_common.common, index1, (*layout1)[index1], index_merge_common.index, (*merge_layout)[index_merge_common.index], (*layout1)[index1] == (*merge_layout)[index_merge_common.index]);
            assert(layout1->at(index1) == merge_layout->at(index_merge_common.index));
        }
    }
    for (const auto& [index1, index_merge_common] : merge_report.sera_primary_target) {
        if (!index_merge_common.common)
            assert(layout1->at(index1 + chart1_num_antigens) == merge_layout->at(index_merge_common.index + merge_num_antigens));
    }

    // gradient norm is non-zero, i.e. merge is not relaxed
    const auto gradient = merge_projection->calculate_gradient();
    const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
    // fmt::print(stderr, "DEBUG: test_merge_3 merge gradient: {}\n", gradient_max);
    assert(gradient_max > 10.0);
    // fmt::print(stderr, "DEBUG: test_merge_3 chart1 gradient: {}\n", chart1.projection(0)->calculate_gradient());

} // test_merge_3

// ----------------------------------------------------------------------

void test_merge_4(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    // fmt::print(stderr, "DEBUG: test_merge_4\n");

    using namespace acmacs::chart;
    auto [merged3_chart, merge3_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type3));
    auto [merged4_chart, merge4_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type4));

    // export_factory(*merged3_chart, "/d/m3.ace", "test-chart-merge-types-2-5");
    // export_factory(*merged4_chart, "/d/m4.ace", "test-chart-merge-types-2-5");

    assert(merged4_chart->number_of_projections() == 1);

    auto layout1 = chart1.projection(0)->layout();
    auto merge3_layout = merged3_chart->projection(0)->layout();
    auto merge4_layout = merged4_chart->projection(0)->layout();
    const auto merge_num_antigens = merged4_chart->number_of_antigens();
    const auto chart1_num_antigens = chart1.number_of_antigens();

    // fmt::print(stderr, "DEBUG: chart1 {:.8f}\n", *layout1);
    // fmt::print(stderr, "DEBUG: merge3 {:.8f}\n", *merge3_layout);
    // fmt::print(stderr, "DEBUG: merge4 {:.8f}\n", *merge4_layout);

    // coordinates of points of the first chart (including common) must be the same in merged3_chart and merged4_chart
    for (const auto& [index1, index_merge_common] : merge4_report.antigens_primary_target) {
        if (!index_merge_common.common)
            assert(layout1->at(index1) == merge4_layout->at(index_merge_common.index));
        assert(merge3_layout->at(index_merge_common.index) == merge4_layout->at(index_merge_common.index));
    }
    for (const auto& [index1, index_merge_common] : merge4_report.sera_primary_target) {
        if (!index_merge_common.common)
            assert(layout1->at(index1 + chart1_num_antigens) == merge4_layout->at(index_merge_common.index + merge_num_antigens));
        assert(merge3_layout->at(index_merge_common.index + merge_num_antigens) == merge4_layout->at(index_merge_common.index + merge_num_antigens));
    }

    // // gradient for all points (including common) in the chart2 must be about zero
    // const auto gradient = merged4_chart->projection(0)->calculate_gradient();
    // fmt::print(stderr, "DEBUG: merged4_chart gradient: {}\n", gradient);
    // for (const auto& [index2, index_merge_common] : merge4_report.antigens_secondary_target)
    //     assert(std::abs(gradient.at(index_merge_common.index)) < 1e-5);
    // for (const auto& [index2, index_merge_common] : merge4_report.sera_secondary_target)
    //     assert(std::abs(gradient.at(index_merge_common.index + merge_num_antigens)) < 1e-5);

} // test_merge_4

// ----------------------------------------------------------------------

void test_merge_5(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    // fmt::print(stderr, "DEBUG: test_merge_5\n");

    using namespace acmacs::chart;
    auto [merged3_chart, merge3_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type3));
    auto [merged5_chart, merge5_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type5));

    assert(merged5_chart->number_of_projections() == 1);

    auto layout1 = chart1.projection(0)->layout();
    auto merge3_layout = merged3_chart->projection(0)->layout();
    auto merge5_layout = merged5_chart->projection(0)->layout();
    const auto merge_num_antigens = merged5_chart->number_of_antigens();
    const auto chart1_num_antigens = chart1.number_of_antigens();

    // fmt::print(stderr, "DEBUG: chart1 {:.8f}\n", *layout1);
    // fmt::print(stderr, "DEBUG: merge3 {:.8f}\n", *merge3_layout);
    // fmt::print(stderr, "DEBUG: merge5 {:.8f}\n", *merge5_layout);

    // coordinates of points of the first chart (including common) must be the same in layout1, merged3_chart and merged5_chart
    for (const auto& [index1, index_merge_common] : merge5_report.antigens_primary_target) {
        // fmt::print(stderr, "DEBUG: AG {} {} {} {} {} {}\n", index1, index_merge_common.index, index_merge_common.common, layout1->at(index1), merge3_layout->at(index_merge_common.index), merge5_layout->at(index_merge_common.index));
        assert(layout1->at(index1) == merge5_layout->at(index_merge_common.index));
        if (!index_merge_common.common)
            assert(merge3_layout->at(index_merge_common.index) == merge5_layout->at(index_merge_common.index));
    }
    for (const auto& [index1, index_merge_common] : merge5_report.sera_primary_target) {
        assert(layout1->at(index1 + chart1_num_antigens) == merge5_layout->at(index_merge_common.index + merge_num_antigens));
        if (!index_merge_common.common)
            assert(merge3_layout->at(index_merge_common.index + merge_num_antigens) == merge5_layout->at(index_merge_common.index + merge_num_antigens));
    }

} // test_merge_5

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
