#include "acmacs-base/fmt.hh"
#include "acmacs-chart-2/alglib.hh"
#include "acmacs-chart-2/optimize.hh"
#include "acmacs-chart-2/stress.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
#endif
#define AE_COMPILE_MINLBFGS
#define AE_COMPILE_MINCG
#include "alglib-3.13.0/optimization.h"
// rmatrixgemm()
#include "alglib-3.13.0/linalg.h"
#undef AE_COMPILE_MINLBFGS
#undef AE_COMPILE_MINCG

#define AE_COMPILE_PCA
#include "alglib-3.13.0/dataanalysis.h"
#undef AE_COMPILE_PCA

#pragma GCC diagnostic pop

using aint_t = alglib::ae_int_t;
template <typename T> constexpr inline aint_t cint(T src) { return static_cast<aint_t>(src); };
constexpr inline aint_t cint(acmacs::number_of_dimensions_t src) { return static_cast<aint_t>(*src); };

// ----------------------------------------------------------------------

namespace alglib
{
    static void lbfgs_optimize_grad(const alglib::real_1d_array& x, double& func, alglib::real_1d_array& grad, void* ptr);
    static void lbfgs_optimize_step(const alglib::real_1d_array& x, double func, void* ptr); // callback at each iteration

    static constexpr std::array lbfgs_optimize_errors =
    {
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

    static constexpr std::array lbfgs_optimize_termination_types =
    {
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

}

// ----------------------------------------------------------------------

inline std::pair<double, double> eps(acmacs::chart::optimization_precision precision)
{
    switch (precision) {
      case acmacs::chart::optimization_precision::rough:
          return {0.5, 1e-3};
      case acmacs::chart::optimization_precision::very_rough:
          return {1.0, 0.1};
      case acmacs::chart::optimization_precision::fine:
          return {1e-10, 0.0};
    }
    return {1e-10, 0.0};
}

// ----------------------------------------------------------------------

void alglib::lbfgs_optimize(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last,
                            acmacs::chart::optimization_precision precision)
{
    try {
        const auto [epsg, epsx] = eps(precision);
        const double epsf = 0;
        const double stpmax = 0.1;
        const ae_int_t max_iterations = 0;

        real_1d_array x;
        x.attach_to_ptr(arg_last - arg_first, arg_first);

        minlbfgsstate state;
        minlbfgscreate(1, x, state);
        minlbfgssetcond(state, epsg, epsf, epsx, max_iterations);
        minlbfgssetstpmax(state, stpmax);
        minlbfgssetxrep(state, callback_data.intermediate_layouts != nullptr);
        minlbfgsoptimize(state, &lbfgs_optimize_grad, &lbfgs_optimize_step, reinterpret_cast<void*>(&callback_data));
        minlbfgsreport rep;
        minlbfgsresultsbuf(state, x, rep);

        if (rep.terminationtype < 0) {
            const char* msg = lbfgs_optimize_errors[static_cast<size_t>(std::abs(rep.terminationtype) <= 8 ? (std::abs(rep.terminationtype) - 1) : 8)];
            fmt::print(stderr, "> ERROR alglib_lbfgs_optimize: {}\n", msg);
            throw acmacs::chart::optimization_error(msg);
        }

        status.termination_report = lbfgs_optimize_termination_types[static_cast<size_t>((rep.terminationtype > 0 && rep.terminationtype < 9) ? rep.terminationtype - 1 : 8)];
        status.number_of_iterations = static_cast<size_t>(rep.iterationscount);
        status.number_of_stress_calculations = static_cast<size_t>(rep.nfev);
    }
    catch (ap_error& err) {
        throw acmacs::chart::optimization_error(fmt::format("alglib error: {}", err.msg));
    }

} // alglib::lbfgs_optimize

// ----------------------------------------------------------------------

void alglib::lbfgs_optimize_grad(const alglib::real_1d_array& x, double& func, alglib::real_1d_array& grad, void* ptr)
{
    auto* callback_data = reinterpret_cast<acmacs::chart::OptimiserCallbackData*>(ptr);
    func = callback_data->stress.value_gradient(x.getcontent(), x.getcontent() + x.length(), grad.getcontent());
      //std::cout << "grad " << ++called << ' ' << func << '\n';

      // terminate optimization (need to pass state in ptr)
      // minlbfgsrequesttermination(state)

} // alglib::lbfgs_optimize_grad

// ----------------------------------------------------------------------

void alglib::lbfgs_optimize_step(const alglib::real_1d_array& x, double func, void* ptr) // callback at each iteration
{
    auto* callback_data = reinterpret_cast<acmacs::chart::OptimiserCallbackData*>(ptr);
    callback_data->intermediate_layouts->emplace_back(callback_data->stress.number_of_dimensions(), x.getcontent(), x.length(), func);

} // alglib::lbfgs_optimize_step

// ----------------------------------------------------------------------

void alglib::cg_optimize(acmacs::chart::optimization_status& status, acmacs::chart::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last,
                         acmacs::chart::optimization_precision precision)
{
    try {
        const auto [epsg, epsx] = eps(precision);
        const double epsf = 0;
        const ae_int_t max_iterations = 0;

        real_1d_array x;
        x.attach_to_ptr(arg_last - arg_first, arg_first);

        mincgstate state;
        mincgcreate(x, state);
        mincgsetcond(state, epsg, epsf, epsx, max_iterations);
        mincgsetxrep(state, callback_data.intermediate_layouts != nullptr);
        mincgoptimize(state, &lbfgs_optimize_grad, &lbfgs_optimize_step, reinterpret_cast<void*>(&callback_data));
        mincgreport rep;
        mincgresultsbuf(state, x, rep);

        if (rep.terminationtype < 0) {
            const char* msg = lbfgs_optimize_errors[static_cast<size_t>(std::abs(rep.terminationtype) <= 8 ? (std::abs(rep.terminationtype) - 1) : 8)];
            fmt::print(stderr, "> ERROR alglib_cg_optimize: {}\n", msg);
            throw acmacs::chart::optimization_error(msg);
        }

        status.termination_report = lbfgs_optimize_termination_types[static_cast<size_t>((rep.terminationtype > 0 && rep.terminationtype < 9) ? rep.terminationtype - 1 : 8)];
        status.number_of_iterations = static_cast<size_t>(rep.iterationscount);
        status.number_of_stress_calculations = static_cast<size_t>(rep.nfev);
    }
    catch (ap_error& err) {
        throw acmacs::chart::optimization_error(fmt::format("alglib error: {}", err.msg));
    }

} // alglib::cg_optimize

// ----------------------------------------------------------------------

void alglib::pca(acmacs::chart::OptimiserCallbackData& callback_data, acmacs::number_of_dimensions_t source_number_of_dimensions, acmacs::number_of_dimensions_t target_number_of_dimensions,
                 double* arg_first, double* arg_last)
{
    try {
        const double eps{0};
        const aint_t maxits{0};
        const aint_t number_of_points = (arg_last - arg_first) / cint(source_number_of_dimensions);

        // alglib does not like NaN coordinates of disconnected points, set them to 0
        callback_data.stress.set_coordinates_of_disconnected(arg_first, 0.0, source_number_of_dimensions);

        alglib::real_2d_array x;
        x.attach_to_ptr(number_of_points, cint(source_number_of_dimensions), arg_first);
        alglib::real_1d_array s2; // output Variance values corresponding to basis vectors.
        s2.setlength(cint(target_number_of_dimensions));
        alglib::real_2d_array v; // output matrix to transform x to target
        v.setlength(cint(source_number_of_dimensions), cint(target_number_of_dimensions));

        alglib::pcatruncatedsubspace(x, number_of_points, cint(source_number_of_dimensions), cint(target_number_of_dimensions), eps, maxits, s2, v);

        // x * v -> t
        // https://www.tol-project.org/svn/tolp/OfficialTolArchiveNetwork/AlgLib/CppTools/source/alglib/manual.cpp.html#example_ablas_d_gemm
        // https://stackoverflow.com/questions/5607631/matrix-multiplication-alglib
        alglib::real_2d_array t;
        t.setlength(number_of_points, cint(target_number_of_dimensions));
        alglib::rmatrixgemm(number_of_points, cint(target_number_of_dimensions), cint(source_number_of_dimensions), 1.0, x, 0, 0, 0, v, 0, 0, 0, 0, t, 0, 0);

        double* target = arg_first;
        for (aint_t p_no = 0; p_no < number_of_points; ++p_no) {
            for (aint_t dim_no = 0; dim_no < cint(target_number_of_dimensions); ++dim_no) {
                *target++ = t(p_no, dim_no);
            }
        }

        // return back NaN for disconnected points
        // number of dimensions changed!
        callback_data.stress.set_coordinates_of_disconnected(arg_first, std::numeric_limits<double>::quiet_NaN(), target_number_of_dimensions);
    }
    catch (ap_error& err) {
        AD_ERROR("alglib::pca: {}", err.msg);
        throw acmacs::chart::optimization_error(fmt::format("alglib::pca error: {}", err.msg));
    }

} // alglib::pca

// ----------------------------------------------------------------------

void alglib::pca_full(acmacs::chart::OptimiserCallbackData& callback_data, acmacs::number_of_dimensions_t number_of_dimensions, double* arg_first, double* arg_last)
{
    try {
        const aint_t number_of_points = (arg_last - arg_first) / cint(number_of_dimensions);

        // alglib does not like NaN coordinates of disconnected points, set them to 0
        callback_data.stress.set_coordinates_of_disconnected(arg_first, 0.0, number_of_dimensions);

        alglib::real_2d_array x;
        x.attach_to_ptr(number_of_points, cint(number_of_dimensions), arg_first);
        alglib::real_1d_array s2; // output Variance values corresponding to basis vectors.
        s2.setlength(cint(number_of_dimensions));
        alglib::real_2d_array v; // output matrix to transform x to target
        v.setlength(cint(number_of_dimensions), cint(number_of_dimensions));

        aint_t info{0}; // -4, if SVD subroutine haven't converged; -1, if wrong parameters has been passed (NPoints<0, NVars<1); 1, if task is solved
        alglib::pcabuildbasis(x, number_of_points, cint(number_of_dimensions), // input
                              info, s2, v);                                    // output
        switch (info) {
            case -4:
                throw std::runtime_error{"alglib pca failed: SVD subroutine haven't converged"};
            case -1:
                throw std::runtime_error{"alglib pca failed: wrong parameters passed (NPoints<0, NVars<1)"};
            case 1: // good
                break;
            default:
                throw std::runtime_error{fmt::format("alglib pca failed: unknown error {}", info)};
        }

        // x * v -> t
        // https://www.tol-project.org/svn/tolp/OfficialTolArchiveNetwork/AlgLib/CppTools/source/alglib/manual.cpp.html#example_ablas_d_gemm
        // https://stackoverflow.com/questions/5607631/matrix-multiplication-alglib
        alglib::real_2d_array t;
        t.setlength(number_of_points, cint(number_of_dimensions));
        alglib::rmatrixgemm(number_of_points, cint(number_of_dimensions), cint(number_of_dimensions), 1.0, x, 0, 0, 0, v, 0, 0, 0, 0, t, 0, 0);

        double* target = arg_first;
        for (aint_t p_no = 0; p_no < number_of_points; ++p_no) {
            for (aint_t dim_no = 0; dim_no < cint(number_of_dimensions); ++dim_no) {
                *target++ = t(p_no, dim_no);
            }
        }

        // return back NaN for disconnected points
        // number of dimensions changed!
        callback_data.stress.set_coordinates_of_disconnected(arg_first, std::numeric_limits<double>::quiet_NaN(), number_of_dimensions);
    }
    catch (ap_error& err) {
        AD_ERROR("alglib::pca_full: {}", err.msg);
        throw acmacs::chart::optimization_error(fmt::format("alglib::pca_full error: {}", err.msg));
    }

} // alglib::pca_full

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
