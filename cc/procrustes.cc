#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/chart.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
// #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#define AE_COMPILE_SVD
#include "alglib-3.13.0/linalg.h"
#undef AE_COMPILE_SVD

#pragma GCC diagnostic pop

using namespace acmacs::chart;

using aint_t = alglib::ae_int_t;
template <typename T> constexpr inline aint_t cint(T src) { return static_cast<aint_t>(src); };

// ----------------------------------------------------------------------

ProcrustesData acmacs::chart::procrustes(const Projection& primary, const Projection& secondary, const std::vector<CommonAntigensSera::common_t>& common)
{
    auto primary_layout = primary.layout();
    auto secondary_layout = secondary.layout();
    const auto number_of_dimensions = primary_layout->number_of_dimensions();
    if (number_of_dimensions != secondary_layout->number_of_dimensions())
        throw invalid_data("procrustes: projections have different number of dimensions");

    alglib::real_2d_array x, y;
    x.setlength(cint(common.size()), cint(number_of_dimensions));
    y.setlength(cint(common.size()), cint(number_of_dimensions));
    for (size_t point_no = 0; point_no < common.size(); ++point_no) {
        for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
            const auto x_v = primary_layout->coordinate(common[point_no].primary, dim);
            x(cint(point_no), cint(dim)) = x_v;
            const auto y_v = secondary_layout->coordinate(common[point_no].secondary, dim);
            y(cint(point_no), cint(dim)) = y_v;
        }
    }

    // const auto common_points = common.points();
    // acmacs::Layout x(common_points.size(), number_of_dimensions);
    // acmacs::Layout y(common_points.size(), number_of_dimensions);
    // size_t index = 0;
    // std::vector<double> x_mean(number_of_dimensions, 0), y_mean(number_of_dimensions, 0);
    // for (const auto& cp: common_points) {
    //     for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
    //         const auto x_v = primary_layout->coordinate(cp.primary, dim);
    //         x.set(index, dim, x_v);
    //         x_mean[dim] += x_v;
    //         const auto y_v = secondary_layout->coordinate(cp.secondary, dim);
    //         y.set(index, dim, y_v);
    //         y_mean[dim] += y_v;
    //     }
    //     ++index;
    // }
    // for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
    //     x_mean[dim] /= common_points.size();
    //     y_mean[dim] /= common_points.size();
    // }
    // for (auto p = x.begin(); p != x.end(); p += number_of_dimensions) {
    // }

} // acmacs::chart::procrustes

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
