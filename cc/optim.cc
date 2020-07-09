#include "acmacs-chart-2/optim.hh"
#include "acmacs-chart-2/optimize.hh"
#include "acmacs-chart-2/stress.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wshadow-field"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wnewline-eof"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wunused-template"
#endif

#include "optim/optim.hpp"

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

static double objective_function(const arma::vec& vals_inp, arma::vec* grad_out, void* opt_data);

// ----------------------------------------------------------------------


void optim::bfgs(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision)
{
    algo_settings_t settings;
    arma::vec vals(static_cast<arma::uword>(arg_last - arg_first));
    std::copy(arg_first, arg_first, vals.begin());
    if (bfgs(vals, &objective_function, reinterpret_cast<void*>(&callback_data), settings)) {
    }
    else
        throw acmacs::chart::optimization_error("optim::bfgs failed");

} // optim::bfgs

// ----------------------------------------------------------------------

double objective_function(const arma::vec& vals_inp, arma::vec* grad_out, void* opt_data)
{
    auto* callback_data = reinterpret_cast<acmacs::chart::OptimiserCallbackData*>(opt_data);
    if (grad_out)
        return callback_data->stress.value_gradient(vals_inp.begin(), vals_inp.end(), grad_out->begin());
    else
        return callback_data->stress.value(vals_inp.begin());

} // objective_function

// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
