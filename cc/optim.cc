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

#else

#pragma GCC diagnostic ignored "-Wunused-parameter"

#endif

#include "optim/optim.hpp"

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

static double objective_function(const arma::vec& vals_inp, arma::vec* grad_out, void* opt_data);

// ----------------------------------------------------------------------

double objective_function(const arma::vec& vals_inp, arma::vec* grad_out, void* opt_data)
{
    auto* callback_data = reinterpret_cast<acmacs::chart::OptimiserCallbackData*>(opt_data);
    ++callback_data->iteration_no;
    if (grad_out)
        return callback_data->stress.value_gradient(vals_inp.begin(), vals_inp.end(), grad_out->begin());
    else
        return callback_data->stress.value(vals_inp.begin());

} // objective_function

// ----------------------------------------------------------------------

void optim::bfgs(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision /*precision*/)
{
    algo_settings_t settings;
    arma::vec vals(static_cast<arma::uword>(arg_last - arg_first));
    std::copy(arg_first, arg_last, vals.begin());
    callback_data.iteration_no = 0;
    if (bfgs(vals, &objective_function, reinterpret_cast<void*>(&callback_data), settings)) {
        std::copy(vals.begin(), vals.end(), arg_first);

        status.number_of_iterations = callback_data.iteration_no;
        status.number_of_stress_calculations = callback_data.iteration_no;
    }
    else
        throw acmacs::chart::optimization_error("optim::bfgs failed");

} // optim::bfgs

// ----------------------------------------------------------------------

void optim::differential_evolution(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, acmacs::chart::optimization_precision /*precision*/)
{
    // algo_settings_t settings{.de_n_pop=200, .de_n_pop_best=6, .de_n_gen=10000, .de_mutation_method=2, .de_par_F=0.8, .de_par_CR=0.9};
    algo_settings_t settings{.de_n_pop=2000, .de_n_pop_best=6, .de_n_gen=100000, .de_mutation_method=2, .de_par_F=0.8, .de_par_CR=0.9};
    arma::vec vals(static_cast<arma::uword>(arg_last - arg_first));
    std::copy(arg_first, arg_last, vals.begin());
    callback_data.iteration_no = 0;
    if (de_prmm(vals, &objective_function, reinterpret_cast<void*>(&callback_data), settings)) {
        std::copy(vals.begin(), vals.end(), arg_first);

        status.number_of_iterations = callback_data.iteration_no;
        status.number_of_stress_calculations = callback_data.iteration_no;
    }
    else
        throw acmacs::chart::optimization_error("optim::differential_evolution failed");

} // optim::differential_evolution

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
