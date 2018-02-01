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

class MatrixJ
{
 public:
    template <typename S> MatrixJ(S size) : size_(cint(size)) {}
    virtual ~MatrixJ() {}

    constexpr aint_t rows() const { return size_; }
    constexpr aint_t cols() const { return size_; }
    virtual double operator()(aint_t row, aint_t col) const = 0;

 private:
    const aint_t size_;
};

class MatrixJProcrustes : public MatrixJ
{
 public:
    template <typename S> MatrixJProcrustes(S size) : MatrixJ(size), diagonal_(1.0 - 1.0 / size), non_diagonal_(-1.0 / size) {}
    double operator()(aint_t row, aint_t column) const override { return row == column ? diagonal_ : non_diagonal_; }

 private:
    const double diagonal_, non_diagonal_;

}; // class MatrixJProcrustes

class MatrixJProcrustesScaling : public MatrixJ
{
 public:
    template <typename S> MatrixJProcrustesScaling(S size)
        : MatrixJ(size),
          diagonal_0_(1.0 - 1.0 / size), non_diagonal_0_(-1.0 / size),
          diagonal_(non_diagonal_0_ * non_diagonal_0_ * (size - 1) + diagonal_0_ * diagonal_0_),
          non_diagonal_(non_diagonal_0_ * non_diagonal_0_ * (size - 2) + non_diagonal_0_ * diagonal_0_ * 2)
        {}
    double operator()(aint_t row, aint_t column) const override { return row == column ? diagonal_ : non_diagonal_; }

 private:
    const double diagonal_0_, non_diagonal_0_;
    const double diagonal_, non_diagonal_;

}; // class MatrixJProcrustesScaling

// ----------------------------------------------------------------------

static alglib::real_2d_array multiply(const MatrixJ& left, const alglib::real_2d_array& right);
static alglib::real_2d_array multiply(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static void multiply(alglib::real_2d_array& matrix, double scale);
static void multiply_add(alglib::real_2d_array& matrix, double scale, const alglib::real_2d_array& to_add);
static alglib::real_2d_array multiply_left_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static alglib::real_2d_array multiply_both_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static alglib::real_2d_array transpose(const alglib::real_2d_array& matrix);
static void singular_value_decomposition(const alglib::real_2d_array& matrix, alglib::real_2d_array& u, alglib::real_2d_array& vt);

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

    ProcrustesData result(number_of_dimensions);
    auto set_transformation = [&result,number_of_dimensions=cint(number_of_dimensions)](const auto& source) {
        for (aint_t row = 0; row < number_of_dimensions; ++row)
            for (aint_t col = 0; col < number_of_dimensions; ++col)
                result.transformation(row, col) = source(row, col);
    };

    alglib::real_2d_array transformation;
    if (scaling == procrustes_scaling_t::no) {
        const MatrixJProcrustes j(common.size());
        auto m4 = transpose(multiply_left_transposed(multiply(j, y), multiply(j, x)));
        alglib::real_2d_array u, vt;
        singular_value_decomposition(m4, u, vt);
        transformation = multiply_both_transposed(vt, u);
    }
    else {
        const MatrixJProcrustesScaling j(common.size());
        const auto m1 = multiply(j, y);
        const auto m2 = multiply_left_transposed(x, m1);
        alglib::real_2d_array u, vt;
        singular_value_decomposition(m2, u, vt);
        transformation = multiply_both_transposed(vt, u);
        // std::cerr << "transformation0: " << transformation.tostring(8) << '\n';

          // calculate optimal scale parameter
        const auto denominator = multiply_left_transposed(y, m1);
        const auto trace_denominator = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(number_of_dimensions)), 0.0, [&denominator](double sum, auto i) { return sum + denominator(i, i); });
        const auto m3 = multiply(y, transformation);
        const auto m4 = multiply(j, m3);
        const auto numerator = multiply_left_transposed(x, m4);
        const auto trace_numerator = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(number_of_dimensions)), 0.0, [&numerator](double sum, auto i) { return sum + numerator(i, i); });
        const auto scale = trace_numerator / trace_denominator;
        multiply(transformation, scale);
        result.scale = scale;
    }
    set_transformation(transformation);

      // translation
    auto m5 = multiply(y, transformation);
    multiply_add(m5, -1, x);
    for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
        const auto t_i = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(common.size())), 0.0, [&m5,dim=cint(dim)](auto sum, auto row) { return sum + m5(row, dim); });
        result.transformation(dim, number_of_dimensions) = t_i / common.size();
    }

    // std::cerr << "common points: " << common.size() << '\n';
    std::cerr << "transformation: " << acmacs::to_string(result.transformation) << '\n';

        // if (aCalculateTranslation) {
        //     result = T->augment(1, 1, 0.0);
        //     result->set(N, N, 1.0);
        //     Intermediate5 = multiplyMatrices(aSecondaryPoints, *T);
        //     multiplyAddMatrix(*Intermediate5, -1, aPrimaryPoints);
        //     for (Index i = 0; i < N; ++i) {
        //         FloatType t_i = 0.0;
        //         for (Index j = 0; j < M; ++j) {
        //             t_i += Intermediate5->get(j, i);
        //         }
        //         result->set(N, i, t_i / FloatType(M));
        //     }
        // }

    return result;

} // acmacs::chart::procrustes

// ----------------------------------------------------------------------

alglib::real_2d_array multiply(const MatrixJ& left, const alglib::real_2d_array& right)
{
    // std::cerr << "multiply left " << left.rows() << 'x' << left.cols() << "  right " << right.rows() << 'x' << right.cols() << '\n';
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

alglib::real_2d_array multiply(const alglib::real_2d_array& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.rows(), right.cols());
    // std::cerr << "multiply left " << left.rows() << 'x' << left.cols() << "  right " << right.rows() << 'x' << right.cols() << "  result " << result.rows() << 'x' << result.cols() << '\n';
    alglib::rmatrixgemm(left.rows(), right.cols(), right.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 0 /*left-no-transform*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 0 /*right-no-transform*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // multiply

// ----------------------------------------------------------------------

void multiply(alglib::real_2d_array& matrix, double scale)
{
    for (aint_t row = 0; row < matrix.rows(); ++row)
        alglib::vmul(&matrix[row][0], 1, matrix.cols(), scale);

} // multiply

// ----------------------------------------------------------------------

void multiply_add(alglib::real_2d_array& matrix, double scale, const alglib::real_2d_array& to_add)
{
    for (aint_t row = 0; row < matrix.rows(); ++row) {
        alglib::vmul(&matrix[row][0], 1, matrix.cols(), scale);
        alglib::vadd(&matrix[row][0], 1, &to_add[row][0], 1, matrix.cols());
    }

} // multiply_add

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

void singular_value_decomposition(const alglib::real_2d_array& matrix, alglib::real_2d_array& u, alglib::real_2d_array& vt)
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
