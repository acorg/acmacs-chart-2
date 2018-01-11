#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
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
            const size_t attempts = args["-n"];
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            for (size_t no = 0; no < attempts; ++no) {
                  // Timeit ti("randomize and relax: ");
                auto projection = chart.projections_modify()->new_from_scratch(args["-d"], args["-m"].str());
                projection->randomize_layout(args["--md"]);
                const auto status = projection->relax(acmacs::chart::OptimizationMethod::alglib_lbfgs);
                if (attempts == 1)
                    std::cout << status << '\n';
                else
                    std::cout << no << ' ' << std::setprecision(4) << status.final_stress << " time: " << acmacs::format(status.time) << '\n';
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
