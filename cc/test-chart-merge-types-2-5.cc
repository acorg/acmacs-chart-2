#include <cassert>
#include <cmath>

#include "acmacs-base/argv.hh"
#include "acmacs-base/debug.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void test_merge_2(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);
static void test_merge_2_frozen(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level);
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

        fmt::print(stderr, "test-chart-merge-types-2-5\n");
        test_merge_2(*chart1, *chart2, match_level);
        test_merge_2_frozen(*chart1, *chart2, match_level);
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
    fmt::print(stderr, "    test_merge_2\n");

    using namespace acmacs::chart;
    auto [merged2_chart, merge_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type2));
    // export_factory(*merged2_chart, "/d/merge-type2.ace", "test-chart-merge-types-2-5", report_time::yes);
    assert(merged2_chart->number_of_projections() == 1);

    auto merge_layout = merged2_chart->projection(0)->layout();
    const auto merge_num_antigens = merged2_chart->number_of_antigens();
    auto layout1 = chart1.projection(0)->layout();
    const auto chart1_num_antigens = chart1.number_of_antigens();

    // fmt::print(stderr, "DEBUG: test_merge_2 chart1 layout:\n{:.8f}\n", *layout1);
    // fmt::print(stderr, "DEBUG: test_merge_2 merge layout:\n{}\n", *merge_layout);

    // primary layout must be copied
    for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target)
        assert((*layout1)[index1] == (*merge_layout)[index_merge_common.index]);
    // fmt::print(stderr, "DEBUG: test_merge_2 merge: number_of_antigens:{} number_of_sera:{}\n", merge_num_antigens, merged2_chart->number_of_sera());
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

void test_merge_2_frozen(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    fmt::print(stderr, "    test_merge_2_frozen\n");

    using namespace acmacs::chart;
    auto [merged2_chart, merge_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type2));
    assert(merged2_chart->number_of_projections() == 1);
    // AD_DEBUG("merged\n{}\ndisconnected: {}", merged2_chart->make_info(), merged2_chart->titers()->having_too_few_numeric_titers());

    auto layout1 = chart1.projection(0)->layout();
    const auto chart1_num_antigens = chart1.number_of_antigens();
    const auto merge_num_antigens = merged2_chart->number_of_antigens();
    // AD_DEBUG("merge before\n{}", *merged2_chart->projection(0)->layout());

    constexpr size_t num_opt{10};
    merged2_chart->relax_incremental(0, number_of_optimizations_t{num_opt}, optimization_options{}, disconnect_having_too_few_titers::yes, remove_source_projection::yes, unmovable_non_nan_points::yes);
    assert(merged2_chart->number_of_projections() == num_opt);

    for (size_t projection_no{0}; projection_no < num_opt; ++projection_no) {
        auto merge_layout = merged2_chart->projection(projection_no)->layout();

        // for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target)
        //     AD_DEBUG("AG {:3d} {:20.17f}   {:3d} {:20.17f}", index1, (*layout1)[index1], index_merge_common.index, (*merge_layout)[index_merge_common.index]);
        // for (const auto& [index1, index_merge_common] : merge_report.sera_primary_target)
        //     AD_DEBUG("SR {:3d} {:20.17f}   {:3d} {:20.17f}", index1, (*layout1)[index1 + chart1_num_antigens], index_merge_common.index, (*merge_layout)[index_merge_common.index + merge_num_antigens]);

        // primary layout must be copied
        for (const auto& [index1, index_merge_common] : merge_report.antigens_primary_target) {
            AD_ASSERT(index1 == index_merge_common.index && (*layout1)[index1] == (*merge_layout)[index_merge_common.index], "projection: {} AG index1:{} {} index_merge_common:{} {}", projection_no, index1, (*layout1)[index1], index_merge_common.index, (*merge_layout)[index_merge_common.index]);
        }
        for (const auto& [index1, index_merge_common] : merge_report.sera_primary_target) {
            AD_ASSERT(index1 == index_merge_common.index && (*layout1)[index1 + chart1_num_antigens] == (*merge_layout)[index_merge_common.index + merge_num_antigens], "projection: {} SR index1:{} {} index_merge_common:{} {}", projection_no, index1, (*layout1)[index1 + chart1_num_antigens], index_merge_common.index, (*merge_layout)[index_merge_common.index + merge_num_antigens]);
        }
    }

} // test_merge_2_frozen

// ----------------------------------------------------------------------

void test_merge_3(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    fmt::print(stderr, "    test_merge_3\n");

    using namespace acmacs::chart;
    auto [merged3_chart, merge_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type3));

    // export_factory(*merged3_chart, "/d/merge-type3.ace", "test-chart-merge-types-2-5", report_time::yes);

    assert(merged3_chart->number_of_projections() == 1);

    auto merge_projection = merged3_chart->projection(0);
    auto merge_layout = merge_projection->layout();
    const auto merge_num_antigens = merged3_chart->number_of_antigens();
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
    fmt::print(stderr, "    test_merge_4\n");

    using namespace acmacs::chart;
    auto [merged3_chart, merge3_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type3));
    auto [merged4_chart, merge4_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type4));

    // export_factory(*merged4_chart, "/d/merge-type4.ace", "test-chart-merge-types-2-5", report_time::yes);

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

    const auto gradient = merged4_chart->projection(0)->calculate_gradient();
    const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
    assert(gradient_max < 1e-5);
    // fmt::print(stderr, "DEBUG: merged4_chart gradient: {}\n", gradient);

} // test_merge_4

// ----------------------------------------------------------------------

void test_merge_5(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    fmt::print(stderr, "    test_merge_5\n");

    using namespace acmacs::chart;
    auto [merged3_chart, merge3_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type3));
    auto [merged5_chart, merge5_report] = merge(chart1, chart2, MergeSettings(match_level, projection_merge_t::type5));

    // export_factory(*merged5_chart, "/d/merge-type5.ace", "test-chart-merge-types-2-5", report_time::yes);

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

    const auto gradient = merged5_chart->projection(0)->calculate_gradient();
    const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
    assert(gradient_max < 1e-5);
    // fmt::print(stderr, "DEBUG: merged5_chart stress: {}\n", merged5_chart->projection(0)->stress());
    // fmt::print(stderr, "DEBUG: merged5_chart gradient: {}\n", gradient);

} // test_merge_5

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
