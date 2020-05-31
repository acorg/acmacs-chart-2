#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <chrono>

#include "acmacs-base/layout.hh"
#include "acmacs-chart-2/optimize-options.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Stress;

    class optimization_error : public std::runtime_error { public: inline optimization_error(std::string msg) : std::runtime_error("invalid_data: " + msg) {} };

    class ChartModify;
    class ProjectionModify;

// ----------------------------------------------------------------------

    struct optimization_status
    {
        optimization_status(optimization_method a_method) : method{a_method} {}
        optimization_method method;
        size_t number_of_iterations = 0;
        size_t number_of_stress_calculations = 0;
        std::string termination_report;
        std::chrono::microseconds time;
        double initial_stress;
        double final_stress;

    }; // struct optimization_status

    std::ostream& operator<<(std::ostream& out, const optimization_status& status);

    struct DimensionAnnelingStatus
    {
        std::chrono::microseconds time;
    };

    struct IntermediateLayout
    {
        IntermediateLayout(number_of_dimensions_t number_of_dimensions, const double* first, long layout_size, double a_stress) : layout(number_of_dimensions, first, first + layout_size), stress{a_stress} {}
        const acmacs::Layout layout;
        const double stress;
    };

    using IntermediateLayouts = std::vector<IntermediateLayout>;

// ----------------------------------------------------------------------

      // optimizes existing projection without dimension annealing
    optimization_status optimize(ProjectionModify& projection, optimization_options options = optimization_options{});
      // optimizes existing projection without dimension annealing and saves intermediate layouts
    optimization_status optimize(ProjectionModify& projection, IntermediateLayouts& intermediate_layouts, optimization_options options = optimization_options{});
      // optimizes existing projection with dimension annealing
    optimization_status optimize(ProjectionModify& projection, const dimension_schedule& schedule, optimization_options options = optimization_options{});
      // creates new projection and optimizes it with or without dimension annealing
    optimization_status optimize(ChartModify& chart, MinimumColumnBasis minimum_column_basis, const dimension_schedule& schedule, optimization_options options = optimization_options{});

    optimization_status optimize(optimization_method method, const Stress& stress, double* arg_first, double* arg_last, optimization_precision precision = optimization_precision::fine);
    inline optimization_status optimize(optimization_method method, const Stress& stress, double* arg_first, size_t arg_size, optimization_precision precision = optimization_precision::fine)
    {
        return optimize(method, stress, arg_first, arg_first + arg_size, precision);
    }

    DimensionAnnelingStatus dimension_annealing(optimization_method optimization_method, const Stress& stress, number_of_dimensions_t source_number_of_dimensions, number_of_dimensions_t target_number_of_dimensions, double* arg_first, double* arg_last);

    // replaces layout in (arg_first, arg_last)
    void pca(const Stress& stress, number_of_dimensions_t number_of_dimensions, double* arg_first, double* arg_last);

// ----------------------------------------------------------------------

    struct ErrorLine
    {
        ErrorLine(size_t p1, size_t p2, double el) : point_1{p1}, point_2{p2}, error_line{el} {}
        size_t point_1, point_2;
        double error_line;      // positive: table_dist > map_dist, i.e. draw line in direction opposite to corresponding point

    }; // struct ErrorLine

    using ErrorLines = std::vector<ErrorLine>;

    ErrorLines error_lines(const Projection& projection);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
