#pragma once

#include <stdexcept>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    template <typename Float> class Stress;

    enum class OptimizationMethod { alglib_lbfgs };

    class optimization_error : public std::runtime_error { public: inline optimization_error(std::string msg) : std::runtime_error("invalid_data: " + msg) {} };

    void optimize(OptimizationMethod optimization_method, const Stress<double>& stress, double* arg_first, double* arg_last);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
