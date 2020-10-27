#include "acmacs-base/range.hh"
#include "acmacs-base/vector-math.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

// std::transform_reduce is not in g++-7.2 (nor in g++-8, see Parallelism TS in https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017)
#if !defined(__clang__) && __GNUC__ < 9
namespace std
{
      // extracted from clang5 lib: /usr/local/opt/llvm/include/c++/v1/numeric

    template <class _InputIterator, class _Tp, class _BinaryOp, class _UnaryOp>
        inline _Tp transform_reduce(_InputIterator __first, _InputIterator __last, _Tp __init,  _BinaryOp __b, _UnaryOp __u)
    {
        for (; __first != __last; ++__first)
            __init = __b(__init, __u(*__first));
        return __init;
    }

    template <class _InputIterator1, class _InputIterator2, class _Tp, class _BinaryOp1, class _BinaryOp2>
        inline _Tp transform_reduce(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2, _Tp __init,  _BinaryOp1 __b1, _BinaryOp2 __b2)
    {
        for (; __first1 != __last1; ++__first1, (void) ++__first2)
            __init = __b1(__init, __b2(*__first1, *__first2));
        return __init;
    }
}
#endif

// ----------------------------------------------------------------------

acmacs::chart::Stress acmacs::chart::stress_factory(const acmacs::chart::Projection& projection, acmacs::chart::multiply_antigen_titer_until_column_adjust mult)
{
    Stress stress(projection, mult);
    auto cb = projection.forced_column_bases();
    if (!cb)
        cb = projection.chart().column_bases(projection.minimum_column_basis());
    projection.chart().titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // acmacs::chart::stress_factory

// ----------------------------------------------------------------------

acmacs::chart::Stress acmacs::chart::stress_factory(const acmacs::chart::Chart& chart, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular a_dodgy_titer_is_regular)
{
    Stress stress(number_of_dimensions, chart.number_of_points(), mult, a_dodgy_titer_is_regular);
    auto cb = chart.forced_column_bases(minimum_column_basis);
    if (!cb)
        cb = chart.column_bases(minimum_column_basis);
    chart.titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // acmacs::chart::stress_factory

// ----------------------------------------------------------------------

acmacs::chart::TableDistances acmacs::chart::table_distances(const acmacs::chart::Chart& chart, MinimumColumnBasis minimum_column_basis, dodgy_titer_is_regular a_dodgy_titer_is_regular)
{
    Stress stress(number_of_dimensions_t{2}, chart.number_of_points(), multiply_antigen_titer_until_column_adjust::yes, a_dodgy_titer_is_regular);
    auto cb = chart.forced_column_bases(minimum_column_basis);
    if (!cb)
        cb = chart.column_bases(minimum_column_basis);
    return chart.titers()->table_distances(*cb, stress.parameters());

} // acmacs::chart::table_distances

// ----------------------------------------------------------------------

constexpr inline double non_zero(double value) { return float_zero(value) ? 1e-5 : value; };

// ----------------------------------------------------------------------

static inline double map_distance(const double* first, size_t point_1, size_t point_2, acmacs::number_of_dimensions_t number_of_dimensions)
{
    using diff_t = typename std::vector<double>::difference_type;
    return acmacs::vector_math::distance(
        first + static_cast<diff_t>(static_cast<size_t>(number_of_dimensions) * point_1),
        first + static_cast<diff_t>(static_cast<size_t>(number_of_dimensions) * (point_1 + 1)),
        first + static_cast<diff_t>(static_cast<size_t>(number_of_dimensions) * point_2));

} // map_distance

// ----------------------------------------------------------------------

static inline double map_distance(const double* first, const typename acmacs::chart::TableDistances::Entry& entry, acmacs::number_of_dimensions_t number_of_dimensions)
{
    return map_distance(first, entry.point_1, entry.point_2, number_of_dimensions);

} // map_distance

// ----------------------------------------------------------------------

acmacs::chart::Stress::Stress(const Projection& projection, acmacs::chart::multiply_antigen_titer_until_column_adjust mult)
    : number_of_dimensions_(projection.number_of_dimensions()),
      parameters_(projection.number_of_points(), projection.unmovable(), projection.disconnected(), projection.unmovable_in_the_last_dimension(),
                  mult, projection.avidity_adjusts(), projection.dodgy_titer_is_regular())
{
} // acmacs::chart::Stress::Stress

// ----------------------------------------------------------------------

acmacs::chart::Stress::Stress(number_of_dimensions_t number_of_dimensions, size_t number_of_points, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular a_dodgy_titer_is_regular)
    : number_of_dimensions_(number_of_dimensions),
      parameters_(number_of_points, mult, a_dodgy_titer_is_regular)
{

} // acmacs::chart::Stress::Stress

// ----------------------------------------------------------------------

acmacs::chart::Stress::Stress(number_of_dimensions_t number_of_dimensions, size_t number_of_points)
    : number_of_dimensions_(number_of_dimensions),
      parameters_(number_of_points)
{

} // acmacs::chart::Stress::Stress

// ----------------------------------------------------------------------

inline double contribution_regular(size_t point_1, size_t point_2, double table_distance, const double* first, acmacs::number_of_dimensions_t num_dim)
{
    const double diff = table_distance - map_distance(first, point_1, point_2, num_dim);
    return diff * diff;
}

inline double contribution_less_than(size_t point_1, size_t point_2, double table_distance, const double* first, acmacs::number_of_dimensions_t num_dim)
{
    const double diff = table_distance - map_distance(first, point_1, point_2, num_dim) + 1;
    return diff * diff * acmacs::sigmoid(diff * acmacs::chart::SigmoidMutiplier());
}

// inline double contribution_regular(const typename acmacs::chart::TableDistances::Entry& entry, const double* first, acmacs::number_of_dimensions_t num_dim)
// {
//     const double diff = entry.table_distance - map_distance(first, entry, num_dim);
//     return diff * diff;
// }

// inline double contribution_less_than(const typename acmacs::chart::TableDistances::Entry& entry, const double* first, acmacs::number_of_dimensions_t num_dim)
// {
//     const double diff = entry.table_distance - map_distance(first, entry, num_dim) + 1;
//     return diff * diff * acmacs::sigmoid(diff * SigmoidMutiplier());
// }

// ----------------------------------------------------------------------

double acmacs::chart::Stress::value(const double* first, const double*) const
{
    return std::transform_reduce(table_distances().regular().begin(), table_distances().regular().end(), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(entry.point_1, entry.point_2, entry.distance, first, num_dim); }) +
           std::transform_reduce(table_distances().less_than().begin(), table_distances().less_than().end(), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(entry.point_1, entry.point_2, entry.distance, first, num_dim); });

} // acmacs::chart::Stress::value

