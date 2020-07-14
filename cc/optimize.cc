#include <memory>

#include "acmacs-base/timeit.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/randomizer.hh"
#include "acmacs-chart-2/disconnected-points-handler.hh"
#include "acmacs-chart-2/alglib.hh"
#include "acmacs-chart-2/optim.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    static acmacs::chart::optimization_status optimize(acmacs::chart::optimization_method optimization_method, OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision);
}

// ----------------------------------------------------------------------

acmacs::chart::optimization_method acmacs::chart::optimization_method_from_string(std::string_view source)
{
    optimization_method method{optimization_method::alglib_cg_pca};
    if (source == "alglib-lbfgs")
        method = optimization_method::alglib_lbfgs_pca;
    else if (source == "alglib-cg")
        method = optimization_method::alglib_cg_pca;
    else if (source == "optim-bfgs")
        method = optimization_method::optimlib_bfgs_pca;
    else if (source == "optim-differential-evolution")
        method = optimization_method::optimlib_differential_evolution;
    else
        throw std::runtime_error{fmt::format("unrecognized method: \"{}\", expected: alglib-lbfgs, alglib-cg, optim-bfgs", source)};
    return method;

} // acmacs::chart::optimization_method_from_string

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(acmacs::chart::ProjectionModify& projection, acmacs::chart::optimization_options options)
{
    auto layout = projection.layout_modified();
    auto stress = stress_factory(projection, options.mult);
    OptimiserCallbackData callback_data(stress);
    return optimize(options.method, callback_data, layout->data(), layout->data() + layout->size(), options.precision);

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(ProjectionModify& projection, IntermediateLayouts& intermediate_layouts, optimization_options options)
{
    auto layout = projection.layout_modified();
    auto stress = stress_factory(projection, options.mult);
    OptimiserCallbackData callback_data(stress, intermediate_layouts);
    return optimize(options.method, callback_data, layout->data(), layout->data() + layout->size(), options.precision);

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(ProjectionModify& projection, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_options options)
{
    if (schedule.initial() != projection.number_of_dimensions())
        throw std::runtime_error("acmacs::chart::optimize existing with dimension_schedule: invalid number_of_dimensions in schedule");

    const auto start = std::chrono::high_resolution_clock::now();
    optimization_status status(options.method);
    auto layout = projection.layout_modified();
    auto stress = stress_factory(projection, options.mult);

    bool initial_opt = true;
    for (auto num_dims: schedule) {
        if (!initial_opt) {
            dimension_annealing(options.method, stress, projection.number_of_dimensions(), num_dims, layout->data(), layout->data() + layout->size());
            layout->change_number_of_dimensions(num_dims);
            stress.change_number_of_dimensions(num_dims);
        }
        const auto sub_status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
        if (initial_opt) {
            status.initial_stress = sub_status.initial_stress;
            status.termination_report = sub_status.termination_report;
        }
        else {
            status.termination_report += "\n" + sub_status.termination_report;
        }
        status.final_stress = sub_status.final_stress;
        status.number_of_iterations += sub_status.number_of_iterations;
        status.number_of_stress_calculations += sub_status.number_of_stress_calculations;
        initial_opt = false;
    }
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return status;

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(acmacs::chart::ChartModify& chart, MinimumColumnBasis minimum_column_basis, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_options options)
{
    auto projection = chart.projections_modify()->new_from_scratch(schedule.initial(), minimum_column_basis);
    projection->randomize_layout(randomizer_plain_with_table_max_distance(*projection));
    return optimize(*projection, schedule, options);

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(optimization_method optimization_method, const Stress& stress, double* arg_first, double* arg_last, optimization_precision precision)
{
    OptimiserCallbackData callback_data(stress);
    return optimize(optimization_method, callback_data, arg_first, arg_last, precision);

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(acmacs::chart::optimization_method optimization_method, OptimiserCallbackData& callback_data, double* arg_first, double* arg_last,
                                                           acmacs::chart::optimization_precision precision)
{
    DisconnectedPointsHandler disconnected_point_handler{callback_data.stress, arg_first};
    optimization_status status(optimization_method);
    status.initial_stress = callback_data.stress.value(arg_first);
    const auto start = std::chrono::high_resolution_clock::now();
    switch (optimization_method) {
        case optimization_method::alglib_lbfgs_pca:
            alglib::lbfgs_optimize(status, callback_data, arg_first, arg_last, precision);
            break;
        case optimization_method::alglib_cg_pca:
            alglib::cg_optimize(status, callback_data, arg_first, arg_last, precision);
            break;
        case optimization_method::optimlib_bfgs_pca:
            optim::bfgs(status, callback_data, arg_first, arg_last, precision);
            break;
        case optimization_method::optimlib_differential_evolution:
            optim::differential_evolution(status, callback_data, arg_first, arg_last, precision);
            alglib::cg_optimize(status, callback_data, arg_first, arg_last, precision);
            break;
    }
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    status.final_stress = callback_data.stress.value(arg_first);
    return status;

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::ErrorLines acmacs::chart::error_lines(const acmacs::chart::Projection& projection)
{
    auto layout = projection.layout();
    auto stress = stress_factory(projection, multiply_antigen_titer_until_column_adjust::yes);
    const auto& table_distances = stress.table_distances();
    const MapDistances map_distances(*layout, table_distances);
    ErrorLines result;
    for (auto td = table_distances.regular().begin(), md = map_distances.regular().begin(); td != table_distances.regular().end(); ++td, ++md) {
        result.emplace_back(td->point_1, td->point_2, td->distance - md->distance);
    }
    for (auto td = table_distances.less_than().begin(), md = map_distances.less_than().begin(); td != table_distances.less_than().end(); ++td, ++md) {
        auto diff = td->distance - md->distance + 1;
        diff *= std::sqrt(acmacs::sigmoid(diff * SigmoidMutiplier())); // see Derek's message Thu, 10 Mar 2016 16:32:20 +0000 (Re: acmacs error line error)
        result.emplace_back(td->point_1, td->point_2, diff);
    }
    return result;

} // acmacs::chart::error_lines

// ----------------------------------------------------------------------

acmacs::chart::DimensionAnnelingStatus acmacs::chart::dimension_annealing(optimization_method optimization_method, const Stress& stress, number_of_dimensions_t source_number_of_dimensions,
                                                                          number_of_dimensions_t target_number_of_dimensions, double* arg_first, double* arg_last)
{
    DimensionAnnelingStatus status;
    OptimiserCallbackData callback_data(stress);
    const auto start = std::chrono::high_resolution_clock::now();

    switch (optimization_method) {
        case optimization_method::alglib_lbfgs_pca:
        case optimization_method::alglib_cg_pca:
        case optimization_method::optimlib_bfgs_pca:
            alglib::pca(callback_data, source_number_of_dimensions, target_number_of_dimensions, arg_first, arg_last);
            break;
      case optimization_method::optimlib_differential_evolution:
          throw std::runtime_error{"optimlib_differential_evolution method does not support dimension annealing"};
    }

    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return status;

} // acmacs::chart::dimension_annealing

// ----------------------------------------------------------------------

void acmacs::chart::pca(const Stress& stress, number_of_dimensions_t number_of_dimensions, double* arg_first, double* arg_last)
{
    OptimiserCallbackData callback_data(stress);
    alglib::pca_full(callback_data, number_of_dimensions, arg_first, arg_last);

} // acmacs::chart::pca

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
