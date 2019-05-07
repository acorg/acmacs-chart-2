#include "acmacs-base/range.hh"
#include "acmacs-chart-2/map-resolution-test.hh"
#include "acmacs-chart-2/chart-modify.hh"

static void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::MapResoltionTestParameters& parameters); // acmacs::chart::number_of_optimizations_t number_of_optimizations, acmacs::chart::MinimumColumnBasis minimum_column_basis, enum acmacs::chart::optimization_precision optimization_precision);
static void relax_with_proportion_dontcared(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, const acmacs::chart::MapResoltionTestParameters& parameters);

// ----------------------------------------------------------------------

acmacs::chart::MapResoltionTestResults acmacs::chart::map_resolution_test(ChartModify& chart, const MapResoltionTestParameters& parameters)
{
    chart.projections_modify()->remove_all();
    for (auto number_of_dimensions : parameters.number_of_dimensions) {
        for (auto proportion_to_dont_care : parameters.proportions_to_dont_care) {
            if (parameters.relax_from_full_table == relax_from_full_table::yes)
                relax(chart, number_of_dimensions, parameters); // parameters.number_of_optimizations, parameters.minimum_column_basis, parameters.optimization_precision);
            for (auto replicate : range(parameters.number_of_random_replicates_for_each_proportion))
                relax_with_proportion_dontcared(chart, number_of_dimensions, proportion_to_dont_care, parameters); // parameters.number_of_optimizations, parameters.minimum_column_basis, parameters.optimization_precision);
        }
    }

} // acmacs::chart::map_resolution_test

// ----------------------------------------------------------------------

void relax(acmacs::chart::ChartModify& chart, acmacs::number_of_dimensions_t number_of_dimensions, const acmacs::chart::MapResoltionTestParameters& parameters) // acmacs::chart::number_of_optimizations_t number_of_optimizations, acmacs::chart::MinimumColumnBasis minimum_column_basis, enum acmacs::chart::optimization_precision optimization_precision)
{
    chart.relax(parameters.number_of_optimizations, parameters.minimum_column_basis, number_of_dimensions, acmacs::chart::use_dimension_annealing::yes, {parameters.optimization_precision}, acmacs::chart::report_stresses::no);

} // relax

// ----------------------------------------------------------------------

void relax_with_proportion_dontcared(acmacs::chart::ChartModify& master_chart, acmacs::number_of_dimensions_t number_of_dimensions, double proportion_to_dont_care, const acmacs::chart::MapResoltionTestParameters& parameters)
{
    acmacs::chart::ChartClone chart(master_chart);
    auto master_column_bases = parameters.column_bases_from_master == acmacs::chart::column_bases_from_master::yes ? master_chart.column_bases(parameters.minimum_column_basis) : std::shared_ptr<acmacs::chart::ColumnBases>{};

            // self.rnd_first = chart.set_proportion_of_titers_to_dont_care(self.proportion_to_dont_care)
            // if master_column_bases is not None:
            //     chart.table.set_column_bases(master_column_bases)
            // chart.info.comment = ' '.join(((chart.info.comment or ''), '{}-dont-cared'.format(self.proportion_to_dont_care))).strip()
            // if self.relax_from_full_table:
            //     chart.projections.remove_except('best')
            //     chart.projections[0].stress_evaluator_parameters.columnBases(master_column_bases)
            //     chart.projections[0].comment = "relaxed-from-full-table-best"
            //     chart.projections[0].relax(chart=chart, number_of_dimensions=self.number_of_dimensions, rough_optimization=self.rough_optimization, dodgy_titer_is_regular=self.dodgy_titer_is_regular)
            //     module_logger.info('Relaxed from full table: {}'.format(chart.projections[0].stress()))
            // else:
            //     chart.projections.remove('all')
            // entry_table, entry_projections = mongodb_collections.charts.new_from_chart(session=self.S, source=chart, copy_permissions_from=self, save=True, table_keywords={'map-resolution-test-result'})
            // source_id = entry_projections._id if entry_projections else entry_table._id
            // self.results = [source_id]

} // relax_with_proportion_dontcared

// ----------------------------------------------------------------------


/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