// ----------------------------------------------------------------------

double acmacs::chart::Stress::value(const acmacs::Layout& aLayout) const
{
    return value(aLayout.as_flat_vector_double().data());

} // acmacs::chart::Stress::value

// ----------------------------------------------------------------------

double acmacs::chart::Stress::contribution(size_t point_no, const double* first) const
{
    return std::transform_reduce(table_distances().begin_regular_for(point_no), table_distances().end_regular_for(point_no), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(entry.point_1, entry.point_2, entry.distance, first, num_dim); }) +
           std::transform_reduce(table_distances().begin_less_than_for(point_no), table_distances().end_less_than_for(point_no), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(entry.point_1, entry.point_2, entry.distance, first, num_dim); });

} // acmacs::chart::Stress::contribution

// ----------------------------------------------------------------------

double acmacs::chart::Stress::contribution(size_t point_no, const acmacs::Layout& aLayout) const
{
    return contribution(point_no, aLayout.as_flat_vector_double().data());

} // acmacs::chart::Stress::contribution

// ----------------------------------------------------------------------

double acmacs::chart::Stress::contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const double* first) const
{
    return std::transform_reduce(
               table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), double{0}, std::plus<>(),
               [point_no, first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(point_no, entry.another_point, entry.distance, first, num_dim); }) +
           std::transform_reduce(
               table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), double{0}, std::plus<>(),
               [point_no, first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(point_no, entry.another_point, entry.distance, first, num_dim); });

} // acmacs::chart::Stress::contribution

// ----------------------------------------------------------------------

double acmacs::chart::Stress::contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const acmacs::Layout& aLayout) const
{
    return contribution(point_no, table_distances_for_point, aLayout.as_flat_vector_double().data());

} // acmacs::chart::Stress::contribution

// ----------------------------------------------------------------------

std::vector<double> acmacs::chart::Stress::gradient(const double* first, const double* last) const
{
    std::vector<double> result(static_cast<size_t>(last - first), 0);
    gradient(first, last, result.data());
    return result;

} // acmacs::chart::Stress::gradient

// ----------------------------------------------------------------------

std::vector<double> acmacs::chart::Stress::gradient(const acmacs::Layout& aLayout) const
{
    const auto& arg = aLayout.as_flat_vector_double();
    return gradient(arg.data(), arg.data() + arg.size());

} // acmacs::chart::Stress::gradient

// ----------------------------------------------------------------------

void acmacs::chart::Stress::gradient(const double* first, const double* last, double* gradient_first) const
{
    if (parameters_.unmovable->empty() && parameters_.unmovable_in_the_last_dimension->empty())
        gradient_plain(first, last, gradient_first);
    else
        gradient_with_unmovable(first, last, gradient_first);

} // acmacs::chart::Stress::gradient

// ----------------------------------------------------------------------

double acmacs::chart::Stress::value_gradient(const double* first, const double* last, double* gradient_first) const
{
    gradient(first, last, gradient_first);
    return value(first, last);

} // acmacs::chart::Stress::value_gradient

// ----------------------------------------------------------------------

