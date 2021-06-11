#include "acmacs-chart-2/avidity-test.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/optimize.hh"

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

acmacs::chart::avidity::Result acmacs::chart::avidity::test(const ChartModify& chart, const ProjectionModify& original_projection, size_t antigen_no, const Settings& settings,
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

acmacs::chart::avidity::PerAdjust acmacs::chart::avidity::test(const acmacs::chart::ChartModify& chart, const acmacs::chart::ProjectionModify& original_projection, size_t antigen_no,
                                                               double logged_adjust, const acmacs::chart::optimization_options& options)
{
    const auto original_stress = original_projection.stress();
    ProjectionModifyNew projection{original_projection, chart};
    projection.avidity_adjusts_modify().resize(chart.number_of_antigens() + chart.number_of_sera());
    auto layout = projection.layout_modified();
    auto stress = stress_factory(projection, antigen_no, logged_adjust, options.mult);
    const auto status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
    // AD_DEBUG("AG {} adjust:{:4.1f} stress: {:10.4f} diff: {:8.4f}", antigen_no, logged_adjust, status.final_stress, status.final_stress - original_stress);

    PerAdjust result{.logged_adjust = logged_adjust,
                     .angle_test_antigen = 0.0, // todo
                     .average_procrustes_distances_except_test_antigen = 0.0, // todo
                     .distance_test_antigen = 0.0, // todo
                     .final = layout->at(antigen_no),
                     .stress_diff = status.final_stress - original_stress};
    return result;

} // acmacs::chart::avidity::test

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
