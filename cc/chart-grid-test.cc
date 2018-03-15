#include <iostream>
#include <algorithm>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

class GridTest
{
 public:
    using Chart = acmacs::chart::ChartModify;
    using Projection = acmacs::chart::ProjectionModifyP;
    using Coordinates = acmacs::Coordinates;
    using Stress = acmacs::chart::Stress<double>;

    GridTest(Chart& chart, size_t projection_no, double grid_step)
        : chart_(chart), projection_(chart.projection_modify(projection_no)),
          grid_step_(grid_step),
          original_layout_(*projection_->layout()), stress_(chart.make_stress<double>(projection_no))
        {
        }

    struct Result
    {
        enum diagnosis_t { normal, trapped, hemisphering };

        Result(size_t a_point_no) : point_no(a_point_no), diagnosis(normal) {}
        Result(size_t a_point_no, diagnosis_t a_diagnosis, const Coordinates& a_pos, double a_distance, double diff)
            : point_no(a_point_no), diagnosis(a_diagnosis), pos(a_pos), distance(a_distance), contribution_diff(diff) {}
        operator bool() const { return diagnosis != normal; }
        std::string report() const;

        size_t point_no;
        diagnosis_t diagnosis;
        Coordinates pos;
        double distance;
        double contribution_diff;
    };

    std::string point_name(size_t point_no) const;
    Result test_point(size_t point_no);
    void test_all();

 private:
    Chart& chart_;
    Projection projection_;
    const double grid_step_;          // acmacs-c2: 0.01
    const double hemisphering_distance_threshold_ = 1.0; // from acmacs-c2 hemi-local test: 1.0
    const double hemisphering_stress_threshold_ = 0.25;  // stress diff within threshold -> hemisphering, from acmacs-c2 hemi-local test: 0.25
    const acmacs::Layout original_layout_;
    const Stress stress_;

    bool antigen(size_t point_no) const { return point_no < chart_.number_of_antigens(); }
    size_t antigen_serum_no(size_t point_no) const { return antigen(point_no) ? point_no : (point_no - chart_.number_of_antigens()); }
    // acmacs::Area area_for(size_t point_no) const;
    acmacs::Area area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const;

}; // class GridTest

// ----------------------------------------------------------------------

std::string GridTest::point_name(size_t point_no) const
{
    if (antigen(point_no)) {
        return "AG " + acmacs::to_string(point_no) + ' ' + (*chart_.antigens())[point_no]->full_name();
    }
    else {
        const auto serum_no = antigen_serum_no(point_no);
        return "SR " + acmacs::to_string(serum_no) + ' ' + (*chart_.sera())[serum_no]->full_name();
    }

} // GridTest::point_name

// ----------------------------------------------------------------------

// acmacs::Area GridTest::area_for(size_t point_no) const
// {
//     const auto& table_distances = stress_.table_distances();
//     auto it_regular = table_distances.begin_regular_for(point_no);
//     const auto coord0 = original_layout_.get(it_regular->point_1 == point_no ? it_regular->point_2 : it_regular->point_1);
//     acmacs::Area result(coord0 - it_regular->table_distance, coord0 + it_regular->table_distance);
//     ++it_regular;
//     for (const auto last_regular = table_distances.end_regular_for(point_no); it_regular != last_regular; ++it_regular) {
//         const auto coord = original_layout_.get(it_regular->point_1 == point_no ? it_regular->point_2 : it_regular->point_1);
//         const auto radius = it_regular->table_distance; // + 1;
//         result.extend(coord - radius);
//         result.extend(coord + radius);
//     }
//     for (auto it_less_than = table_distances.begin_less_than_for(point_no), last_less_than = table_distances.end_less_than_for(point_no); it_less_than != last_less_than; ++it_less_than) {
//         const auto coord = original_layout_.get(it_less_than->point_1 == point_no ? it_less_than->point_2 : it_less_than->point_1);
//         const auto radius = it_less_than->table_distance; // + 1;
//         result.extend(coord - radius);
//         result.extend(coord + radius);
//     }
//     return result;

// } // GridTest::area_for

// ----------------------------------------------------------------------

acmacs::Area GridTest::area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const
{
    acmacs::Area result(original_layout_.get(table_distances_for_point.regular.empty() ? table_distances_for_point.less_than.front().another_point : table_distances_for_point.regular.front().another_point));
    auto extend = [&result,this](const auto& entry) {
        const auto coord = this->original_layout_.get(entry.another_point);
        const auto radius = entry.table_distance; // + 1;
        result.extend(coord - radius);
        result.extend(coord + radius);
    };
    std::for_each(table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), extend);
    std::for_each(table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), extend);
    return result;

} // GridTest::area_for

