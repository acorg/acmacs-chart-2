#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/avidity-test.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/optimize.hh"
#include "acmacs-chart-2/procrustes.hh"

// ----------------------------------------------------------------------

acmacs::chart::avidity::Results acmacs::chart::avidity::test(ChartModify& chart, size_t projection_no, const Settings& settings, const optimization_options& options)
{
    auto projection = chart.projection_modify(projection_no);
    Results results{.original_stress = projection->stress()};
    for (size_t ag_no{0}; ag_no < chart.number_of_antigens(); ++ag_no)
        results.results.push_back(test(chart, *projection, ag_no, settings, options));
    results.post_process();
    return results;

} // acmacs::chart::avidity::test

// ----------------------------------------------------------------------

acmacs::chart::avidity::Results acmacs::chart::avidity::test(ChartModify& chart, size_t projection_no, const std::vector<size_t>& antigens_to_test, const Settings& settings, const optimization_options& options)
{
    auto projection = chart.projection_modify(projection_no);
    Results results{.original_stress = projection->stress()};
    for (size_t ag_no : antigens_to_test)
        results.results.push_back(test(chart, *projection, ag_no, settings, options));
    results.post_process();
    return results;

} // acmacs::chart::avidity::test

// ----------------------------------------------------------------------

acmacs::chart::avidity::Result acmacs::chart::avidity::test(ChartModify& chart, const ProjectionModify& original_projection, size_t antigen_no, const Settings& settings,
                                                            const optimization_options& options)
{
    Result result{.antigen_no = antigen_no, .best_logged_adjust = 0.0, .original = original_projection.layout()->at(antigen_no)};
    // low avidity
    for (double adjust = settings.step; adjust <= settings.max_adjust; adjust += settings.step)
        result.adjusts.push_back(test(chart, original_projection, antigen_no, adjust, options));
    // high avidity
    for (double adjust = - settings.step; adjust >= settings.min_adjust; adjust -= settings.step)
        result.adjusts.push_back(test(chart, original_projection, antigen_no, adjust, options));

    return result;

} // acmacs::chart::avidity::test

// ----------------------------------------------------------------------

acmacs::chart::avidity::PerAdjust acmacs::chart::avidity::test(ChartModify& chart, const ProjectionModify& original_projection, size_t antigen_no, double logged_adjust, const optimization_options& options, bool add_new_projection_to_chart)
{
    const auto original_stress = original_projection.stress();
    auto projection = chart.projections_modify().new_by_cloning(original_projection, add_new_projection_to_chart);
    auto& avidity_adjusts = projection->avidity_adjusts_modify();
    avidity_adjusts.resize(chart.number_of_antigens() + chart.number_of_sera());
    avidity_adjusts.set_logged(antigen_no, logged_adjust);
    projection->comment(fmt::format("avidity {:+.1f} AG {}", logged_adjust, antigen_no));
    auto stress = stress_factory(*projection, options.mult);
    // auto stress = stress_factory(*projection, antigen_no, logged_adjust, options.mult);
    auto layout = projection->layout_modified();
    const auto status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
    // AD_DEBUG("avidity relax AG {} adjust:{:4.1f} stress: {:10.4f} diff: {:8.4f}", antigen_no, logged_adjust, status.final_stress, status.final_stress - original_stress);

    const auto pc_data = procrustes(original_projection, *projection, CommonAntigensSera{chart}.points(), procrustes_scaling_t::no);
    // AD_DEBUG("AG {} pc-rms:{}", antigen_no, pc_data.rms);
    const auto summary = procrustes_summary(*original_projection.layout(), *pc_data.secondary_transformed,
                                            ProcrustesSummaryParameters{.number_of_antigens = chart.number_of_antigens(), .antigen_being_tested = antigen_no});

    PerAdjust result{.logged_adjust = logged_adjust,
                     .distance_test_antigen = summary.antigen_distances[antigen_no],
                     .angle_test_antigen = summary.test_antigen_angle,
                     .average_procrustes_distances_except_test_antigen = summary.average_distance,
                     .final_coordinates = layout->at(antigen_no),
                     .stress_diff = status.final_stress - original_stress};
    size_t most_moved_no{0};
    for (const auto ag_no : summary.antigens_by_distance) {
        if (ag_no != antigen_no) { // do not put antigen being tested into the most moved list
            result.most_moved[most_moved_no] = MostMoved{ag_no, summary.antigen_distances[ag_no]};
            ++most_moved_no;
            if (most_moved_no >= result.most_moved.size())
                break;
        }
    }
    // if (parameters().validVaccineAntigen()) {
    //     result.distance_vaccine_to_test_antigen = summary.distance_vaccine_to_test_antigen;
    //     result.angle_vaccine_to_test_antigen = summary.angle_vaccine_to_test_antigen;
    // }
    return result;

} // acmacs::chart::avidity::test

