#include "acmacs-chart-2/map-resolution-test.hh"
#include "acmacs-chart-2/chart-modify.hh"

static void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::map_resolution_test_data::Parameters& parameters);
static acmacs::chart::map_resolution_test_data::Predictions relax_with_proportion_dontcared(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, size_t replicate_no, const acmacs::chart::map_resolution_test_data::Parameters& parameters);
static acmacs::chart::map_resolution_test_data::ReplicateStat collect_errors(acmacs::chart::ChartModify& master_chart, acmacs::chart::ChartModify& prediction_chart, const acmacs::chart::map_resolution_test_data::Parameters& parameters);

// ----------------------------------------------------------------------

acmacs::chart::map_resolution_test_data::Results acmacs::chart::map_resolution_test(ChartModify& chart, const map_resolution_test_data::Parameters& parameters)
{
    map_resolution_test_data::Results results(parameters);
    chart.projections_modify()->remove_all();
    // std::cout << "master dot-cares: " << (1.0 - chart.titers()->percent_of_non_dont_cares()) << '\n' << chart.titers()->print() << '\n';
    for (auto number_of_dimensions : parameters.number_of_dimensions) {
        for (auto proportion_to_dont_care : parameters.proportions_to_dont_care) {
            if (parameters.relax_from_full_table == map_resolution_test_data::relax_from_full_table::yes)
                relax(chart, number_of_dimensions, parameters);
            std::vector<double> av_abs_error(parameters.number_of_random_replicates_for_each_proportion), sd_error(parameters.number_of_random_replicates_for_each_proportion),
                correlations(parameters.number_of_random_replicates_for_each_proportion), r2(parameters.number_of_random_replicates_for_each_proportion);
            size_t number_of_samples = 0;
            for (auto replicate_no : range(parameters.number_of_random_replicates_for_each_proportion)) {
                const auto predictions = relax_with_proportion_dontcared(chart, number_of_dimensions, proportion_to_dont_care, replicate_no + 1, parameters);
                av_abs_error[replicate_no] = predictions.av_abs_error;
                sd_error[replicate_no] = predictions.sd_error;
                correlations[replicate_no] = predictions.correlation;
                r2[replicate_no] = predictions.linear_regression.r2();
                number_of_samples += predictions.number_of_samples;
            }

            results.predictions().emplace_back(number_of_dimensions, proportion_to_dont_care, statistics::standard_deviation(av_abs_error), statistics::standard_deviation(sd_error),
                                 statistics::standard_deviation(correlations), statistics::standard_deviation(r2), number_of_samples);

            // std::cout << results.predictions().back() << '\n';
        }
    }

    return results;

} // acmacs::chart::map_resolution_test

// ----------------------------------------------------------------------

void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::map_resolution_test_data::Parameters& parameters)
{
    chart.relax(parameters.number_of_optimizations, parameters.minimum_column_basis, number_of_dimensions, acmacs::chart::use_dimension_annealing::yes, {parameters.optimization_precision}, acmacs::chart::report_stresses::no);

} // relax

// ----------------------------------------------------------------------

