#pragma once

#include "acmacs-chart-2/optimization-precision.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    struct optimization_status;
    struct OptimiserCallbackData;
}

// ----------------------------------------------------------------------

namespace optim
{
    void bfgs(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