// ----------------------------------------------------------------------

const acmacs::chart::avidity::PerAdjust& acmacs::chart::avidity::Result::best_adjust() const
{
    if (const auto found = std::find_if(std::begin(adjusts), std::end(adjusts), [best = best_logged_adjust](const auto& en) { return float_equal(best, en.logged_adjust); });
        found != std::end(adjusts)) {
        return *found;
    }
    else {
        for (const auto& adjust : adjusts)
            AD_WARNING("avidity adjust {}", adjust);
        throw std::runtime_error{AD_FORMAT("avidity: no best adjust entry for {} (internal error)", best_logged_adjust)};
    }

} // acmacs::chart::avidity::Result::best_adjust

// ----------------------------------------------------------------------

void acmacs::chart::avidity::Result::post_process()
{
    if (const auto best = std::min_element(std::begin(adjusts), std::end(adjusts), [](const auto& en1, const auto& en2) { return en1.stress_diff < en2.stress_diff; });
        best != std::end(adjusts) && best->stress_diff < 0) {
        best_logged_adjust = best->logged_adjust;
    }

} // acmacs::chart::avidity::Result::post_process

// ----------------------------------------------------------------------

void acmacs::chart::avidity::Results::post_process()
{
    for (auto& result : results)
        result.post_process();

} // acmacs::chart::avidity::Results::post_process

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::ProjectionModify> acmacs::chart::avidity::move_antigens(ChartModify& chart, size_t projection_no, const Results& avidity_results)
{
    auto projection = chart.projections_modify().new_by_cloning(*chart.projection_modify(projection_no));
    auto layout = projection->layout_modified();
    auto& avidity_adjusts = projection->avidity_adjusts_modify();
    avidity_adjusts.resize(chart.number_of_antigens() + chart.number_of_sera());
    for (const auto& result : avidity_results.results) {
        if (!float_zero(result.best_logged_adjust)) {
            const auto& best_adjust = result.best_adjust();
            layout->update(result.antigen_no, best_adjust.final_coordinates);
            avidity_adjusts.set_logged(result.antigen_no, best_adjust.logged_adjust);
        }
    }
    return projection;

} // acmacs::chart::avidity::move_antigens

// ----------------------------------------------------------------------

void acmacs::chart::avidity::relax(ChartModify& chart, number_of_optimizations_t number_of_optimizations, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, const AvidityAdjusts& avidity_adjusts, const optimization_options& options)
{
    const auto first_projection_no = chart.number_of_projections();
    for ([[maybe_unused]] const auto  p_no : range_from_0_to(*number_of_optimizations)) {
        auto projection = chart.projections_modify().new_from_scratch(number_of_dimensions, minimum_column_basis);
        projection->avidity_adjusts_modify() = avidity_adjusts;
        projection->comment("avidity");
    }
    chart.relax_projections(options, first_projection_no);

} // acmacs::chart::avidity::relax

// ----------------------------------------------------------------------
