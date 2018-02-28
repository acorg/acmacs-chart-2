#include <iostream>
#include <algorithm>

#include "acmacs-base/argc-argv.hh"
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

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--projection", 0},
                {"--points", "", "comma separated list of point indexes, no spaces!"},
                {"--antigens", "", "comma separated list of antigen indexes, no spaces!"},
                {"--sera", "", "comma separated list of serum indexes, no spaces!"},
                {"--move-to", "", "comma separated list of coordinates, no spaces!" },
                {"--rotate-degrees", 0.0},
                {"--rotate-radians", 0.0},
                {"--flip-ew", false},
                {"--flip-ns", false},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <input-chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            std::cout << chart->make_info() << '\n';
            const size_t projection_no = args["--projection"];
            if (projection_no >= chart->number_of_projections())
                throw std::runtime_error("Invalid projection: " + acmacs::to_string(projection_no) + ", expected in range 0.." + acmacs::to_string(chart->number_of_projections() - 1) + " inclusive");

            std::vector<size_t> points;
            if (args["--points"].present())
                extend(points, acmacs::string::split_into_uint(static_cast<std::string_view>(args["--points"]), ","), 0, chart->number_of_points());
            if (args["--antigens"].present())
                extend(points, acmacs::string::split_into_uint(static_cast<std::string_view>(args["--antigens"]), ","), 0, chart->number_of_antigens());
            if (args["--sera"].present())
                extend(points, acmacs::string::split_into_uint(static_cast<std::string_view>(args["--sera"]), ","), chart->number_of_antigens(), chart->number_of_sera());
            std::sort(points.begin(), points.end());
            points.erase(std::unique(points.begin(), points.end()), points.end());
            if (!points.empty() && points.back() >= chart->number_of_points())
                throw std::runtime_error("Invalid point index: " + acmacs::to_string(points.back()) + ", expected in range 0.." + acmacs::to_string(chart->number_of_points() - 1) + " inclusive");

            acmacs::chart::ChartModify chart_modify(chart);
            auto projection = chart_modify.projection_modify(projection_no);
            if (args["--move-to"].present()) {
                if (points.empty())
                    throw std::runtime_error("--move-to requires at least one of --antigens, --sera, --points");
                const auto target_coordinates = acmacs::string::split_into_double(std::string_view(args["--move-to"]), ",");
                for (auto point_no: points)
                    projection->move_point(point_no, target_coordinates);
            }
            if (const double rotate_degrees = args["--rotate-degrees"]; !float_zero(rotate_degrees))
                projection->rotate_degrees(rotate_degrees);
            if (const double rotate_radians = args["--rotate-radians"]; !float_zero(rotate_radians))
                projection->rotate_radians(rotate_radians);
            if (args["--flip-ew"])
                projection->flip_east_west();
            if (args["--flip-ns"])
                projection->flip_north_south();

            acmacs::chart::export_factory(chart_modify, args[1], fs::path(args.program()).filename(), args["--time"] ? report_time::Yes : report_time::No);
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
