#include "acmacs-base/layout.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart-modify.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
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

// ----------------------------------------------------------------------

static void alglib_lbfgs_optimize(acmacs::chart::optimization_status& status, const acmacs::chart::Stress<double>& stress, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision);
static void alglib_cg_optimize(acmacs::chart::optimization_status& status, const acmacs::chart::Stress<double>& stress, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision);
static void alglib_pca(const acmacs::chart::Stress<double>& stress, size_t source_number_of_dimensions, size_t target_number_of_dimensions, double* arg_first, double* arg_last);

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(acmacs::chart::ProjectionModify& projection, acmacs::chart::optimization_options options)
{
    auto layout = projection.layout_modified();
    auto stress = stress_factory<double>(projection, options.mult);
    const auto status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
    return status;

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(ProjectionModify& projection, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_options options)
{
    if (schedule.initial() != projection.number_of_dimensions())
        throw std::runtime_error("acmacs::chart::optimize existing with dimension_schedule: invalid number_of_dimensions in schedule");

    const auto start = std::chrono::high_resolution_clock::now();
    optimization_status status(options.method);
    auto layout = projection.layout_modified();
    auto stress = stress_factory<double>(projection, options.mult);

    bool initial_opt = true;
    for (size_t num_dims: schedule) {
        if (!initial_opt) {
            dimension_annealing(options.method, stress, projection.number_of_dimensions(), num_dims, layout->data(), layout->data() + layout->size());
            layout->change_number_of_dimensions(num_dims);
            stress.change_number_of_dimensions(num_dims);
        }
        const auto sub_status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
        if (initial_opt) {
            status.initial_stress = sub_status.initial_stress;
            status.termination_report = sub_status.termination_report;
        }
        else {
            status.termination_report += "\n" + sub_status.termination_report;
        }
        status.final_stress = sub_status.final_stress;
        status.number_of_iterations += sub_status.number_of_iterations;
        status.number_of_stress_calculations += sub_status.number_of_stress_calculations;
        initial_opt = false;
    }
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return status;

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(acmacs::chart::ChartModify& chart, MinimumColumnBasis minimum_column_basis, const acmacs::chart::dimension_schedule& schedule, acmacs::chart::optimization_options options)
{
    auto projection = chart.projections_modify()->new_from_scratch(schedule.initial(), minimum_column_basis);
    projection->randomize_layout(randomizer_plain_with_table_max_distance(*projection));
    return optimize(*projection, schedule, options);

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

static const char* const s_optimization_method[] = {
    "alglib_lbfgs_pca", "alglib_cg_pca",
};

std::ostream& acmacs::chart::operator<<(std::ostream& out, const acmacs::chart::optimization_status& status)
{
    // out << "stress: " << status.final_stress << " <-- " << status.initial_stress << '\n'
    //     << "termination: " << status.termination_report << '\n'
    //     << "iterations: " << status.number_of_iterations << '\n'
    //     << "stress calculations: " << status.number_of_stress_calculations << '\n'
    //     << "method: " << s_optimization_method[static_cast<size_t>(status.method)] << '\n'
    //     << "time: " << acmacs::format(status.time)
    //         ;

    out << s_optimization_method[static_cast<size_t>(status.method)] << ' ' << std::setprecision(12) << status.final_stress << " <- " << status.initial_stress
        << " time: " << acmacs::format(status.time)
        << " iter: " << status.number_of_iterations
        << " nstress: " << status.number_of_stress_calculations
              // << " term: " << status.termination_report
            ;
    return out;

} // acmacs::chart::operator<<

// ----------------------------------------------------------------------

acmacs::chart::optimization_status acmacs::chart::optimize(optimization_method optimization_method, const Stress<double>& stress, double* arg_first, double* arg_last, optimization_precision precision)
{
    optimization_status status(optimization_method);
    status.initial_stress = stress.value(arg_first);
    const auto start = std::chrono::high_resolution_clock::now();
    try {
        switch (optimization_method) {
            case optimization_method::alglib_lbfgs_pca:
                alglib_lbfgs_optimize(status, stress, arg_first, arg_last, precision);
                break;
            case optimization_method::alglib_cg_pca:
                alglib_cg_optimize(status, stress, arg_first, arg_last, precision);
                break;
        }
    }
    catch (alglib::ap_error& err) {
        throw optimization_error("alglib error: " + err.msg);
    }
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    status.final_stress = stress.value(arg_first);
    return status;

} // acmacs::chart::optimize

// ----------------------------------------------------------------------

acmacs::chart::ErrorLines acmacs::chart::error_lines(const acmacs::chart::Projection& projection)
{
    auto layout = projection.layout();
    auto stress = stress_factory<double>(projection, multiply_antigen_titer_until_column_adjust::yes);
    const auto& table_distances = stress.table_distances();
    const MapDistances map_distances(*layout, table_distances);
    ErrorLines result;
    for (auto td = table_distances.regular().begin(), md = map_distances.regular().begin(); td != table_distances.regular().end(); ++td, ++md) {
        result.emplace_back(td->point_1, td->point_2, td->distance - md->distance);
    }
    for (auto td = table_distances.less_than().begin(), md = map_distances.less_than().begin(); td != table_distances.less_than().end(); ++td, ++md) {
        auto diff = td->distance - md->distance + 1;
        diff *= std::sqrt(acmacs::sigmoid(diff * SigmoidMutiplier<double>())); // see Derek's message Thu, 10 Mar 2016 16:32:20 +0000 (Re: acmacs error line error)
        result.emplace_back(td->point_1, td->point_2, diff);
    }
    return result;

} // acmacs::chart::error_lines

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
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

void alglib_lbfgs_optimize(acmacs::chart::optimization_status& status, const acmacs::chart::Stress<double>& stress, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision)
{
    using namespace alglib;

    const auto [epsg, epsx] = eps(precision);
    const double epsf = 0;
    const double stpmax = 0.1;
    const ae_int_t max_iterations = 0;


      // alglib does not like NaN coordinates of disconnected points, set them to 0
    stress.set_coordinates_of_disconnected(arg_first, 0.0, stress.number_of_dimensions());

    real_1d_array x;
    x.attach_to_ptr(arg_last - arg_first, arg_first);

    minlbfgsstate state;
    minlbfgscreate(1, x, state);
    minlbfgssetcond(state, epsg, epsf, epsx, max_iterations);
    minlbfgssetstpmax(state, stpmax);
    minlbfgsoptimize(state, &alglib_lbfgs_optimize_grad, nullptr, const_cast<void*>(reinterpret_cast<const void*>(&stress)));
    minlbfgsreport rep;
    minlbfgsresultsbuf(state, x, rep);

      // return back NaN for disconnected points
    stress.set_coordinates_of_disconnected(arg_first, std::numeric_limits<double>::quiet_NaN(), stress.number_of_dimensions());

    if (rep.terminationtype < 0) {
        const char* msg = alglib_lbfgs_optimize_errors[std::abs(rep.terminationtype) <= 8 ? (std::abs(rep.terminationtype) - 1) : 8];
        std::cerr << "alglib_lbfgs_optimize error: " << msg << '\n';
        throw acmacs::chart::optimization_error(msg);
    }

    status.termination_report = alglib_lbfgs_optimize_termination_types[(rep.terminationtype > 0 && rep.terminationtype < 9) ? rep.terminationtype - 1 : 8];
    status.number_of_iterations = static_cast<size_t>(rep.iterationscount);
    status.number_of_stress_calculations = static_cast<size_t>(rep.nfev);
    std::cerr << "iter: " << rep.iterationscount << " str: " << rep.nfev << '\n';

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

void alglib_cg_optimize(acmacs::chart::optimization_status& status, const acmacs::chart::Stress<double>& stress, double* arg_first, double* arg_last, acmacs::chart::optimization_precision precision)
{
    using namespace alglib;

    const auto [epsg, epsx] = eps(precision);
    const double epsf = 0;
    const ae_int_t max_iterations = 0;

      // alglib does not like NaN coordinates of disconnected points, set them to 0
    stress.set_coordinates_of_disconnected(arg_first, 0.0, stress.number_of_dimensions());

    real_1d_array x;
    x.attach_to_ptr(arg_last - arg_first, arg_first);

    mincgstate state;
    mincgcreate(x, state);
    mincgsetcond(state, epsg, epsf, epsx, max_iterations);
    mincgoptimize(state, &alglib_lbfgs_optimize_grad, nullptr, const_cast<void*>(reinterpret_cast<const void*>(&stress)));
    mincgreport rep;
    mincgresultsbuf(state, x, rep);

      // return back NaN for disconnected points
    stress.set_coordinates_of_disconnected(arg_first, std::numeric_limits<double>::quiet_NaN(), stress.number_of_dimensions());

    if (rep.terminationtype < 0) {
        const char* msg = alglib_lbfgs_optimize_errors[std::abs(rep.terminationtype) <= 8 ? (std::abs(rep.terminationtype) - 1) : 8];
        std::cerr << "alglib_cg_optimize error: " << msg << '\n';
        throw acmacs::chart::optimization_error(msg);
    }

    status.termination_report = alglib_lbfgs_optimize_termination_types[(rep.terminationtype > 0 && rep.terminationtype < 9) ? rep.terminationtype - 1 : 8];
    status.number_of_iterations = static_cast<size_t>(rep.iterationscount);
    status.number_of_stress_calculations = static_cast<size_t>(rep.nfev);

} // alglib_cg_optimize

// ----------------------------------------------------------------------

acmacs::chart::DimensionAnnelingStatus acmacs::chart::dimension_annealing(optimization_method optimization_method, const Stress<double>& stress, size_t source_number_of_dimensions, size_t target_number_of_dimensions, double* arg_first, double* arg_last)
{
    // std::cerr << "dimension_annealing " << std::pair(arg_first, arg_last) << '\n';
    DimensionAnnelingStatus status;
    const auto start = std::chrono::high_resolution_clock::now();

    switch (optimization_method) {
      case optimization_method::alglib_lbfgs_pca:
      case optimization_method::alglib_cg_pca:
          alglib_pca(stress, source_number_of_dimensions, target_number_of_dimensions, arg_first, arg_last);
          break;
    }

    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return status;

} // acmacs::chart::pca

// ----------------------------------------------------------------------

void alglib_pca(const acmacs::chart::Stress<double>& stress, size_t source_number_of_dimensions, size_t target_number_of_dimensions, double* arg_first, double* arg_last)
{
    const double eps{0};
    const aint_t maxits{0};
    const aint_t number_of_points = (arg_last - arg_first) / cint(source_number_of_dimensions);

      // alglib does not like NaN coordinates of disconnected points, set them to 0
    stress.set_coordinates_of_disconnected(arg_first, 0.0, source_number_of_dimensions);

    alglib::real_2d_array x;
    x.attach_to_ptr(number_of_points, cint(source_number_of_dimensions), arg_first);
    alglib::real_1d_array s2; // output Variance values corresponding to basis vectors.
    s2.setlength(cint(target_number_of_dimensions));
    alglib::real_2d_array v;  // output matrix to transform x to target
    v.setlength(cint(source_number_of_dimensions), cint(target_number_of_dimensions));

    alglib::pcatruncatedsubspace(x, number_of_points, cint(source_number_of_dimensions), cint(target_number_of_dimensions), eps, maxits, s2, v);

      // x * v -> t
      // https://www.tol-project.org/svn/tolp/OfficialTolArchiveNetwork/AlgLib/CppTools/source/alglib/manual.cpp.html#example_ablas_d_gemm
      // https://stackoverflow.com/questions/5607631/matrix-multiplication-alglib
    alglib::real_2d_array t;
    t.setlength(number_of_points, cint(target_number_of_dimensions));
    alglib::rmatrixgemm(number_of_points, cint(target_number_of_dimensions), cint(source_number_of_dimensions), 1.0, x, 0, 0, 0, v, 0, 0, 0, 0, t, 0, 0);

    double* target = arg_first;
    for (aint_t p_no = 0;  p_no < number_of_points; ++p_no) {
        for (aint_t dim_no = 0; dim_no < cint(target_number_of_dimensions); ++dim_no) {
            *target++ = t(p_no, dim_no);
        }
    }

      // return back NaN for disconnected points
      // number of dimensions changed!
    stress.set_coordinates_of_disconnected(arg_first, std::numeric_limits<double>::quiet_NaN(), target_number_of_dimensions);

} // alglib_pca

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
