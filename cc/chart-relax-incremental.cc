#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-n", 1, "number of optimizations"},
                {"--rough", false},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 2.0, "randomization diameter multiplier"},
                {"--keep-projections", 0, "number of projections to keep, 0 - keep all"},
                {"--no-disconnect-having-few-titers", false, "do not disconnect points having too few numeric titers"},
                {"--threads", 0, "number of threads to use for optimization (omp): 0 - autodetect, 1 - sequential"},
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
            const auto report = do_report_time(args["--time"]);
            const size_t number_of_attempts = args["-n"];
            const Timeit ti("performing " + std::to_string(number_of_attempts) + " optimizations: ", report);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            const auto method{acmacs::chart::optimization_method_from_string(args["--method"])};

            const size_t source_projection_no = 0;
            acmacs::chart::optimization_options options(method, precision, args["--md"]);
            options.num_threads = args["--threads"];
            chart.relax_incremental(source_projection_no, acmacs::chart::number_of_optimizations_t{number_of_attempts}, options, args["--no-disconnect-having-few-titers"] ? acmacs::chart::disconnect_having_too_few_titers::no : acmacs::chart::disconnect_having_too_few_titers::yes);
            auto projections = chart.projections_modify();
            if (const size_t keep_projections = args["--keep-projections"]; keep_projections > 0 && projections->size() > keep_projections)
                projections->keep_just(keep_projections);
            std::cout << chart.make_info() << '\n';
            if (args.number_of_arguments() > 1)
                acmacs::chart::export_factory(chart, args[1], args.program(), report);
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
