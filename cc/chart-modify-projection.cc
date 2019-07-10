#include <iostream>
#include <algorithm>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

inline void extend(std::vector<size_t>& target, std::vector<size_t>&& source, size_t aIncrementEach, size_t aMax)
{
    std::transform(source.begin(), source.end(), std::back_inserter(target), [aIncrementEach,aMax](auto v) {
        if (v >= aMax)
            throw std::runtime_error("invalid index " + acmacs::to_string(v) + ", expected in range 0.." + acmacs::to_string(aMax - 1) + " inclusive");
        return v + aIncrementEach;
    });
}

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> projection{*this, "projection", dflt{0UL}};
    option<str>    points{*this, "points", dflt{""}, desc{"comma separated list of point indexes, no spaces!"}};
    option<str>    antigens{*this, "antigens", dflt{""}, desc{"comma separated list of antigen indexes, no spaces!"}};
    option<str>    sera{*this, "sera", dflt{""}, desc{"comma separated list of serum indexes, no spaces!"}};
    option<str>    move_to{*this, "move-to", dflt{""}, desc{"comma separated list of coordinates, no spaces!"}};
    option<double> rotate_degrees{*this, "rotate-degrees", dflt{0.0}};
    option<double> rotate_radians{*this, "rotate-radians", dflt{0.0}};
    option<bool>   flip_ew{*this, "flip-ew"};
    option<bool>   flip_ns{*this, "flip-ns"};

    argument<str> input_chart{*this, arg_name{"input-chart-file"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        std::cout << chart->make_info() << '\n';
        const size_t projection_no = opt.projection;
        if (projection_no >= chart->number_of_projections())
            throw std::runtime_error("Invalid projection: " + acmacs::to_string(projection_no) + ", expected in range 0.." + acmacs::to_string(chart->number_of_projections() - 1) + " inclusive");

        std::vector<size_t> points;
        if (opt.points.has_value())
            extend(points, acmacs::string::split_into_size_t(*opt.points, ","), 0, chart->number_of_points());
        if (opt.antigens.has_value())
            extend(points, acmacs::string::split_into_size_t(*opt.antigens, ","), 0, chart->number_of_antigens());
        if (opt.sera.has_value())
            extend(points, acmacs::string::split_into_size_t(*opt.sera, ","), chart->number_of_antigens(), chart->number_of_sera());
        std::sort(points.begin(), points.end());
        points.erase(std::unique(points.begin(), points.end()), points.end());
        if (!points.empty() && points.back() >= chart->number_of_points())
            throw std::runtime_error("Invalid point index: " + acmacs::to_string(points.back()) + ", expected in range 0.." + acmacs::to_string(chart->number_of_points() - 1) + " inclusive");

        acmacs::chart::ChartModify chart_modify(chart);
        auto projection = chart_modify.projection_modify(projection_no);
        if (opt.move_to.has_value()) {
            if (points.empty())
                throw std::runtime_error("--move-to requires at least one of --antigens, --sera, --points");
            const auto target_coordinates_v = acmacs::string::split_into_double(*opt.move_to, ",");
            const acmacs::PointCoordinates target_coordinates(target_coordinates_v[0], target_coordinates_v[1]);
            for (auto point_no : points)
                projection->move_point(point_no, target_coordinates);
        }
        if (const double rotate_degrees = opt.rotate_degrees; !float_zero(rotate_degrees))
            projection->rotate_degrees(rotate_degrees);
        if (const double rotate_radians = opt.rotate_radians; !float_zero(rotate_radians))
            projection->rotate_radians(rotate_radians);
        if (opt.flip_ew)
            projection->flip_east_west();
        if (opt.flip_ns)
            projection->flip_north_south();

        acmacs::chart::export_factory(chart_modify, opt.output_chart, fs::path(opt.program_name()).filename());
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
