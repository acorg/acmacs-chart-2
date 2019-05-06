#pragma once

#include <vector>

#include "acmacs-chart-2/optimize-options.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs { class Layout; }

namespace acmacs::chart
{
    class ChartModify;

     //  additional projection in each replicate, first full table is
     //  relaxed, then titers dont-cared and the best projection
     //  relaxed again from already found starting coordinates.
    enum class relax_from_full_table { no, yes };

     // converting titers to dont-care may change column bases, force
     // master chart column bases
    enum class column_bases_from_master { no, yes };

    struct MapResoltionTestParameters
    {
        std::vector<size_t> number_of_dimensions{1, 2, 3, 4, 5};
        size_t number_of_optimizations{100};
        size_t number_of_random_replicates_for_each_proportion{25};
        std::vector<double> proportions_to_dont_care{0.1, 0.2, 0.3};
        acmacs::chart::MinimumColumnBasis minimum_column_basis{};
        enum column_bases_from_master column_bases_from_master{column_bases_from_master::yes};
        enum optimization_precision optimization_precision{optimization_precision::rough};
        enum relax_from_full_table relax_from_full_table{relax_from_full_table::no};
    };

    class MapResoltionTestResults
    {
    };

    MapResoltionTestResults map_resolution_test(ChartModify& chart, const MapResoltionTestParameters& parameters);

} // namespace acmacs::chart

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
