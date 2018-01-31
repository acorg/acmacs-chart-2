#include "acmacs-base/range.hh"
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

class MatrixJProcrustes
{
 public:
    template <typename S> MatrixJProcrustes(S size) : size_(cint(size)), diagonal_(1.0 - 1.0 / size), non_diagonal_(-1.0 / size) {}
    template <typename S> double operator()(S row, S column) const { return row == column ? diagonal_ : non_diagonal_; }
    constexpr aint_t rows() const { return size_; }
    constexpr aint_t cols() const { return size_; }

 private:
    const aint_t size_;
    const double diagonal_, non_diagonal_;

}; // class MatrixJProcrustes

static alglib::real_2d_array multiply(const MatrixJProcrustes& left, const alglib::real_2d_array& right);
static alglib::real_2d_array multiply_left_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static alglib::real_2d_array multiply_both_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static alglib::real_2d_array transpose(const alglib::real_2d_array& matrix);
static void singular_value_decomposition(alglib::real_2d_array& matrix, alglib::real_2d_array& u, alglib::real_2d_array& vt);

// ----------------------------------------------------------------------

// Code for this function was extracted from Procrustes3-for-lisp.c from lispmds

ProcrustesData acmacs::chart::procrustes(const Projection& primary, const Projection& secondary, const std::vector<CommonAntigensSera::common_t>& common, procrustes_scaling_t scaling)
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

    if (scaling == procrustes_scaling_t::no) {
        MatrixJProcrustes j(common.size());
        auto m4 = transpose(multiply_left_transposed(multiply(j, y), multiply(j, x)));
        alglib::real_2d_array u, vt;
        singular_value_decomposition(m4, u, vt);
        const auto transormation = multiply_both_transposed(vt, u);
    }
    else {
        throw std::runtime_error("procrustes with scaling not yet implemented");
    }

    return {number_of_dimensions};

} // acmacs::chart::procrustes

// ----------------------------------------------------------------------

alglib::real_2d_array multiply(const MatrixJProcrustes& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.rows(), right.cols());
    for (aint_t row = 0; row < left.rows(); ++row) {
        for (aint_t column = 0; column < right.cols(); ++column) {
            result(row, column) = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(left.cols()), 0.0,
                                                  [&left,&right,row,column](double sum, auto i) { return sum + left(row, i) * right(i, column); });
        }
    }
    return result;

} // multiply

// ----------------------------------------------------------------------

alglib::real_2d_array multiply_left_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.cols(), right.cols());
    alglib::rmatrixgemm(left.cols(), right.cols(), right.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 1 /*left-transpose*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 0 /*right-no-transform*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // multiply_left_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array multiply_both_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.cols(), right.rows());
    alglib::rmatrixgemm(left.cols(), right.rows(), left.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 1 /*left-transpose*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 1 /*right-transpose*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // multiply_both_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array transpose(const alglib::real_2d_array& source)
{
    alglib::real_2d_array result;
    result.setlength(source.cols(), source.rows());
    alglib::rmatrixtranspose(source.rows(), source.cols(), source, 0/*i-source*/, 0/*j-source*/, result, 0/*i-result*/, 0/*j-result*/);
      // std::cerr << "transposed: " << source.rows() << 'x' << source.cols() << " --> " << source.cols() << 'x' << source.rows() << '\n';
    return result;

} // transpose

// ----------------------------------------------------------------------

void singular_value_decomposition(alglib::real_2d_array& matrix, alglib::real_2d_array& u, alglib::real_2d_array& vt)
{
    vt.setlength(matrix.cols(), matrix.cols());
    u.setlength(matrix.rows(), matrix.rows());
    alglib::real_1d_array w;
    w.setlength(matrix.cols());
    alglib::rmatrixsvd(matrix, matrix.rows(), matrix.cols(), 2/*u-needed*/, 2/*vt-needed*/, 2 /*additionalmemory -> max performance*/, w, u, vt);

} // singular_value_decomposition

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
