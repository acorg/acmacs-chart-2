#pragma once

#include "acmacs-chart-2/optimize-options.hh"
#include "acmacs-chart-2/table-distances.hh"
#include "acmacs-chart-2/point-index-list.hh"
#include "acmacs-chart-2/avidity-adjusts.hh"

// ----------------------------------------------------------------------

namespace acmacs { class LayoutInterface; }

namespace acmacs::chart
{
    class Chart;
    class Projection;

    struct StressParameters
    {
        StressParameters(size_t a_number_of_points, PointIndexList&& a_unmovable, PointIndexList&& a_disconnected, PointIndexList&& a_unmovable_in_the_last_dimension, multiply_antigen_titer_until_column_adjust a_mult, AvidityAdjusts&& a_avidity_adjusts, bool a_dodgy_titer_is_regular)
            : number_of_points(a_number_of_points), unmovable(std::move(a_unmovable)), disconnected(std::move(a_disconnected)),
              unmovable_in_the_last_dimension(std::move(a_unmovable_in_the_last_dimension)), mult(a_mult),
              avidity_adjusts(std::move(a_avidity_adjusts)), dodgy_titer_is_regular(a_dodgy_titer_is_regular) {}
        StressParameters(size_t a_number_of_points, multiply_antigen_titer_until_column_adjust a_mult, bool a_dodgy_titer_is_regular)
            : number_of_points(a_number_of_points), mult(a_mult), dodgy_titer_is_regular(a_dodgy_titer_is_regular) {}

        size_t number_of_points;
        PointIndexList unmovable;
        PointIndexList disconnected;
        PointIndexList unmovable_in_the_last_dimension;
        multiply_antigen_titer_until_column_adjust mult;
        AvidityAdjusts avidity_adjusts;
        bool dodgy_titer_is_regular;

    }; // struct StressParameters

    template <typename Float> class Stress
    {
     public:
        using TableDistancesForPoint = typename TableDistances<Float>::EntriesForPoint;

        Stress(const Projection& projection, multiply_antigen_titer_until_column_adjust mult);
        Stress(size_t number_of_dimensions, size_t number_of_points, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular);

        Float value(const Float* first, const Float* /* unused */ = nullptr) const;
        Float value(const acmacs::LayoutInterface& aLayout) const;
        Float contribution(size_t point_no, const Float* first) const;
        Float contribution(size_t point_no, const acmacs::LayoutInterface& aLayout) const;
        Float contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const Float* first) const;
        Float contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const acmacs::LayoutInterface& aLayout) const;
        std::vector<Float> gradient(const Float* first, const Float* last) const;
        void gradient(const Float* first, const Float* last, Float* gradient_first) const;
        Float value_gradient(const Float* first, const Float* last, Float* gradient_first) const;
        std::vector<Float> gradient(const acmacs::LayoutInterface& aLayout) const;
        void change_number_of_dimensions(size_t num_dim) { number_of_dimensions_ = num_dim; }

        const TableDistances<Float>& table_distances() const { return table_distances_; }
        TableDistances<Float>& table_distances() { return table_distances_; }
        TableDistancesForPoint table_distances_for(size_t point_no) const { return TableDistancesForPoint(point_no, table_distances_); }
        const StressParameters& parameters() const { return parameters_; }

        void set_coordinates_of_disconnected(Float* first, Float value) const;

     private:
        size_t number_of_dimensions_;
        TableDistances<Float> table_distances_;
        StressParameters parameters_;

        void gradient_plain(const Float* first, const Float* last, Float* gradient_first) const;
        void gradient_with_unmovable(const Float* first, const Float* last, Float* gradient_first) const;

    }; // class Stress

    template <typename Float> Stress<Float> stress_factory(const Projection& projection, multiply_antigen_titer_until_column_adjust mult);
    template <typename Float> Stress<Float> stress_factory(const acmacs::chart::Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular);

    extern template class Stress<float>;
    extern template class Stress<double>;
#ifndef __clang__
      // g++7 does not like extern template below
#else
      // clang5 wants those externs (otherwise warning -Wundefined-func-template)
    extern template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Projection& projection, multiply_antigen_titer_until_column_adjust mult);
    extern template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Projection& projection, multiply_antigen_titer_until_column_adjust mult);
    extern template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular);
    extern template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular);
#endif

    TableDistances<double> table_distances(const acmacs::chart::Chart& chart, MinimumColumnBasis minimum_column_basis, bool a_dodgy_titer_is_regular);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
