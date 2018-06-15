#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/randomizer.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/serum-line.hh"

// ----------------------------------------------------------------------

struct Options
{
    Options(size_t a_number_of_attempts, double a_serum_line_sd_threshold, double a_rms_threshold)
        : number_of_attempts{a_number_of_attempts}, serum_line_sd_threshold{a_serum_line_sd_threshold}, rms_threshold{a_rms_threshold} {}

    size_t number_of_attempts;
    double serum_line_sd_threshold;
    double rms_threshold;
    size_t max_levels_in_randomization_descent = 20;
};

struct SplitData
{
    SplitData(const acmacs::chart::Projection& projection)
        : serum_line(projection)
        {
            auto antigens_relative_to_line = serum_line.antigens_relative_to_line(projection);
            if (antigens_relative_to_line.negative.size() < antigens_relative_to_line.positive.size()) {
                good_side = acmacs::LineSide::side::positive;
                on_the_wrong_side = std::move(antigens_relative_to_line.negative);
            }
            else {
                good_side = acmacs::LineSide::side::negative;
                on_the_wrong_side = std::move(antigens_relative_to_line.positive);
            }
        }

    acmacs::chart::SerumLine serum_line;
    acmacs::LineSide::side good_side;
    acmacs::chart::PointIndexList on_the_wrong_side;
};

static acmacs::chart::ProjectionModifyP flip_relax(acmacs::chart::ChartModify& chart, acmacs::chart::ProjectionModifyP original_projection, const Options& options);
static acmacs::chart::ProjectionModifyP randomize_found_on_the_wrong_side_of_serum_line(acmacs::chart::ChartModify& chart, acmacs::chart::ProjectionModifyP original_projection, size_t level, std::string level_path, const Options& options);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"-n", 1U, "number of resolution attempts"},
                           {"--keep-projections", 10, "number of projections to keep, 0 - keep all"},
                           {"--no-disconnect-having-few-titers", false, "do not disconnect points having too few numeric titers"},
                           {"--serum-line-sd-threshold", 0.4, "do not run resolver if serum line sd higher than this threshold"},
                           {"--rms-threshold", 0.1, "run resolver until rms between current and previous map bigger than this"},
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
            const Options options(args["-n"], args["--serum-line-sd-threshold"], args["--rms-threshold"]);
            // const size_t number_of_attempts = args["-n"];
            // const double rms_threshold = args["--rms-threshold"];
            const auto report = do_report_time(args["--time"]);
            fs::path output_filename(args[1]);
            auto intermediate_filename = [&output_filename](size_t step) {
                fs::path fn{output_filename};
                return fn.replace_extension(".i" + std::to_string(step) + ".ace");
            };

            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            auto original_projection = chart.projection_modify(projection_no);

            auto found1 = flip_relax(chart, original_projection, options);
            // std::cerr << found1->make_info() << '\n' << '\n';
            auto found2 = randomize_found_on_the_wrong_side_of_serum_line(chart, original_projection, 0, "", options);
            chart.projections_modify()->add(found2);
            // std::cerr << '\n' << found2->make_info() << '\n' << '\n';

// #if 0
//             for (size_t attempt = 0; attempt < number_of_attempts; ++attempt) {
//                 auto previous_projection = original_projection;
//                 auto final_projection = original_projection;
//                 const auto number_of_projections_before = chart.number_of_projections();
//                 for (size_t step = 0; step < 1000; ++step) {
//                     acmacs::chart::SerumLine serum_line(*previous_projection);
//                     std::cerr << serum_line << '\n';
//                     const auto antigens_relative_to_line = serum_line.antigens_relative_to_line(*previous_projection);
//                     const bool good_side_positive = antigens_relative_to_line.negative.size() < antigens_relative_to_line.positive.size();
//                     const auto& antigens_to_flip = good_side_positive ? antigens_relative_to_line.negative : antigens_relative_to_line.positive;
//                     std::cerr << "attempt: " << (attempt + 1) << " step: " << (step + 1) << " antigens_relative_to_line: neg:" << antigens_relative_to_line.negative.size() << " pos:" << antigens_relative_to_line.positive.size() << '\n';

//                     auto randomized = chart.projections_modify()->new_by_cloning(*previous_projection);
//                     // randomized->comment("step " + std::to_string(step + 1) + ": " + (good_side_positive ? "negative" : "positive") + " " + std::to_string(antigens_to_flip.size()) +
//                     //                     " antigens randomized in the good side");
//                     auto randomizer = acmacs::chart::randomizer_border_with_current_layout_area(
//                         *randomized, 1.0, {serum_line.line(), good_side_positive ? acmacs::LineSide::side::positive : acmacs::LineSide::side::negative});
//                     auto layout_randomized = randomized->randomize_layout(antigens_to_flip, randomizer);

