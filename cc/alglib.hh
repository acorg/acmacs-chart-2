#pragma once

#include "acmacs-base/number-of-dimensions.hh"
#include "acmacs-chart-2/optimization-precision.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    struct optimization_status;
    struct OptimiserCallbackData;
}

// ----------------------------------------------------------------------

namespace alglib
{
    void lbfgs_optimize(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision);
    void cg_optimize(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision);
    void pca(acmacs::chart::OptimiserCallbackData& callback_data, acmacs::number_of_dimensions_t source_number_of_dimensions, acmacs::number_of_dimensions_t target_number_of_dimensions, double* arg_first, double* arg_last);
    void pca_full(acmacs::chart::OptimiserCallbackData& callback_data, acmacs::number_of_dimensions_t number_of_dimensions, double* arg_first, double* arg_last);

} // namespace alglib

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
