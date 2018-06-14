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
            const auto& antigens_to_flip = good_side_positive ? antigens_relative_to_line.negative : antigens_relative_to_line.positive;
            std::cerr << "antigens_relative_to_line: neg:" << antigens_relative_to_line.negative.size() << " pos:" << antigens_relative_to_line.positive.size() << '\n';

              // mark bad side antigens
            acmacs::PointStyle style;
            style.outline = "orange";
            style.outline_width = Pixels{2};
            chart.plot_spec_modify()->modify(antigens_to_flip, style);

              // flip bad side antigens to good side
            auto flipped = chart.projections_modify()->new_by_cloning(*original_projection);
            flipped->comment((good_side_positive ? "negative " : "positive ") + std::to_string(antigens_to_flip.size()) + " antigens flipped");
            auto layout = flipped->layout();
            for (auto index : antigens_to_flip)
                flipped->move_point(index, serum_line.line().flip_over(layout->get(index), 1.0));
              //acmacs::chart::export_factory(chart, intermediate_filename(1), fs::path(args.program()).filename(), report);

              // relax from flipped
            auto relax_from_flipped = chart.projections_modify()->new_by_cloning(*flipped);
            relax_from_flipped->comment("flipped relaxed");
            relax_from_flipped->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_precision::rough));
              //acmacs::chart::export_factory(chart, intermediate_filename(2), fs::path(args.program()).filename(), report);

            auto randomized = chart.projections_modify()->new_by_cloning(*original_projection);
            randomized->comment((good_side_positive ? "negative " : "positive ") + std::to_string(antigens_to_flip.size()) + " antigens randomized in the good side");
            // auto stress = acmacs::chart::stress_factory<double>(*randomized);
            // auto randomizer = acmacs::chart::randomizer_plain_from_sample_optimization(*randomized, stress, 1.0);
            auto randomizer = acmacs::chart::randomizer_plain_with_current_layout_area(*randomized, 1.0);
            auto layout_randomized = randomized->randomize_layout(antigens_to_flip, randomizer);
            const auto antigens_relative_to_line_randomized = serum_line.antigens_relative_to_line(*randomized);
            for (auto index : (good_side_positive ? antigens_relative_to_line_randomized.negative : antigens_relative_to_line_randomized.positive))
                randomized->move_point(index, serum_line.line().flip_over(layout_randomized->get(index), 1.0));

            auto randomized_relaxed = chart.projections_modify()->new_by_cloning(*randomized);
            randomized_relaxed->comment("randomized relaxed");
            randomized_relaxed->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_precision::rough));
            acmacs::chart::export_factory(chart, intermediate_filename(3), fs::path(args.program()).filename(), report);

            std::cout << chart.make_info() << '\n';
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