acmacs::chart::map_resolution_test_data::Predictions relax_with_proportion_dontcared(acmacs::chart::ChartModify& master_chart, acmacs::number_of_dimensions_t number_of_dimensions,
                                                                                     double proportion_to_dont_care, [[maybe_unused]] size_t replicate_no,
                                                                                     const acmacs::chart::map_resolution_test_data::Parameters& parameters)
{
    acmacs::chart::ChartClone chart(master_chart, acmacs::chart::ChartClone::clone_data::titers);
    chart.info_modify()->name_append(string::concat(proportion_to_dont_care, "-dont-cared"));
    chart.titers_modify()->remove_layers();
    chart.titers_modify()->set_proportion_of_titers_to_dont_care(proportion_to_dont_care);
    if (parameters.column_bases_from_master == acmacs::chart::map_resolution_test_data::column_bases_from_master::yes)
        chart.forced_column_bases_modify(*master_chart.column_bases(parameters.minimum_column_basis));
    if (parameters.relax_from_full_table == acmacs::chart::map_resolution_test_data::relax_from_full_table::yes) {
        auto projection = chart.projections_modify()->new_by_cloning(*master_chart.projections_modify()->at(0), true);
        projection->set_forced_column_bases(master_chart.column_bases(parameters.minimum_column_basis));
        projection->comment("relaxed-from-full-table-best");
        projection->relax(acmacs::chart::optimization_options{parameters.optimization_precision});
    }
    relax(chart, number_of_dimensions, parameters);
    chart.projections_modify()->sort();

    // collect statistics
    const auto replicate_stat = collect_errors(master_chart, chart, parameters);
    std::vector<double> prediction_errors(replicate_stat.prediction_errors_for_titers.size());
    std::transform(std::begin(replicate_stat.prediction_errors_for_titers), std::end(replicate_stat.prediction_errors_for_titers), std::begin(prediction_errors),
                   [=](const auto& entry) -> double {
                       // std::cout << "dim:" << number_of_dimensions << " prop:" << proportion_to_dont_care << " ag:" << entry.antigen << " sr:" << entry.serum << " err:" << entry.error << '\n';
                       return entry.error;
                   });

    const acmacs::chart::map_resolution_test_data::Predictions predictions{
        acmacs::statistics::mean_abs(prediction_errors), acmacs::statistics::standard_deviation(prediction_errors).population_sd(),
        acmacs::statistics::correlation(replicate_stat.master_distances, replicate_stat.predicted_distances),
        acmacs::statistics::simple_linear_regression(std::begin(replicate_stat.master_distances), std::end(replicate_stat.master_distances), std::begin(replicate_stat.predicted_distances)),
        prediction_errors.size()};

    // std::cout << "replicate:" << replicate_no << " dim:" << number_of_dimensions << " prop:" << proportion_to_dont_care << '\n'
    //           << "    " << predictions << '\n'
    //           // << chart.make_info(*parameters.number_of_optimizations + 10)
    //           << '\n';

    //  if (*number_of_dimensions == 5 && float_equal(proportion_to_dont_care, 0.1)) {
    //      std::cout << "dot-cares: " << (1.0 - chart.titers_modify()->percent_of_non_dont_cares()) << '\n' << chart.titers_modify()->print() << '\n';
    //      std::cout << replicate_no << " corr:" << predictions.correlation << " mean-abs:" << predictions.av_abs_error << ' ' << replicate_stat.master_distances << ' ' << replicate_stat.predicted_distances << '\n';
    // //      // std::cout << "DEBUG: rep:" << replicate_no << " mean-abs:" << predictions.av_abs_error << " sd-err:" << predictions.sd_error << "\n " << prediction_errors << '\n';
    // //      // std::cout << "DEBUG: corr:" << predictions.correlation << " var-m:" << acmacs::statistics::varianceN(std::begin(replicate_stat.master_distances), std::end(replicate_stat.master_distances))
    // //      //          << "\n  m: " << replicate_stat.master_distances << "\n  p: " << replicate_stat.predicted_distances << '\n';
    //  }

    return predictions;

} // relax_with_proportion_dontcared

// ----------------------------------------------------------------------

