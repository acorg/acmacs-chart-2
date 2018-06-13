#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
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
            fs::path output_filename(args[1]);
            auto intermediate_filename = [&output_filename](size_t step) { fs::path fn{output_filename}; return fn.replace_extension(".i" + std::to_string(step) + ".ace"); };

            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};

            auto original_projection = chart.projection_modify(projection_no);
            acmacs::chart::SerumLine serum_line(*original_projection);
            std::cerr << serum_line << '\n';
            if (serum_line.standard_deviation() > static_cast<double>(args["--serum-line-sd-threshold"]))
                throw std::runtime_error("serum line sd " + std::to_string(serum_line.standard_deviation()) + " > " + acmacs::to_string(args["--serum-line-sd-threshold"]));

            const auto antigens_relative_to_line = serum_line.antigens_relative_to_line(*original_projection);
            const bool good_side_positive = antigens_relative_to_line.negative.size() < antigens_relative_to_line.positive.size();
            const auto antigens_to_flip = good_side_positive ? antigens_relative_to_line.negative.size() : antigens_relative_to_line.positive.size();
            std::cerr << "antigens_relative_to_line: neg:" << antigens_relative_to_line.negative.size() << " pos:" << antigens_relative_to_line.positive.size() << '\n';

              // flip bad side antigens to good side
            auto flipped = chart.projections_modify()->new_by_cloning(*original_projection);
            flipped->comment((good_side_positive ? "negative " : "positive ") + std::to_string(antigens_to_flip) + " antigens flipped");
            auto layout = flipped->layout();
            for (auto index : (good_side_positive ? antigens_relative_to_line.negative : antigens_relative_to_line.positive))
                flipped->move_point(index, serum_line.line().flip_over(layout->get(index), 1.0));
            acmacs::chart::export_factory(chart, intermediate_filename(1), fs::path(args.program()).filename(), report);

              // relax from flipped
            auto relax_from_flipped = chart.projections_modify()->new_by_cloning(*flipped);
            relax_from_flipped->comment("flipped relaxed");
            relax_from_flipped->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_precision::rough));
            acmacs::chart::export_factory(chart, intermediate_filename(2), fs::path(args.program()).filename(), report);

            std::cout << chart.make_info() << '\n';

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
