#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/grid-test.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> number_of_optimizations{*this, 'n', dflt{1UL}, desc{"number of optimizations"}};
    option<size_t> number_of_dimensions{*this, 'd', dflt{2UL}, desc{"number of dimensions"}};
    option<str>    minimum_column_basis{*this, 'm', dflt{"none"}, desc{"minimum column basis"}};
    option<str>    reorient{*this, "reorient", dflt{""}, desc{"chart to re-orient resulting projections to"}};
    option<double> grid_step{*this, "step", dflt{0.1}};
    option<str>    method{*this, "method", dflt{"cg"}, desc{"method: lbfgs, cg"}};
    option<double> max_distance_multiplier{*this, "md", dflt{2.0}, desc{"randomization diameter multiplier"}};
    option<size_t> keep_projections{*this, "keep-projections", dflt{0UL}, desc{"number of projections to keep, 0 - keep all"}};
    option<bool>   no_disconnect_having_few_titers{*this, "no-disconnect-having-few-titers"};
    option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for optimization (omp): 0 - autodetect, 1 - sequential"}};
    option<bool>   report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool>   verbose{*this, 'v', "verbose"};

    argument<str>  source_chart{*this, arg_name{"source-chart"}, mandatory};
    argument<str>  output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);

        const Timeit ti("performing " + std::to_string(opt.number_of_optimizations) + " optimizations: ", report);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source_chart, acmacs::chart::Verify::None, report)};
        const auto method{acmacs::chart::optimization_method_from_string(opt.method)};
        acmacs::chart::PointIndexList disconnected;
        if (!opt.no_disconnect_having_few_titers)
            disconnected.extend(chart.titers()->having_too_few_numeric_titers());

        // relax
        acmacs::chart::optimization_options options(method, acmacs::chart::optimization_precision::rough, opt.max_distance_multiplier);
        options.num_threads = opt.threads;
        chart.relax(opt.number_of_optimizations, *opt.minimum_column_basis, opt.number_of_dimensions, true, options, opt.verbose, disconnected);
        auto projections = chart.projections_modify();
        projections->sort();
        chart.projection_modify(0)->relax(acmacs::chart::optimization_options(method, acmacs::chart::optimization_precision::fine));

        // grid test
        size_t grid_projections = 0;
        size_t projection_no_to_test = 0;
        for (auto attempt = 1; attempt < 20; ++attempt) {
            acmacs::chart::GridTest test(chart, projection_no_to_test, opt.grid_step);
            const auto results = test.test_all_parallel(opt.threads);
            std::cout << results.report() << '\n';
            if (opt.verbose) {
                for (const auto& entry : results) {
                    if (entry)
                        std::cout << entry.report() << '\n';
                }
            }
            auto projection = test.make_new_projection_and_relax(results, true);
            ++grid_projections;
            projection->comment("grid-test-" + acmacs::to_string(attempt));
            projection_no_to_test = projection->projection_no();
            // projections_to_reorient.push_back(projection_no_to_test);
            if (std::all_of(results.begin(), results.end(), [](const auto& result) { return result.diagnosis != acmacs::chart::GridTest::Result::trapped; }))
                break;
        }

        chart.projections_modify()->sort();
        if (const size_t keep_projections = opt.keep_projections; keep_projections > 0 && projections->size() > (keep_projections + grid_projections))
            projections->keep_just(keep_projections + grid_projections);

        if (opt.output_chart.has_value() && !opt.reorient->empty()) {
            auto master = acmacs::chart::import_from_file(opt.reorient, acmacs::chart::Verify::None, report);
            acmacs::chart::CommonAntigensSera common(*master, chart, acmacs::chart::CommonAntigensSera::match_level_t::automatic);
            auto master_projection = master->projection(0);
            for (auto projection_to_reorient : acmacs::filled_with_indexes(chart.number_of_projections())) {
                const auto procrustes_data = acmacs::chart::procrustes(*master_projection, *chart.projection(projection_to_reorient), common.points(), acmacs::chart::procrustes_scaling_t::no);
                chart.projection_modify(projection_to_reorient)->transformation(procrustes_data.transformation);
            }
        }

        std::cout << chart.make_info() << '\n';
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(chart, opt.output_chart, fs::path(opt.program_name()).filename(), report);
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
