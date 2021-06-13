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

// std::auto_ptr<Matrix<FloatType> > transformation(procrustes(*mLayoutAsMatrix, *layout_as_matrix, false, true));
// std::auto_ptr<Matrix<FloatType> > transformed(applyProcrustes(*layout_as_matrix, *transformation));
// ProcrustesDistancesSummaryParameters procrustes_distances_summary_parameters(layout.numberOfAntigens(), aAntigenNo, parameters().vaccineAntigen());
// std::auto_ptr<ProcrustesDistancesSummaryResults> summary(procrustesDistancesSummary(*mLayoutAsMatrix, *transformed, procrustes_distances_summary_parameters));
    const auto pc_data = procrustes(original_projection, projection, CommonAntigensSera{chart}.points(), procrustes_scaling_t::no);
    auto transformed_layout = pc_data.apply(*layout);
    const auto summary = procrustes_summary(*original_projection.layout(), *transformed_layout, ProcrustesSummaryParameters{.number_of_antigens=chart.number_of_antigens(), .antigen_being_tested=antigen_no});

    // aResult.distanceTestAntigen((*summary->antigens_distances)[aAntigenNo]);
    // aResult.angleTestAntigen(summary->test_antigen_angle);
    // aResult.averageProcrustesDistancesExceptTestAntigen(summary->average_distance);
    // if (parameters().validVaccineAntigen()) {
    //     aResult.distanceVaccineToTestAntigen(summary->distance_vaccine_to_test_antigen);
    //     aResult.angleVaccineToTestAntigen(summary->angle_vaccine_to_test_antigen);
    // }

    //   // build most moved antigens list and their distances (sorted by distances, longest first)
    // aResult.mostMovedAntigens().resize(0);
    // for (Array<Index>::const_iterator mm = summary->antigens_by_distance->begin(); mm != summary->antigens_by_distance->end() && Index(aResult.mostMovedAntigens().size()) <
    // parameters().mostMovedAntigens(); ++mm) {
    //     if (*mm != aAntigenNo) // do not put antigen being tested into the most moved list
    //         aResult.mostMovedAntigens().push_back(LowHighAvidityTestResultForMostMoved(*mm, (*summary->antigens_distances)[*mm]));
    // }

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
