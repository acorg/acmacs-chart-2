#include "acmacs-base/range.hh"
#include "acmacs-chart-2/map-resolution-test.hh"
#include "acmacs-chart-2/chart-modify.hh"

static void relax(acmacs::chart::ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, acmacs::chart::MinimumColumnBasis minimum_column_basis, enum acmacs::chart::optimization_precision optimization_precision);

// ----------------------------------------------------------------------

acmacs::chart::MapResoltionTestResults acmacs::chart::map_resolution_test(ChartModify& chart, const MapResoltionTestParameters& parameters)
{
    chart.projections_modify()->remove_all();
    for (auto number_of_dimensions : parameters.number_of_dimensions) {
        for (auto proportion_to_dont_care : parameters.proportions_to_dont_care) {
            if (parameters.relax_from_full_table == relax_from_full_table::yes)
                relax(chart, number_of_dimensions, parameters.number_of_optimizations, parameters.minimum_column_basis, parameters.optimization_precision);
        }
    }

} // acmacs::chart::map_resolution_test

// ----------------------------------------------------------------------

void relax(acmacs::chart::ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, acmacs::chart::MinimumColumnBasis minimum_column_basis, enum acmacs::chart::optimization_precision optimization_precision)
{
    chart.relax(number_of_optimizations, minimum_column_basis, number_of_dimensions, acmacs::chart::use_dimension_annealing::yes, {optimization_precision}, acmacs::chart::report_stresses::no);

} // relax

// ----------------------------------------------------------------------


/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
