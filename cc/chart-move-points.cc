#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 4) {
            std::cerr << "Usage: " << args.program() << " [options] <comma-sep-point-numbers> <comma-sep-target-coords> <input-chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            size_t projection_no = 0;
            auto chart = acmacs::chart::import_factory(args[2], acmacs::chart::Verify::None);
            std::cout << chart->make_info() << '\n';
            acmacs::chart::ChartModify chart_modify(chart);
            const auto points = acmacs::string::split_into_uint(std::string_view(args[0]), ",");
            const auto target_coordinates = acmacs::string::split_into_double(std::string_view(args[1]), ",");
            auto projection = chart_modify.projection_modify(projection_no);
            for (auto point_no: points)
                projection->move_point(point_no, target_coordinates);
            acmacs::chart::export_factory(chart_modify, args[3], fs::path(args.program()).filename());
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