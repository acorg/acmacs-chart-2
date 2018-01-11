#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <chrono>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    template <typename Float> class Stress;

    enum class OptimizationMethod { alglib_lbfgs };

    class optimization_error : public std::runtime_error { public: inline optimization_error(std::string msg) : std::runtime_error("invalid_data: " + msg) {} };

    class OptimizationStatus
    {
     public:
        OptimizationMethod method;
        size_t number_of_iterations;
        size_t number_of_stress_calculations;
        std::string termination_report;
        std::chrono::microseconds time;
        double initial_stress;
        double final_stress;

    }; // class OptimizationStatus

    std::ostream& operator<<(std::ostream& out, const OptimizationStatus& status);

    OptimizationStatus optimize(OptimizationMethod optimization_method, const Stress<double>& stress, double* arg_first, double* arg_last);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
