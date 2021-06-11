#pragma once

#include "acmacs-base/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class ChartModify;
    class ProjectionModify;
    struct optimization_options;

    namespace avidity
    {
        struct MostMoved
        {
            size_t antigen_no;
            double distance;
        };

        constexpr const size_t number_of_most_moved_antigens{5};

        struct PerAdjust
        {
            double logged_adjust;
            double angle_test_antigen;
            double average_procrustes_distances_except_test_antigen;
            double distance_test_antigen;
            PointCoordinates final;
            double stress_diff;
            std::array<MostMoved, number_of_most_moved_antigens> most_moved;
        };

        struct Result
        {
            // Result(size_t a_antigen_no, PointCoordinates) : antigen_no{a_antigen_no} {}

            size_t antigen_no;
            double best_logged_adjust;
            PointCoordinates original;
            std::vector<PerAdjust> adjusts;
        };

        struct Results
        {
            double original_stress;
            std::vector<Result> results;
        };

        PerAdjust test(const ChartModify& chart, const ProjectionModify& original_projection, size_t antigen_no, double logged_adjust, const optimization_options& options);

    } // namespace avidity
} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
