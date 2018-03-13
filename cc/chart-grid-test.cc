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

    GridTest(Chart& chart, size_t projection_no)
        : chart_(chart), projection_(chart.projection_modify(projection_no)), boundaries_(projection_->layout()->boundaries()),
          original_layout_(*projection_->layout()), stress_(chart.make_stress<double>(projection_no)) {}

    std::string initial_report() const;
    std::string point_name(size_t point_no) const;
    void test_point(size_t point_no);
    void test_all();

 private:
    Chart& chart_;
    Projection projection_;
    const acmacs::Boundaries boundaries_;
    const double grid_step_ = 0.1;          // acmacs-c2: 0.01
    const double distance_threshold_ = 1.0; // from acmacs-c2 hemi-local test
    const double stress_threshold_ = 0.25;  // stress diff within threshold -> hemisphering, from acmacs-c2 hemi-local test
    const acmacs::Layout original_layout_;
    const Stress stress_;

    constexpr bool antigen(size_t point_no) const { return point_no < chart_.number_of_antigens(); }
    constexpr size_t antigen_serum_no(size_t point_no) const { return antigen(point_no) ? point_no : (point_no - chart_.number_of_antigens()); }

}; // class GridTest

// ----------------------------------------------------------------------

std::string GridTest::initial_report() const
{
    return "Boundaries: " + acmacs::to_string(boundaries_, 4);

} // GridTest::initial_report

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

void GridTest::test_point(size_t point_no)
{
    acmacs::Layout layout(original_layout_);
    const auto having_titers_with = chart_.titers()->having_titers_with(point_no);
    acmacs::Boundaries boundaries = original_layout_.boundaries(having_titers_with.data());

    Coordinates best_coord; //  = layout.get(point_no);
    auto best_stress = projection_->stress() * 100.0;
    for (double x = boundaries_.min[0]; x < boundaries_.max[0]; x += grid_step_) {
        for (double y = boundaries_.min[1]; y < boundaries_.max[1]; y += grid_step_) {
            const Coordinates coord{x, y};
            layout.set(point_no, coord);
            const auto stress = stress_.value(layout);
            if (stress < best_stress) {
                best_stress = stress;
                best_coord = coord;
            }
            // projection->move_point(point_no, coord);
            // entries.emplace_back(coord, projection->calculate_stress(stress));
            // std::cout << coord << ' ' << projection->calculate_stress(stress) << '\n';
        }
    }
    if (const auto distance = best_coord.distance(original_layout_.get(point_no)); best_stress < projection_->stress() || distance > distance_threshold_) {
        std::cout << point_name(point_no) << " stress-diff: " << (best_stress - projection_->stress()) << " distance: " << distance << '\n';
    }

} // GridTest::test_point

// ----------------------------------------------------------------------

void GridTest::test_all()
{
    const auto unmovable = projection_->unmovable(), disconnected = projection_->disconnected();
    for (auto point_no : acmacs::range(chart_.number_of_points())) {
        if (!unmovable.contains(point_no) && !disconnected.contains(point_no)) {
            test_point(point_no);
        }
    }

} // GridTest::test_all

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
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

            GridTest test(chart, projection_no);
            std::cout << test.initial_report() << '\n';
            if (args[1] == std::string("all")) {
                test.test_all();
            }
            else {
                test.test_point(std::stoul(args[1]));
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

// struct Entry
// {
//     acmacs::Coordinates coord;
//     double stress;

//     Entry(const acmacs::Coordinates& a_coord, double a_stress) : coord(a_coord), stress(a_stress) {}
//     bool operator<(const Entry& rhs) const { return stress < rhs.stress; }
// };

// void test_point(acmacs::chart::ChartModify& chart, size_t projection_no, size_t point_no, double step)
// {
//     auto projection = chart.projection_modify(projection_no);
//     auto layout = projection->layout();
//     const acmacs::Layout saved_layout(*layout);
//     std::cout << point_name(chart, point_no) << ' ' << layout->get(point_no) << ' ' << std::setprecision(6) << std::fixed << projection->stress() << '\n';

//     const auto [top_left, bottom_right] = layout->boundaries();
//     auto stress = chart.make_stress<double>(projection_no);

//     std::vector<Entry> entries;
//     for (double x = top_left[0]; x < bottom_right[0]; x += step) {
//         for (double y = top_left[1]; y < bottom_right[1]; y += step) {
//             const acmacs::Coordinates coord{x, y};
//             projection->move_point(point_no, coord);
//             entries.emplace_back(coord, projection->calculate_stress(stress));
//             // std::cout << coord << ' ' << projection->calculate_stress(stress) << '\n';
//         }
//     }
//     std::sort(entries.begin(), entries.end());

//     for (auto i : acmacs::range(10UL)) {
//         projection->set_layout(saved_layout);
//         const auto& entry = entries[i];
//         projection->move_point(point_no, entry.coord);
//         std::cout << std::setw(2) << i << ' ' << entry.coord << ' ' << std::setprecision(6) << std::fixed << projection->calculate_stress(stress);
//         const auto status = projection->relax(acmacs::chart::optimization_options{});
//         std::cout << " --> " << projection->layout()->get(point_no) << ' ' << std::setprecision(6) << status.final_stress << '\n';
//     }

// } // test_point

// void test_point_old(acmacs::chart::ChartModify& chart, size_t projection_no, size_t point_no, double step)
// {
//     auto projection = chart.projection_modify(projection_no);
//     auto layout = projection->layout();
//     const acmacs::Layout saved_layout(*layout);
//     std::cout << point_name(chart, point_no) << ' ' << layout->get(point_no) << ' ' << std::setprecision(6) << std::fixed << projection->stress() << '\n';

//     const auto [top_left, bottom_right] = layout->boundaries();
//     auto stress = chart.make_stress<double>(projection_no);

//     std::vector<Entry> entries;
//     for (double x = top_left[0]; x < bottom_right[0]; x += step) {
//         for (double y = top_left[1]; y < bottom_right[1]; y += step) {
//             const acmacs::Coordinates coord{x, y};
//             projection->move_point(point_no, coord);
//             entries.emplace_back(coord, projection->calculate_stress(stress));
//             // std::cout << coord << ' ' << projection->calculate_stress(stress) << '\n';
//         }
//     }
//     std::sort(entries.begin(), entries.end());

//     for (auto i : acmacs::range(10UL)) {
//         projection->set_layout(saved_layout);
//         const auto& entry = entries[i];
//         projection->move_point(point_no, entry.coord);
//         std::cout << std::setw(2) << i << ' ' << entry.coord << ' ' << std::setprecision(6) << std::fixed << projection->calculate_stress(stress);
//         const auto status = projection->relax(acmacs::chart::optimization_options{});
//         std::cout << " --> " << projection->layout()->get(point_no) << ' ' << std::setprecision(6) << status.final_stress << '\n';
//     }

// } // test_point_old

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
