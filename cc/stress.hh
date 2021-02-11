#pragma once

#include "acmacs-chart-2/optimize-options.hh"
#include "acmacs-chart-2/table-distances.hh"
#include "acmacs-chart-2/point-index-list.hh"
#include "acmacs-chart-2/avidity-adjusts.hh"

// ----------------------------------------------------------------------

namespace acmacs { class Layout; }

namespace acmacs::chart
{
    class Chart;
    class Projection;

    struct StressParameters
    {
        StressParameters(size_t a_number_of_points, UnmovablePoints&& a_unmovable, DisconnectedPoints&& a_disconnected, UnmovableInTheLastDimensionPoints&& a_unmovable_in_the_last_dimension, multiply_antigen_titer_until_column_adjust a_mult, AvidityAdjusts&& a_avidity_adjusts, dodgy_titer_is_regular a_dodgy_titer_is_regular)
            : number_of_points(a_number_of_points), unmovable(std::move(a_unmovable)), disconnected(std::move(a_disconnected)),
              unmovable_in_the_last_dimension(std::move(a_unmovable_in_the_last_dimension)), mult(a_mult),
              avidity_adjusts(std::move(a_avidity_adjusts)), dodgy_titer_is_regular(a_dodgy_titer_is_regular) {}
        StressParameters(size_t a_number_of_points, multiply_antigen_titer_until_column_adjust a_mult, dodgy_titer_is_regular a_dodgy_titer_is_regular)
            : number_of_points(a_number_of_points), mult(a_mult), dodgy_titer_is_regular(a_dodgy_titer_is_regular) {}
        StressParameters(size_t a_number_of_points)
            : number_of_points(a_number_of_points) {}

        size_t number_of_points;
        UnmovablePoints unmovable;
        DisconnectedPoints disconnected;
        UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension;
        multiply_antigen_titer_until_column_adjust mult{multiply_antigen_titer_until_column_adjust::yes};
        AvidityAdjusts avidity_adjusts;
        enum dodgy_titer_is_regular dodgy_titer_is_regular{dodgy_titer_is_regular::no};

    }; // struct StressParameters

    class Stress
    {
     public:
        using TableDistancesForPoint = typename TableDistances::EntriesForPoint;

        Stress(const Projection& projection, multiply_antigen_titer_until_column_adjust mult);
        Stress(number_of_dimensions_t number_of_dimensions, size_t number_of_points, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular a_dodgy_titer_is_regular);
        Stress(number_of_dimensions_t number_of_dimensions, size_t number_of_points);

        double value(const double* first, const double* /* unused */ = nullptr) const;
        double value(const acmacs::Layout& aLayout) const;
        double contribution(size_t point_no, const double* first) const;
        double contribution(size_t point_no, const acmacs::Layout& aLayout) const;
        double contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const double* first) const;
        double contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const acmacs::Layout& aLayout) const;
        std::vector<double> gradient(const double* first, const double* last) const;
        void gradient(const double* first, const double* last, double* gradient_first) const;
        double value_gradient(const double* first, const double* last, double* gradient_first) const;
        std::vector<double> gradient(const acmacs::Layout& aLayout) const;
        constexpr auto number_of_dimensions() const { return number_of_dimensions_; }
        void change_number_of_dimensions(number_of_dimensions_t num_dim) { number_of_dimensions_ = num_dim; }

        constexpr const TableDistances& table_distances() const { return table_distances_; }
        constexpr TableDistances& table_distances() { return table_distances_; }
        TableDistancesForPoint table_distances_for(size_t point_no) const { return TableDistancesForPoint(point_no, table_distances_); }
        constexpr const StressParameters& parameters() const { return parameters_; }
        constexpr StressParameters& parameters() { return parameters_; }
        void set_disconnected(const DisconnectedPoints& to_disconnect) { parameters_.disconnected = to_disconnect; }
        void extend_disconnected(const PointIndexList& to_disconnect) { parameters_.disconnected.extend(to_disconnect); }
        void set_unmovable(const UnmovablePoints& unmovable) { parameters_.unmovable = unmovable; }
        void set_unmovable_in_the_last_dimension(const UnmovableInTheLastDimensionPoints& unmovable_in_the_last_dimension) { parameters_.unmovable_in_the_last_dimension = unmovable_in_the_last_dimension; }

        void set_coordinates_of_disconnected(double* first, size_t num_args, double value, number_of_dimensions_t number_of_dimensions) const;

     private:
        number_of_dimensions_t number_of_dimensions_;
        TableDistances table_distances_;
        StressParameters parameters_;

        void gradient_plain(const double* first, const double* last, double* gradient_first) const;
        void gradient_with_unmovable(const double* first, const double* last, double* gradient_first) const;

    }; // class Stress

    Stress stress_factory(const Projection& projection, multiply_antigen_titer_until_column_adjust mult);
    Stress stress_factory(const Chart& chart, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular a_dodgy_titer_is_regular = dodgy_titer_is_regular::no);

    TableDistances table_distances(const acmacs::chart::Chart& chart, MinimumColumnBasis minimum_column_basis, dodgy_titer_is_regular a_dodgy_titer_is_regular = dodgy_titer_is_regular::no);

    constexpr inline double SigmoidMutiplier() { return 10.0; }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
