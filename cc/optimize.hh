#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <chrono>

#include "acmacs-chart-2/optimize-options.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    template <typename Float> class Stress;

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

// ----------------------------------------------------------------------

      // optimizes existing projection without dimension annealing
    optimization_status optimize(const Chart& chart, ProjectionModify& projection, optimization_options options = optimization_options{});
      // optimizes existing projection with dimension annealing
    optimization_status optimize(const Chart& chart, ProjectionModify& projection, const dimension_schedule& schedule, optimization_options options = optimization_options{});
      // creates new projection and optimizes it with or without dimension annealing
    optimization_status optimize(ChartModify& chart, MinimumColumnBasis minimum_column_basis, const dimension_schedule& schedule, optimization_options options = optimization_options{});

    optimization_status optimize(optimization_method optimization_method, const Stress<double>& stress, double* arg_first, double* arg_last, optimization_precision precision = optimization_precision::fine);
    DimensionAnnelingStatus dimension_annealing(optimization_method optimization_method, size_t source_number_of_dimensions, size_t target_number_of_dimensions, double* arg_first, double* arg_last);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
