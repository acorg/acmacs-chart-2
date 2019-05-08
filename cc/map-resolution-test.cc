#include "acmacs-base/range.hh"
#include "acmacs-chart-2/map-resolution-test.hh"
#include "acmacs-chart-2/chart-modify.hh"

static void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::MapResoltionTestParameters& parameters);
static void relax_with_proportion_dontcared(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, size_t replicate_no, const acmacs::chart::MapResoltionTestParameters& parameters);

// ----------------------------------------------------------------------

acmacs::chart::MapResoltionTestResults acmacs::chart::map_resolution_test(ChartModify& chart, const MapResoltionTestParameters& parameters)
{
    chart.projections_modify()->remove_all();
    for (auto number_of_dimensions : parameters.number_of_dimensions) {
        for (auto proportion_to_dont_care : parameters.proportions_to_dont_care) {
            if (parameters.relax_from_full_table == relax_from_full_table::yes)
                relax(chart, number_of_dimensions, parameters);
            for (auto replicate_no : range(parameters.number_of_random_replicates_for_each_proportion))
                relax_with_proportion_dontcared(chart, number_of_dimensions, proportion_to_dont_care, replicate_no + 1, parameters);
        }
    }

} // acmacs::chart::map_resolution_test

// ----------------------------------------------------------------------

void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::MapResoltionTestParameters& parameters)
{
    chart.relax(parameters.number_of_optimizations, parameters.minimum_column_basis, number_of_dimensions, acmacs::chart::use_dimension_annealing::yes, {parameters.optimization_precision}, acmacs::chart::report_stresses::no);

} // relax

// ----------------------------------------------------------------------

void relax_with_proportion_dontcared(acmacs::chart::ChartModify& master_chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, size_t replicate_no, const acmacs::chart::MapResoltionTestParameters& parameters)
{
    acmacs::chart::ChartClone chart(master_chart, acmacs::chart::ChartClone::clone_data::titers);
    chart.info_modify()->name_append(string::concat(proportion_to_dont_care, "-dont-cared"));
    chart.titers_modify()->remove_layers();
    chart.titers_modify()->set_proportion_of_titers_to_dont_care(proportion_to_dont_care);
    if (parameters.column_bases_from_master == acmacs::chart::column_bases_from_master::yes)
        chart.forced_column_bases_modify(*master_chart.column_bases(parameters.minimum_column_basis));
    if (parameters.relax_from_full_table == acmacs::chart::relax_from_full_table::yes) {
        auto projection = chart.projections_modify()->new_by_cloning(*master_chart.projections_modify()->at(0), true);
        projection->set_forced_column_bases(master_chart.column_bases(parameters.minimum_column_basis));
        projection->comment("relaxed-from-full-table-best");
        projection->relax(acmacs::chart::optimization_options{parameters.optimization_precision});
    }
    relax(chart, number_of_dimensions, parameters);

    std::cout << "replicate:" << replicate_no << " dim:" << number_of_dimensions << " prop:" << proportion_to_dont_care << '\n'
              << chart.make_info() << "\n\n";

    // collect statistics

} // relax_with_proportion_dontcared

// ----------------------------------------------------------------------


/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