void acmacs::chart::Stress::gradient_plain(const double* first, const double* last, double* gradient_first) const
{
    std::for_each(gradient_first, gradient_first + (last - first), [](double& val) { val = 0; });

    auto update = [first,gradient_first,num_dim=static_cast<size_t>(number_of_dimensions_)](const auto& entry, double inc_base) {
        using diff_t = typename std::vector<double>::difference_type;
        auto p1 = first + static_cast<diff_t>(entry.point_1 * num_dim),
                p2 = first + static_cast<diff_t>(entry.point_2 * num_dim);
        auto r1 = gradient_first + static_cast<diff_t>(entry.point_1 * num_dim),
                r2 = gradient_first + static_cast<diff_t>(entry.point_2 * num_dim);
        for (size_t dim = 0; dim < num_dim; ++dim, ++p1, ++p2, ++r1, ++r2) {
            const double inc = inc_base * (*p1 - *p2);
            *r1 -= inc;
            *r2 += inc;
        }
    };

    auto contribution_regular = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double inc_base = (entry.distance - map_dist) * 2 / non_zero(map_dist);
        update(entry, inc_base);
    };
    auto contribution_less_than = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double diff = entry.distance - map_dist + 1;
        const double inc_base = (diff * 2 * acmacs::sigmoid(diff * SigmoidMutiplier())
                                + diff * diff * acmacs::d_sigmoid(diff * SigmoidMutiplier()) * SigmoidMutiplier()) / non_zero(map_dist);
        update(entry, inc_base);
    };

    std::for_each(table_distances().regular().begin(), table_distances().regular().end(), contribution_regular);
    std::for_each(table_distances().less_than().begin(), table_distances().less_than().end(), contribution_less_than);

} // acmacs::chart::Stress::gradient_plain

// ----------------------------------------------------------------------

void acmacs::chart::Stress::gradient_with_unmovable(const double* first, const double* last, double* gradient_first) const
{
    std::vector<bool> unmovable(parameters_.number_of_points, false);
    for (const auto p_no: parameters_.unmovable)
        unmovable[p_no] = true;
    std::vector<bool> unmovable_in_the_last_dimension(parameters_.number_of_points, false);
    for (const auto p_no: parameters_.unmovable_in_the_last_dimension)
        unmovable_in_the_last_dimension[p_no] = true;

    std::for_each(gradient_first, gradient_first + (last - first), [](double& val) { val = 0; });

    auto update = [first,gradient_first,num_dim=static_cast<size_t>(number_of_dimensions_),&unmovable,&unmovable_in_the_last_dimension](const auto& entry, double inc_base) {
        using diff_t = typename std::vector<double>::difference_type;
        auto p1f = [p=static_cast<diff_t>(entry.point_1 * num_dim)] (auto b) { return b + p; };
        auto p2f = [p=static_cast<diff_t>(entry.point_2 * num_dim)] (auto b) { return b + p; };
        auto p1 = p1f(first);
        auto r1 = p1f(gradient_first);
        auto p2 = p2f(first);
        auto r2 = p2f(gradient_first);
        for (size_t dim = 0; dim < num_dim; ++dim, ++p1, ++p2, ++r1, ++r2) {
            const double inc = inc_base * (*p1 - *p2);
            if (!unmovable[entry.point_1] && (!unmovable_in_the_last_dimension[entry.point_1] || (dim + 1) < num_dim))
                *r1 -= inc;
            if (!unmovable[entry.point_2] && (!unmovable_in_the_last_dimension[entry.point_2] || (dim + 1) < num_dim))
                *r2 += inc;
        }
    };

    auto contribution_regular = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double inc_base = (entry.distance - map_dist) * 2 / non_zero(map_dist);
        update(entry, inc_base);
    };
    auto contribution_less_than = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double diff = entry.distance - map_dist + 1;
        const double inc_base = (diff * 2 * acmacs::sigmoid(diff * SigmoidMutiplier())
                                + diff * diff * acmacs::d_sigmoid(diff * SigmoidMutiplier()) * SigmoidMutiplier()) / non_zero(map_dist);
        update(entry, inc_base);
    };

    std::for_each(table_distances().regular().begin(), table_distances().regular().end(), contribution_regular);
    std::for_each(table_distances().less_than().begin(), table_distances().less_than().end(), contribution_less_than);

} // acmacs::chart::Stress::gradient_with_unmovable

// ----------------------------------------------------------------------

void acmacs::chart::Stress::set_coordinates_of_disconnected(double* first, double value, number_of_dimensions_t number_of_dimensions) const
{
      // do not use number_of_dimensions_! after pca its value is wrong!
    for (auto p_no: parameters_.disconnected) {
        for (auto dim : range(number_of_dimensions))
            *(first + p_no * static_cast<size_t>(number_of_dimensions) + static_cast<size_t>(dim)) = value;
    }

} // acmacs::chart::Stress::set_coordinates_of_disconnected

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