acmacs::chart::map_resolution_test_data::ReplicateStat collect_errors(acmacs::chart::ChartModify& master_chart, acmacs::chart::ChartModify& prediction_chart, const acmacs::chart::map_resolution_test_data::Parameters& parameters)
{
    const auto number_of_antigens = master_chart.number_of_antigens();
    auto prediction_projection = prediction_chart.projection(0);
    auto prediction_layout = prediction_projection->layout();
    auto prediction_chart_titers = prediction_chart.titers();
    auto master_chart_titers = master_chart.titers();
    auto master_column_bases = master_chart.column_bases(parameters.minimum_column_basis);

    acmacs::chart::map_resolution_test_data::ReplicateStat replicate_stat;

    for (const auto& master_titer_ref : master_chart_titers->titers_regular()) {
        // std::cout << ' ' << master_titer_ref << '\n';
        if (prediction_chart_titers->titer(master_titer_ref.antigen, master_titer_ref.serum).is_dont_care()) {
            // std::cout << master_titer_ref << '\n';
            const auto predicted_distance = prediction_layout->distance(master_titer_ref.antigen, master_titer_ref.serum + number_of_antigens);
            const auto master_distance = master_column_bases->column_basis(master_titer_ref.serum) - master_titer_ref.titer.logged();
            replicate_stat.prediction_errors_for_titers.emplace_back(master_titer_ref.antigen, master_titer_ref.serum, master_distance - predicted_distance);
            replicate_stat.master_distances.push_back(master_distance);
            replicate_stat.predicted_distances.push_back(predicted_distance);
        }
    }

    return replicate_stat;

} // collect_errors

// ----------------------------------------------------------------------

std::ostream& acmacs::chart::map_resolution_test_data::operator << (std::ostream& out, const Predictions& predictions)
{
    return out << "av_abs_error:" << predictions.av_abs_error
               << " sd_error:" << predictions.sd_error
               << " correlation:" << predictions.correlation
               << " linear_regression:" << predictions.linear_regression
               << " number_of_samples:" << predictions.number_of_samples;

} // acmacs::chart::map_resolution_test_data::operator <<

// ----------------------------------------------------------------------

std::ostream& acmacs::chart::map_resolution_test_data::operator << (std::ostream& out, const PredictionsSummary& predictions_summary)
{
    return out << "dimensions:" << acmacs::to_string(predictions_summary.number_of_dimensions)
               << " proportion:" << predictions_summary.proportion_to_dont_care
               << " (mean sd) av_abs_error(" << predictions_summary.av_abs_error.mean() << ' ' << predictions_summary.av_abs_error.population_sd() << ')'
               << " sd_error(" << predictions_summary.sd_error.mean() << ' ' << predictions_summary.sd_error.population_sd() << ')'
               << " correlations(" << predictions_summary.correlations.mean() << ' ' << predictions_summary.correlations.population_sd() << ')'
               << " r2(" << predictions_summary.r2.mean() << ' ' << predictions_summary.r2.population_sd() << ')'
               << " number_of_samples:" << predictions_summary.number_of_samples;

} // acmacs::chart::map_resolution_test_data::operator <<

// ----------------------------------------------------------------------

std::ostream& acmacs::chart::map_resolution_test_data::operator << (std::ostream& out, const Results& results)
{
    const auto print = [&out,&results](const char* prefix, double prop, auto func) {
        out << "  " << prefix << ':';
        for (auto dim : results.parameters_.number_of_dimensions) {
            for (const auto& entry : results.predictions()) {
                if (float_equal(entry.proportion_to_dont_care, prop) && entry.number_of_dimensions == dim)
                    out << ' ' << func(entry);
            }
        }
        out << '\n';
    };

    for (auto prop: results.parameters_.proportions_to_dont_care) {
        out << "prop:" << prop << '\n';
        print("av_abs_error", prop, [](const auto& entry) { return entry.av_abs_error.mean(); });
        print("av_abs_error_sd", prop, [](const auto& entry) { return entry.av_abs_error.population_sd(); });
        print("sd_error", prop, [](const auto& entry) { return entry.sd_error.mean(); });
        print("sd_error_sd", prop, [](const auto& entry) { return entry.sd_error.population_sd(); });
        print("correlation", prop, [](const auto& entry) { return entry.correlations.mean(); });
        print("correlation_sd", prop, [](const auto& entry) { return entry.correlations.population_sd(); });
        out << '\n';
    }
    return out;

} // acmacs::chart::map_resolution_test_data::operator <<

// ----------------------------------------------------------------------


/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
