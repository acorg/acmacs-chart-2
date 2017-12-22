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
                {"--points", "", "comma separated list of point indexes, no spaces!"},
                {"--antigens", "", "comma separated list of antigen indexes, no spaces!"},
                {"--sera", "", "comma separated list of serum indexes, no spaces!"},

                {"--size", 0.0, "change point size"},
                {"--fill", "", "change point fill color"},
                {"--outline", "", "change point outline color"},

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
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, args["--time"] ? report_time::Yes : report_time::No);
            std::cout << chart->make_info() << '\n';

            std::vector<size_t> points;
            if (args["--points"])
                extend(points, acmacs::string::split_into_uint(static_cast<std::string_view>(args["--points"]), ","), 0, chart->number_of_points());
            if (args["--antigens"])
                extend(points, acmacs::string::split_into_uint(static_cast<std::string_view>(args["--antigens"]), ","), 0, chart->number_of_antigens());
            if (args["--sera"])
                extend(points, acmacs::string::split_into_uint(static_cast<std::string_view>(args["--sera"]), ","), chart->number_of_antigens(), chart->number_of_sera());
            std::sort(points.begin(), points.end());
            points.erase(std::unique(points.begin(), points.end()), points.end());
            if (points.empty())
                throw std::runtime_error("No point indexes were provided, use --points, --antigens, --sera");
            // else if (!points.empty() && points.back() >= chart->number_of_points())
            //     throw std::runtime_error("Invalid point index: " + acmacs::to_string(points.back()) + ", expected in range 0.." + acmacs::to_string(chart->number_of_points() - 1) + " inclusive");
            std::cerr << "points: " << points << '\n';

            acmacs::chart::ChartModify chart_modify(chart);
            auto plot_spec = chart_modify.plot_spec_modify();

            if (args["--size"].present()) {
                for (auto point_no: points)
                    plot_spec->size(point_no, Pixels{args["--size"]});
            }
            if (args["--fill"].present()) {
                for (auto point_no: points)
                    plot_spec->fill(point_no, static_cast<std::string_view>(args["--fill"]));
            }
            if (args["--outline"].present()) {
                for (auto point_no: points)
                    plot_spec->outline(point_no, static_cast<std::string_view>(args["--outline"]));
            }

            acmacs::chart::export_factory(chart_modify, args[1], fs::path(args.program()).filename());
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
