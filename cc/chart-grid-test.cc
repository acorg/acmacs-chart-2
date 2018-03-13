#include <iostream>
#include <algorithm>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
// #include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"

static void initial_info(acmacs::chart::ChartModify& chart, size_t projection_no, size_t point_no);
static void test(acmacs::chart::ChartModify& chart, size_t projection_no, size_t point_no, double step);

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
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <point-no>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const size_t point_no = std::stoul(args[1]);
            if (point_no >= chart.number_of_points())
                throw std::runtime_error("invalid point number");
            const size_t projection_no = 0;
            const double step = 0.1;
            initial_info(chart, projection_no, point_no);
            test(chart, projection_no, point_no, step);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void initial_info(acmacs::chart::ChartModify& chart, size_t projection_no, size_t point_no)
{
    auto projection = chart.projection_modify(projection_no);
    auto layout = projection->layout();

    const auto [top_left, bottom_right] = layout->boundaries();
    std::cout << "Boundaries: " << top_left << ' ' << bottom_right << '\n';

    if (point_no < chart.number_of_antigens())
        std::cout << "AG " << point_no << ' ' << (*chart.antigens())[point_no]->full_name() << '\n';
    else
        std::cout << "SR " << point_no - chart.number_of_antigens() << ' ' << (*chart.sera())[point_no - chart.number_of_antigens()]->full_name() << '\n';
    std::cout << "Initial: " << layout->get(point_no) << ' ' << projection->stress() << '\n';

} // initial_info

// ----------------------------------------------------------------------

struct Entry
{
    acmacs::Coordinates coord;
    double stress;

    Entry(const acmacs::Coordinates& a_coord, double a_stress) : coord(a_coord), stress(a_stress) {}
    bool operator<(const Entry& rhs) const { return stress < rhs.stress; }
};

void test(acmacs::chart::ChartModify& chart, size_t projection_no, size_t point_no, double step)
{
    auto projection = chart.projection_modify(projection_no);
    auto stress = chart.make_stress<double>(projection_no);
    const auto [top_left, bottom_right] = projection->layout()->boundaries();

    std::vector<Entry> entries;
    for (double x = top_left[0]; x < bottom_right[0]; x += step) {
        for (double y = top_left[1]; y < bottom_right[1]; y += step) {
            const acmacs::Coordinates coord{x, y};
            projection->move_point(point_no, coord);
            entries.emplace_back(coord, projection->calculate_stress(stress));
            // std::cout << coord << ' ' << projection->calculate_stress(stress) << '\n';
        }
    }
    // std::sort(entries.begin(), entries.end());
    // for (auto i : acmacs::range(10UL))
    //     std::cout << std::setw(2) << i << ' ' << entries[i].coord << ' ' << entries[i].stress << '\n';

    const auto& best = *std::min_element(entries.begin(), entries.end());
    projection->move_point(point_no, best.coord);
    std::cout << "Best: " << best.coord << ' ' << projection->calculate_stress(stress) << '\n';
    const auto status = projection->relax(acmacs::chart::optimization_options{});
    std::cout << "Final: " << projection->layout()->get(point_no) << ' ' << status.final_stress << '\n';

} // test

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
