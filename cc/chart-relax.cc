#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/grid-test.hh"
#include "acmacs-chart-2/log.hh"
#include "acmacs-chart-2/command-helper.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> number_of_optimizations{*this, 'n', dflt{1UL}, desc{"number of optimizations"}};
    option<size_t> number_of_dimensions{*this, 'd', dflt{2UL}, desc{"number of dimensions"}};
    option<str>    minimum_column_basis{*this, 'm', dflt{"none"}, desc{"minimum column basis"}};
    option<bool>   rough{*this, "rough"};
    option<size_t> fine{*this, "fine", dflt{0UL}, desc{"relax roughly, then relax finely N best projections"}};
    option<bool>   incremental{*this, "incremental", desc{"only randomize points having NaN coordinates"}};
    option<bool>   unmovable_non_nan_points{*this, "unmovable-non-nan-points", desc{"requires --incremental, keep ag/sr of primary chart frozen (unmovable)"}};
    option<bool>   grid{*this, "grid", desc{"perform grid test after optimization until no trapped points left"}};
    option<str>    grid_json{*this, "grid-json", desc{"export grid test results into json"}};
    option<double> grid_step{*this, "grid-step", dflt{0.1}};
    // option<bool>   export_pre_grid{*this, "export-pre-grid", desc{"export chart before running grid test (to help debugging crashes)"}};
    option<bool>   no_dimension_annealing{*this, "no-dimension-annealing"};
    option<bool>   dimension_annealing{*this, "dimension-annealing"};
    option<str>    method{*this, "method", dflt{"alglib-cg"}, desc{"method: alglib-lbfgs, alglib-cg, optim-bfgs, optim-differential-evolution"}};
    option<double> randomization_diameter_multiplier{*this, "md", dflt{2.0}, desc{"randomization diameter multiplier"}};
    option<bool>   remove_original_projections{*this, "remove-original-projections", desc{"remove projections found in the source chart"}};
    option<size_t> keep_projections{*this, "keep-projections", dflt{0UL}, desc{"number of projections to keep, 0 - keep all"}};
    option<bool>   no_disconnect_having_few_titers{*this, "no-disconnect-having-few-titers"};
    option<str>    disconnect_antigens{*this, "disconnect-antigens", dflt{""}, desc{"comma or space separated list of antigen/point indexes (0-based) to disconnect for the new projections"}};
    option<str>    disconnect_sera{*this, "disconnect-sera", dflt{""}, desc{"comma or space separated list of serum indexes (0-based) to disconnect for the new projections"}};
    option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for optimization (omp): 0 - autodetect, 1 - sequential"}};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};
    option<unsigned> seed{*this, "seed", desc{"seed for randomization, -n 1 implied"}};

    argument<str>  source_chart{*this, arg_name{"source-chart"}, mandatory};
    argument<str>  output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {

        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);
        acmacs::log::enable(acmacs::log::relax);

        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source_chart, acmacs::chart::Verify::None)};
        auto& projections = chart.projections_modify();
        if (opt.remove_original_projections && !opt.incremental)
            projections.remove_all();
        const auto precision = (opt.rough || opt.fine > 0) ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
        const auto method{acmacs::chart::optimization_method_from_string(opt.method)};
        auto disconnected{acmacs::chart::get_disconnected(opt.disconnect_antigens, opt.disconnect_sera, chart.number_of_antigens(), chart.number_of_sera())};
        const size_t incremental_source_projection_no = 0;

        acmacs::chart::optimization_options options(method, precision, opt.randomization_diameter_multiplier);
        options.disconnect_too_few_numeric_titers = opt.no_disconnect_having_few_titers ? acmacs::chart::disconnect_few_numeric_titers::no : acmacs::chart::disconnect_few_numeric_titers::yes;

        if (opt.no_dimension_annealing)
            AD_WARNING("option --no-dimension-annealing is deprectaed, dimension annealing is disabled by default, use --dimension-annealing to enable");
        const auto dimension_annealing =
            acmacs::chart::use_dimension_annealing_from_bool(opt.dimension_annealing); // && method != acmacs::chart::optimization_method::optimlib_differential_evolution);

        if (opt.seed.has_value()) {
            // --- seeded optimization ---
            if (opt.number_of_optimizations != 1ul)
                fmt::print(stderr, "WARNING: can only perform one optimization when seed is used\n");
            if (opt.incremental)
                chart.relax_incremental(incremental_source_projection_no, acmacs::chart::number_of_optimizations_t{1}, options,
                                        opt.remove_original_projections ? acmacs::chart::remove_source_projection::yes : acmacs::chart::remove_source_projection::no,
                                        opt.unmovable_non_nan_points ? acmacs::chart::unmovable_non_nan_points::yes : acmacs::chart::unmovable_non_nan_points::no);
            else
                chart.relax(*opt.minimum_column_basis, acmacs::number_of_dimensions_t{*opt.number_of_dimensions}, dimension_annealing, options, opt.seed, disconnected);
        }
        else {
            options.num_threads = opt.threads;
            if (opt.incremental)
                chart.relax_incremental(incremental_source_projection_no, acmacs::chart::number_of_optimizations_t{*opt.number_of_optimizations}, options,
                                        opt.remove_original_projections ? acmacs::chart::remove_source_projection::yes : acmacs::chart::remove_source_projection::no,
                                        opt.unmovable_non_nan_points ? acmacs::chart::unmovable_non_nan_points::yes : acmacs::chart::unmovable_non_nan_points::no);
            else
                chart.relax(acmacs::chart::number_of_optimizations_t{*opt.number_of_optimizations}, *opt.minimum_column_basis, acmacs::number_of_dimensions_t{*opt.number_of_dimensions},
                            dimension_annealing, options, disconnected);

            if (opt.grid) {
                const size_t projection_no_to_test = 0, relax_attempts = 20;
                const auto [grid_results, grid_projections] = acmacs::chart::grid_test(chart, projection_no_to_test, opt.grid_step, opt.threads, relax_attempts, opt.grid_json);
            }
        }

        projections.sort();
        for (size_t p_no = 0; p_no < opt.fine; ++p_no)
            chart.projection_modify(p_no)->relax(acmacs::chart::optimization_options(method, acmacs::chart::optimization_precision::fine));
        if (const size_t keep_projections = opt.keep_projections; keep_projections > 0 && projections.size() > keep_projections)
            projections.keep_just(keep_projections);
        fmt::print("{}\n", chart.make_info());
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name());
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
