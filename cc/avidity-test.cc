#include "acmacs-chart-2/avidity-test.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/optimize.hh"

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
    AD_DEBUG("AG {} adjust:{:4.1f} stress: {:10.4f} diff: {:8.4f}", antigen_no, logged_adjust, status.final_stress, status.final_stress - original_stress);

    PerAdjust result{.logged_adjust = logged_adjust,
                     .angle_test_antigen = 0.0, // todo
                     .average_procrustes_distances_except_test_antigen = 0.0, // todo
                     .distance_test_antigen = 0.0, // todo
                     .final = layout->at(antigen_no),
                     .stress_diff = status.final_stress - original_stress};
    return result;

} // acmacs::chart::avidity::test

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
