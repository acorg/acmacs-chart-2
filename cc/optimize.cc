#include "acmacs-base/layout.hh"
#include "acmacs-chart-2/optimize.hh"
#include "acmacs-chart-2/stress.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#define AE_COMPILE_MINLBFGS
#include "alglib-3.13.0/optimization.h"
#undef AE_COMPILE_MINLBFGS
#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

static void alglib_lbfgs_optimize(const acmacs::chart::Stress<double>& stress, double* arg_first, double* arg_last);

// ----------------------------------------------------------------------

void acmacs::chart::optimize(OptimizationMethod optimization_method, const Stress<double>& stress, double* arg_first, double* arg_last)
{
    std::cout << "initial stress: " << stress.value(arg_first) << ' ' << *arg_first << '\n';
    switch (optimization_method) {
      case OptimizationMethod::alglib_lbfgs:
          alglib_lbfgs_optimize(stress, arg_first, arg_last);
          break;
    }
    std::cout << "final stress: " << stress.value(arg_first) << ' ' << *arg_first << '\n';

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

static void alglib_lbfgs_optimize_grad(const alglib::real_1d_array& x, double& func, alglib::real_1d_array& grad, void* ptr);

static const char* alglib_lbfgs_optimize_errors[] = {
    "(-1) incorrect parameters were specified", // -1
    "(-2) rounding errors prevent further improvement. X contains best point found.", // -2
    "(-3) unknown error",
    "(-4) unknown error",
    "(-5) unknown error",
    "(-6) unknown error",
    "(-7) gradient verification failed. See MinLBFGSSetGradientCheck() for more information.",
    "(-8) internal integrity control  detected  infinite or NAN values in  function/gradient. Abnormal termination signalled.",
    "unknown error"
};

static const char* alglib_lbfgs_optimize_termination_types[] = {
    "(1) relative function improvement is no more than EpsF.",
    "(2) relative step is no more than EpsX.",
    "(3) unknown termination type",
    "(4) gradient norm is no more than EpsG",
    "(5) Max iteration steps were taken",
    "(6) unknown termination type",
    "(7) stopping conditions are too stringent, further improvement is impossible.",
    "(8) terminated by user who called minlbfgsrequesttermination(). X contains point which was \"current accepted\" when termination request was submitted.",
    "unknown termination type",
};

void alglib_lbfgs_optimize(const acmacs::chart::Stress<double>& stress, double* arg_first, double* arg_last)
{
    using namespace alglib;

    const double epsg = 1e-10;
    const double epsf = 0;
    const double epsx = 0;
    const double stpmax = 0.1;
    const ae_int_t max_iterations = 0;

    real_1d_array x;
    x.attach_to_ptr(arg_last - arg_first, arg_first);

    minlbfgsstate state;
    minlbfgscreate(1, x, state);
    minlbfgssetcond(state, epsg, epsf, epsx, max_iterations);
    minlbfgssetstpmax(state, stpmax);
    minlbfgsoptimize(state, &alglib_lbfgs_optimize_grad, nullptr, const_cast<void*>(reinterpret_cast<const void*>(&stress)));
    minlbfgsreport rep;
    minlbfgsresultsbuf(state, x, rep);

    if (rep.terminationtype < 0) {
        const char* msg = alglib_lbfgs_optimize_errors[std::abs(rep.terminationtype) <= 8 ? (std::abs(rep.terminationtype) - 1) : 8];
        std::cerr << "alglib_lbfgs_optimize error: " << msg << '\n';
        throw acmacs::chart::optimization_error(msg);
    }

    std::cout << "terminationtype: " << alglib_lbfgs_optimize_termination_types[(rep.terminationtype > 0 && rep.terminationtype < 9) ? rep.terminationtype - 1 : 8] << '\n';
    std::cout << "iterations: " << rep.iterationscount << '\n';
    std::cout << "stress calculations: " << rep.nfev << '\n';

} // alglib_lbfgs_optimize

// ----------------------------------------------------------------------

void alglib_lbfgs_optimize_grad(const alglib::real_1d_array& x, double& func, alglib::real_1d_array& grad, void* ptr)
{
    auto* stress = reinterpret_cast<const acmacs::chart::Stress<double>*>(ptr);
    func = stress->value_gradient(x.getcontent(), x.getcontent() + x.length(), grad.getcontent());
      //std::cout << "grad " << ++called << ' ' << func << '\n';

      // terminate optimization (need to pass state in ptr)
      // minlbfgsrequesttermination(state)

} // alglib_lbfgs_optimize_grad

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
