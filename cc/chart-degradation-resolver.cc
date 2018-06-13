#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/serum-line.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-n", 1U, "number of optimizations"},
                {"--keep-projections", 10, "number of projections to keep, 0 - keep all"},
                {"--no-disconnect-having-few-titers", false, "do not disconnect points having too few numeric titers"},
                {"--serum-line-sd-threshold", 0.4, "do not run resolver if serum line sd higher than this threshold"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const size_t projection_no = 0;
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};

            auto projection = chart.projection(projection_no);
            acmacs::chart::SerumLine serum_line(*projection);
            std::cerr << serum_line << '\n';
            if (serum_line.standard_deviation() > static_cast<double>(args["--serum-line-sd-threshold"]))
                throw std::runtime_error("serum line sd " + std::to_string(serum_line.standard_deviation()) + " > " + acmacs::to_string(args["--serum-line-sd-threshold"]));

            const auto antigens_relative_to_line = serum_line.antigens_relative_to_line(*projection);
            std::cerr << "antigens_relative_to_line: neg:" << antigens_relative_to_line.negative.size() << " pos:" << antigens_relative_to_line.positive.size() << '\n';

            // const size_t number_of_attempts = args["-n"];
            // const Timeit ti("performing " + std::to_string(number_of_attempts) + " optimizations: ", report);
            // const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            // const auto method{acmacs::chart::optimization_method_from_string(args["--method"])};
            // auto disconnected{get_disconnected(args["--disconnect-antigens"], args["--disconnect-sera"], chart.number_of_antigens(), chart.number_of_sera())};
            // if (!args["--no-disconnect-having-few-titers"])
            //     disconnected.extend(chart.titers()->having_too_few_numeric_titers());

            // chart.relax(number_of_attempts, args["-m"].str(), args["-d"], !args["--no-dimension-annealing"], acmacs::chart::optimization_options(method, precision, args["--md"]), args["--verbose"] || args["-v"], disconnected);
            // // for (size_t attempt = 0; attempt < number_of_attempts; ++attempt) {
            // //     auto [status, projection] = chart.relax(args["-m"].str(), args["-d"], !args["--no-dimension-annealing"], acmacs::chart::optimization_options(method, precision, args["--md"]), disconnected);
            // //     std::cout << (attempt + 1) << ' ' << status << '\n';
            // // }
            // auto projections = chart.projections_modify();
            // projections->sort();
            // if (const size_t keep_projections = args["--keep-projections"]; keep_projections > 0 && projections->size() > keep_projections)
            //     projections->keep_just(keep_projections);
            // std::cout << chart.make_info() << '\n';
            // if (args.number_of_arguments() > 1)
            //     acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename(), report);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
