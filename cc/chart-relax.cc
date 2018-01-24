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
                {"-d", 2U, "number of dimensions"},
                {"-m", "none", "minimum column basis"},
                {"--rough", false},
                {"--no-dimension-annealing", false},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 1.0, "max distance multiplier"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> [<output-chart-file>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            acmacs::chart::optimization_method method{acmacs::chart::optimization_method::alglib_cg_pca};
            if (args["--method"] == "lbfgs")
                method = acmacs::chart::optimization_method::alglib_lbfgs_pca;
            else if (args["--method"] == "cg")
                method = acmacs::chart::optimization_method::alglib_cg_pca;
            else
                throw std::runtime_error("unrecognized method, lbfgs or cg expected");

            const size_t number_of_attempts = args["-n"];
            for (size_t attempt = 0; attempt < number_of_attempts; ++attempt) {
                auto [status, projection] = chart.relax(args["-m"].str(), args["-d"], !args["--no-dimension-annealing"], acmacs::chart::optimization_options(method, precision, args["--md"]));
                std::cout << status << '\n';
            }
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
