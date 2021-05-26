#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/fmt.hh"
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
    option<str>    reorient{*this, "reorient", dflt{""}, desc{"chart to re-orient resulting projections to"}};
    option<str>    grid_json{*this, "grid-json", desc{"export grid test results into json"}};
    option<double> grid_step{*this, "step", dflt{0.1}};
    option<str>    method{*this, "method", dflt{"alglib-cg"}, desc{"method: alglib-lbfgs, alglib-cg, optim-bfgs, optim-differential-evolution"}};
    option<bool>   dimension_annealing{*this, "dimension-annealing"};
    option<double> max_distance_multiplier{*this, "md", dflt{2.0}, desc{"randomization diameter multiplier"}};
    option<size_t> keep_projections{*this, "keep-projections", dflt{0UL}, desc{"number of projections to keep, 0 - keep all"}};
    option<bool>   keep_original_projections{*this, "keep-original-projections"};
    option<bool>   no_disconnect_having_few_titers{*this, "no-disconnect-having-few-titers"};
    option<str>    disconnect_antigens{*this, "disconnect-antigens", dflt{""}, desc{"comma or space separated list of antigen/point indexes (0-based) to disconnect for the new projections"}};
    option<str>    disconnect_sera{*this, "disconnect-sera", dflt{""}, desc{"comma or space separated list of serum indexes (0-based) to disconnect for the new projections"}};
    option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for optimization (omp): 0 - autodetect, 1 - sequential"}};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};

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
        if (!opt.keep_original_projections)
            chart.projections_modify().remove_all();
        const auto method{acmacs::chart::optimization_method_from_string(opt.method)};
        auto disconnected{acmacs::chart::get_disconnected(opt.disconnect_antigens, opt.disconnect_sera, chart.number_of_antigens(), chart.number_of_sera())};

        std::shared_ptr<acmacs::chart::Chart> master;
        if (opt.output_chart.has_value() && !opt.reorient->empty()) {
            master = acmacs::chart::import_from_file(opt.reorient, acmacs::chart::Verify::None);
            if (master->number_of_projections() == 0)
                throw std::runtime_error(fmt::format("chart for reorienting has no projections: {}", *opt.reorient));
        }

        auto& projections = chart.projections_modify();

        {
            // relax
            const Timeit ti_relax(fmt::format("{} rough optimizations: ", opt.number_of_optimizations), report_time::yes);
            acmacs::chart::optimization_options options(method, acmacs::chart::optimization_precision::rough, opt.max_distance_multiplier);
            const auto dimension_annealing = acmacs::chart::use_dimension_annealing_from_bool(opt.dimension_annealing); // && method != acmacs::chart::optimization_method::optimlib_differential_evolution);
            options.disconnect_too_few_numeric_titers = opt.no_disconnect_having_few_titers ? acmacs::chart::disconnect_few_numeric_titers::no : acmacs::chart::disconnect_few_numeric_titers::yes;
            options.num_threads = opt.threads;
            chart.relax(acmacs::chart::number_of_optimizations_t{*opt.number_of_optimizations}, *opt.minimum_column_basis, acmacs::number_of_dimensions_t{*opt.number_of_dimensions}, dimension_annealing, options, disconnected);
            projections.sort();
            options.precision = acmacs::chart::optimization_precision::fine;
            chart.projection_modify(0)->relax(options);
        }

        const size_t projection_no_to_test = 0, relax_attempts = 20;
        const auto [grid_results, grid_projections] = acmacs::chart::grid_test(chart, projection_no_to_test, opt.grid_step, opt.threads, relax_attempts, opt.grid_json);

        if (const size_t keep_projections = opt.keep_projections; keep_projections > 0 && projections.size() > (keep_projections + grid_projections))
            projections.keep_just(keep_projections + grid_projections);

        if (opt.output_chart.has_value() && !opt.reorient->empty()) {
            acmacs::chart::CommonAntigensSera common(*master, chart, acmacs::chart::CommonAntigensSera::match_level_t::automatic);
            auto master_projection = master->projection(0);
            for (auto projection_to_reorient : acmacs::filled_with_indexes(chart.number_of_projections())) {
                const auto procrustes_data = acmacs::chart::procrustes(*master_projection, *chart.projection(projection_to_reorient), common.points(), acmacs::chart::procrustes_scaling_t::no);
                chart.projection_modify(projection_to_reorient)->transformation(procrustes_data.transformation);
            }
        }

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
