#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-n", 1U, "number of optimizations"},
                {"--projection", 0},
                {"--rough", false},
                {"--remove-source-projections", false},
                {"--keep-projections", 0, "number of projections to keep, 0 - keep all"},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 1.0, "max distance multiplier"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Randomizes position for disconnected points, connects them and relaxes the map.\n"
                      << "Usage: " << args.program() << " [options] <chart-file> [<output-chart-file>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            const acmacs::chart::optimization_method method{acmacs::chart::optimization_method_from_string(args["--method"])};

            acmacs::chart::ProjectionModifyP source_projection;
            if (args["--remove-source-projections"]) {
                chart.projections_modify()->remove_all_except(args["--projection"]);
                source_projection = chart.projection_modify(0);
            }
            else {
                source_projection = chart.projection_modify(args["--projection"]);
            }
            auto disconnected = source_projection->disconnected();
            if (disconnected.empty())
                throw std::runtime_error("projection has no disconnected points");
            std::cout << "INFO: disconnected points: (" << disconnected.size() << ')' << disconnected << '\n';

            const size_t number_of_attempts = args["-n"];
            for (size_t attempt = 1; attempt <= number_of_attempts; ++attempt) {
                auto projection = source_projection->clone(chart);
                projection->connect(disconnected);
                projection->randomize_layout(disconnected);
                const auto status = projection->relax(acmacs::chart::optimization_options(method, precision));
                std::cout << attempt << ' ' << status << '\n';
            }
            if (args["--remove-source-projections"])
                chart.projections_modify()->remove(0);


            chart.projections_modify()->sort();
            std::cout << chart.make_info() << '\n';
            if (args.number_of_arguments() > 1)
                acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename(), report);
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