// ----------------------------------------------------------------------

GridTest::Result GridTest::test_point(size_t point_no)
{
    constexpr auto optimization_method = acmacs::chart::optimization_method::alglib_cg_pca;

    acmacs::Layout layout(original_layout_);
    const auto table_distances_for_point = stress_.table_distances_for(point_no);
    const auto target_contribution = stress_.contribution(point_no, table_distances_for_point, layout.data());
    const auto original_pos = original_layout_.get(point_no);
    auto best_contribution = target_contribution;
    Coordinates best_coord, hemisphering_coord;
    const auto hemisphering_stress_threshold_rough = hemisphering_stress_threshold_ * 2;
    auto hemisphering_contribution = target_contribution + hemisphering_stress_threshold_rough;
    const auto area = area_for(table_distances_for_point);
    for (auto it = area.begin(grid_step_), last = area.end(); it != last; ++it) {
        layout.set(point_no, *it);
        const auto contribution = stress_.contribution(point_no, table_distances_for_point, layout.data());
        if (contribution < best_contribution) {
            best_contribution = contribution;
            best_coord = *it;
        }
        else if (best_coord.empty() && contribution < hemisphering_contribution && original_pos.distance(*it) > hemisphering_distance_threshold_) {
            hemisphering_contribution = contribution;
            hemisphering_coord = *it;
        }
    }
    if (!best_coord.empty()) {
        layout.set(point_no, best_coord);
        acmacs::chart::optimize(optimization_method, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
        const auto pos = layout.get(point_no);
        const auto diff = stress_.contribution(point_no, table_distances_for_point, layout.data()) - target_contribution;
        return Result(point_no, std::abs(diff) > hemisphering_stress_threshold_ ? Result::trapped : Result::hemisphering, pos, original_pos.distance(pos), diff);
    }
    else if (!hemisphering_coord.empty()) {
        // relax to find real contribution
        layout.set(point_no, hemisphering_coord);
        acmacs::chart::optimize(optimization_method, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
        auto pos = layout.get(point_no);
        auto real_distance = original_pos.distance(pos);
        if (real_distance > hemisphering_distance_threshold_ && real_distance < (hemisphering_distance_threshold_ * 1.2)) {
            acmacs::chart::optimize(optimization_method, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::fine);
            pos = layout.get(point_no);
            real_distance = original_pos.distance(pos);
        }
        if (real_distance > hemisphering_distance_threshold_) {
            if (const auto real_contribution_diff = stress_.contribution(point_no, table_distances_for_point, layout.data()) - target_contribution; real_contribution_diff < hemisphering_stress_threshold_) {
                return Result(point_no, Result::hemisphering, pos, real_distance, real_contribution_diff);
            }
        }
    }
    return Result(point_no);

} // GridTest::test_point

// ----------------------------------------------------------------------

void GridTest::test_all()
{
    const auto unmovable = projection_->unmovable(), disconnected = projection_->disconnected();
    for (auto point_no : acmacs::range(chart_.number_of_points())) {
        if (!unmovable.contains(point_no) && !disconnected.contains(point_no)) {
            if (const auto result = test_point(point_no); result)
                std::cout << result.report() << '\n';
        }
    }

} // GridTest::test_all

// ----------------------------------------------------------------------

std::string GridTest::Result::report() const
{
    std::string diag;
    switch (diagnosis) {
      case normal:
          diag = "norm";
          break;
      case trapped:
          diag = "trap";
          break;
      case hemisphering:
          diag = "hemi";
          break;
    }
    return diag + ' ' + acmacs::to_string(point_no) + " diff:" + acmacs::to_string(contribution_diff, 4) + " dist:" + acmacs::to_string(distance, 4);

} // GridTest::Result::report

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--step", 0.1, "grid step"},
                {"--verbose", false},
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 2 || args.number_of_arguments() > 3) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <point-no> [<output-chart>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            const size_t projection_no = 0;
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};

            GridTest test(chart, projection_no, args["--step"]);
            // std::cout << test.initial_report() << '\n';
            if (args[1] == std::string("all")) {
                test.test_all();
            }
            else {
                if (const auto result = test.test_point(std::stoul(args[1])); result)
                    std::cout << result.report() << '\n';
            }

            // if (point_no >= chart.number_of_points())
            //     throw std::runtime_error("invalid point number");
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