//                     final_projection = randomized; // chart.projections_modify()->new_by_cloning(*randomized); // randomized; //
//                       // final_projection->comment("attempt " + std::to_string(attempt + 1) + "step " + std::to_string(step + 1) + ": " + std::to_string(antigens_to_flip.size()) + " randomized, relaxed");
//                     final_projection->comment("attempt " + std::to_string(attempt + 1) + " step " + std::to_string(step + 1));
//                     final_projection->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_precision::rough));
//                     const auto procrustes_data = final_projection->orient_to(*previous_projection);
//                     std::cerr << "attempt: " << (attempt + 1) << "after step: " << (step + 1) << " stress: " << final_projection->stress() << " rms to previous: " << procrustes_data.rms << '\n';

//                     // acmacs::chart::SerumLine serum_line_2(*final_projection);
//                     // std::cerr << serum_line_2 << '\n';
//                     // const auto antigens_relative_to_line_2 = serum_line.antigens_relative_to_line(*final_projection);
//                     // std::cerr << "after step: " << step << " antigens_relative_to_line: neg:" << antigens_relative_to_line_2.negative.size() << " pos:" <<
//                     // antigens_relative_to_line_2.positive.size() << '\n';

//                     std::cerr << '\n';

//                     if (procrustes_data.rms < rms_threshold)
//                         break;
//                     previous_projection = final_projection;
//                 }
//                 chart.projections_modify()->remove_except(number_of_projections_before, final_projection);
//             }
// #endif

            chart.projections_modify()->sort();
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

acmacs::chart::ProjectionModifyP randomize_found_on_the_wrong_side_of_serum_line(acmacs::chart::ChartModify& chart, acmacs::chart::ProjectionModifyP original_projection, size_t level, std::string level_path, const Options& options)
{
    std::vector<acmacs::chart::ProjectionModifyP> results(options.number_of_attempts);
    for (size_t attempt = 0; attempt < options.number_of_attempts; ++attempt) {
        const std::string sublevel_path = level_path + (level_path.empty() ? "" : "-") + acmacs::to_string(attempt + 1);
          // std::cerr << sublevel_path << '\n';

        SplitData split_data(*original_projection);
        acmacs::chart::ProjectionModifyP new_projection = chart.projections_modify()->new_by_cloning(*original_projection, false);
        auto randomizer = acmacs::chart::randomizer_border_with_current_layout_area(*new_projection, 1.0, {split_data.serum_line.line(), split_data.good_side});
        new_projection->randomize_layout(split_data.on_the_wrong_side, randomizer);
        new_projection->comment("resolver " + sublevel_path);
        new_projection->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_precision::rough));
        const auto procrustes_data = new_projection->orient_to(*original_projection);
        std::cerr << level << ' ' << sublevel_path << " wrong-side: " << split_data.on_the_wrong_side.size() << "  rms: " << procrustes_data.rms << '\n';

        if (procrustes_data.rms > options.rms_threshold && level < options.max_levels_in_randomization_descent) {
            std::cerr << new_projection->make_info() << '\n';
            new_projection = randomize_found_on_the_wrong_side_of_serum_line(chart, new_projection, level + 1, sublevel_path, options);
        }

        std::cerr << "    " << new_projection->make_info() << '\n';
        results[attempt] = new_projection;
    }
    return *std::min_element(results.begin(), results.end(), [](const auto& p1, const auto& p2) { return p1->stress() < p2->stress(); });

} // randomize_found_on_the_wrong_side_of_serum_line

// ----------------------------------------------------------------------


acmacs::chart::ProjectionModifyP flip_relax(acmacs::chart::ChartModify& chart, acmacs::chart::ProjectionModifyP original_projection, const Options& options)
{
    SplitData split_data(*original_projection);
    if (split_data.serum_line.standard_deviation() > options.serum_line_sd_threshold)
        throw std::runtime_error("serum line sd " + std::to_string(split_data.serum_line.standard_deviation()) + " > " + acmacs::to_string(options.serum_line_sd_threshold, 8));
    std::cerr << "antigens on the wrong side: " << split_data.on_the_wrong_side.size() << '\n';

    // mark bad side antigens
    acmacs::PointStyle style;
    style.outline = "orange";
    style.outline_width = Pixels{2};
    chart.plot_spec_modify()->modify(split_data.on_the_wrong_side, style);

    // flip bad side antigens to good side
    auto flipped = chart.projections_modify()->new_by_cloning(*original_projection);
    flipped->comment("flipped " + std::to_string(split_data.on_the_wrong_side.size()) + " antigens");
    auto layout = flipped->layout();
    for (auto index : split_data.on_the_wrong_side)
        flipped->move_point(index, split_data.serum_line.line().flip_over(layout->get(index), 1.0));

    // relax from flipped
    auto relax_from_flipped = flipped; // chart.projections_modify()->new_by_cloning(*flipped);
    relax_from_flipped->comment(relax_from_flipped->comment() + ", relaxed");
    relax_from_flipped->relax(acmacs::chart::optimization_options(acmacs::chart::optimization_precision::rough));
    relax_from_flipped->orient_to(*original_projection);

    return relax_from_flipped;

} // flip_relax

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
