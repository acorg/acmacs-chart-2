#include "acmacs-base/range.hh"
#include "acmacs-base/vector-math.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

// std::transform_reduce is not in g++-7.2 (nor in g++-8, see Parallelism TS in https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017)
#ifndef __clang__
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

template <typename Float> acmacs::chart::Stress<Float> acmacs::chart::stress_factory(const acmacs::chart::Projection& projection, acmacs::chart::multiply_antigen_titer_until_column_adjust mult)
{
    Stress<Float> stress(projection, mult);
    auto cb = projection.forced_column_bases();
    if (!cb)
        cb = projection.chart().column_bases(projection.minimum_column_basis());
    projection.chart().titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // acmacs::chart::stress_factory

// ----------------------------------------------------------------------

template <typename Float> acmacs::chart::Stress<Float> acmacs::chart::stress_factory(const acmacs::chart::Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular)
{
    Stress<Float> stress(number_of_dimensions, chart.number_of_points(), mult, a_dodgy_titer_is_regular);
    auto cb = chart.forced_column_bases();
    if (!cb)
        cb = chart.column_bases(minimum_column_basis);
    chart.titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // acmacs::chart::stress_factory

// ----------------------------------------------------------------------

acmacs::chart::TableDistances<double> acmacs::chart::table_distances(const acmacs::chart::Chart& chart, MinimumColumnBasis minimum_column_basis, bool a_dodgy_titer_is_regular)
{
    Stress<double> stress(2, chart.number_of_points(), multiply_antigen_titer_until_column_adjust::yes, a_dodgy_titer_is_regular);
    auto cb = chart.forced_column_bases();
    if (!cb)
        cb = chart.column_bases(minimum_column_basis);
    return chart.titers()->table_distances(*cb, stress.parameters());

} // acmacs::chart::table_distances

// ----------------------------------------------------------------------

template <typename Float> constexpr inline Float SigmoidMutiplier() { return 10; }
template <typename Float> constexpr inline Float non_zero(Float value) { return float_zero(value) ? static_cast<Float>(1e-5) : value; };

// ----------------------------------------------------------------------

template <typename Float> static inline std::vector<Float> flatten(const acmacs::LayoutInterface& aLayout)
{
    if constexpr (std::is_same_v<Float, double>)
        return aLayout.as_flat_vector_double();
    else
        return aLayout.as_flat_vector_float();

} // flatten

// ----------------------------------------------------------------------

template <typename Float> static inline Float map_distance(const Float* first, size_t point_1, size_t point_2, size_t number_of_dimensions)
{
    using diff_t = typename std::vector<Float>::difference_type;
    return acmacs::vector_math::distance<Float>(
        first + static_cast<diff_t>(number_of_dimensions * point_1),
        first + static_cast<diff_t>(number_of_dimensions * (point_1 + 1)),
        first + static_cast<diff_t>(number_of_dimensions * point_2));

} // map_distance

// ----------------------------------------------------------------------

template <typename Float> static inline Float map_distance(const Float* first, const typename acmacs::chart::TableDistances<Float>::Entry& entry, size_t number_of_dimensions)
{
    return map_distance(first, entry.point_1, entry.point_2, number_of_dimensions);

} // map_distance

// ----------------------------------------------------------------------

template <typename Float> acmacs::chart::Stress<Float>::Stress(const Projection& projection, acmacs::chart::multiply_antigen_titer_until_column_adjust mult)
    : number_of_dimensions_(projection.number_of_dimensions()),
      parameters_(projection.number_of_points(), projection.unmovable(), projection.disconnected(), projection.unmovable_in_the_last_dimension(),
                  mult, projection.avidity_adjusts(), projection.dodgy_titer_is_regular())
{
} // acmacs::chart::Stress<Float>::Stress

// ----------------------------------------------------------------------

template <typename Float> acmacs::chart::Stress<Float>::Stress(size_t number_of_dimensions, size_t number_of_points, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular)
    : number_of_dimensions_(number_of_dimensions),
      parameters_(number_of_points, mult, a_dodgy_titer_is_regular)
{

} // acmacs::chart::Stress<Float>::Stress

// ----------------------------------------------------------------------

template <typename Float> inline Float contribution_regular(size_t point_1, size_t point_2, Float table_distance, const Float* first, size_t num_dim)
{
    const Float diff = table_distance - map_distance(first, point_1, point_2, num_dim);
    return diff * diff;
}

template <typename Float> inline Float contribution_less_than(size_t point_1, size_t point_2, Float table_distance, const Float* first, size_t num_dim)
{
    const Float diff = table_distance - map_distance(first, point_1, point_2, num_dim) + 1;
    return diff * diff * acmacs::sigmoid(diff * SigmoidMutiplier<Float>());
}

// template <typename Float> inline Float contribution_regular(const typename acmacs::chart::TableDistances<Float>::Entry& entry, const Float* first, size_t num_dim)
// {
//     const Float diff = entry.table_distance - map_distance(first, entry, num_dim);
//     return diff * diff;
// }

// template <typename Float> inline Float contribution_less_than(const typename acmacs::chart::TableDistances<Float>::Entry& entry, const Float* first, size_t num_dim)
// {
//     const Float diff = entry.table_distance - map_distance(first, entry, num_dim) + 1;
//     return diff * diff * acmacs::sigmoid(diff * SigmoidMutiplier<Float>());
// }

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const Float* first, const Float*) const
{
    return std::transform_reduce(table_distances().regular().begin(), table_distances().regular().end(), Float{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(entry.point_1, entry.point_2, entry.table_distance, first, num_dim); }) +
           std::transform_reduce(table_distances().less_than().begin(), table_distances().less_than().end(), Float{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(entry.point_1, entry.point_2, entry.table_distance, first, num_dim); });

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const acmacs::LayoutInterface& aLayout) const
{
    const auto arg = ::flatten<Float>(aLayout);
    return value(arg.data());

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::contribution(size_t point_no, const Float* first) const
{
    return std::transform_reduce(table_distances().begin_regular_for(point_no), table_distances().end_regular_for(point_no), Float{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(entry.point_1, entry.point_2, entry.table_distance, first, num_dim); }) +
           std::transform_reduce(table_distances().begin_less_than_for(point_no), table_distances().end_less_than_for(point_no), Float{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(entry.point_1, entry.point_2, entry.table_distance, first, num_dim); });

} // acmacs::chart::Stress<Float>::contribution

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::contribution(size_t point_no, const acmacs::LayoutInterface& aLayout) const
{
    const auto arg = ::flatten<Float>(aLayout);
    return contribution(point_no, arg.data());

} // acmacs::chart::Stress<Float>::contribution

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const Float* first) const
{
    return std::transform_reduce(
               table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), Float{0}, std::plus<>(),
               [point_no, first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(point_no, entry.another_point, entry.table_distance, first, num_dim); }) +
           std::transform_reduce(
               table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), Float{0}, std::plus<>(),
               [point_no, first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(point_no, entry.another_point, entry.table_distance, first, num_dim); });

} // acmacs::chart::Stress<Float>::contribution

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const acmacs::LayoutInterface& aLayout) const
{
    const auto arg = ::flatten<Float>(aLayout);
    return contribution(point_no, table_distances_for_point, arg.data());

} // acmacs::chart::Stress<Float>::contribution

// ----------------------------------------------------------------------

template <typename Float> std::vector<Float> acmacs::chart::Stress<Float>::gradient(const Float* first, const Float* last) const
{
    std::vector<Float> result(static_cast<size_t>(last - first), 0);
    gradient(first, last, result.data());
    return result;

} // acmacs::chart::Stress<Float>::gradient

// ----------------------------------------------------------------------

template <typename Float> std::vector<Float> acmacs::chart::Stress<Float>::gradient(const acmacs::LayoutInterface& aLayout) const
{
    const auto arg = ::flatten<Float>(aLayout);
    return gradient(arg.data(), arg.data() + arg.size());

} // acmacs::chart::Stress<Float>::gradient

// ----------------------------------------------------------------------

template <typename Float> void acmacs::chart::Stress<Float>::gradient(const Float* first, const Float* last, Float* gradient_first) const
{
    if (parameters_.unmovable.empty() && parameters_.unmovable_in_the_last_dimension.empty())
        gradient_plain(first, last, gradient_first);
    else
        gradient_with_unmovable(first, last, gradient_first);

} // acmacs::chart::Stress<Float>::gradient

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value_gradient(const Float* first, const Float* last, Float* gradient_first) const
{
    gradient(first, last, gradient_first);
    return value(first, last);

} // acmacs::chart::Stress<Float>::value_gradient

// ----------------------------------------------------------------------

template <typename Float> void acmacs::chart::Stress<Float>::gradient_plain(const Float* first, const Float* last, Float* gradient_first) const
{
    std::for_each(gradient_first, gradient_first + (last - first), [](Float& val) { val = 0; });

    auto update = [first,gradient_first,num_dim=number_of_dimensions_](const auto& entry, Float inc_base) {
        using diff_t = typename std::vector<Float>::difference_type;
        auto p1 = first + static_cast<diff_t>(entry.point_1 * num_dim),
                p2 = first + static_cast<diff_t>(entry.point_2 * num_dim);
        auto r1 = gradient_first + static_cast<diff_t>(entry.point_1 * num_dim),
                r2 = gradient_first + static_cast<diff_t>(entry.point_2 * num_dim);
        for (size_t d = 0; d < num_dim; ++d, ++p1, ++p2, ++r1, ++r2) {
            const Float inc = inc_base * (*p1 - *p2);
            *r1 -= inc;
            *r2 += inc;
        }
    };

    auto contribution_regular = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const Float map_dist = ::map_distance(first, entry, num_dim);
        const Float inc_base = (entry.table_distance - map_dist) * 2 / non_zero(map_dist);
        update(entry, inc_base);
    };
    auto contribution_less_than = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const Float map_dist = ::map_distance(first, entry, num_dim);
        const Float diff = entry.table_distance - map_dist + 1;
        const Float inc_base = (diff * 2 * acmacs::sigmoid(diff * SigmoidMutiplier<Float>())
                                + diff * diff * acmacs::d_sigmoid(diff * SigmoidMutiplier<Float>()) * SigmoidMutiplier<Float>()) / non_zero(map_dist);
        update(entry, inc_base);
    };

    std::for_each(table_distances().regular().begin(), table_distances().regular().end(), contribution_regular);
    std::for_each(table_distances().less_than().begin(), table_distances().less_than().end(), contribution_less_than);

} // acmacs::chart::Stress<Float>::gradient_plain

// ----------------------------------------------------------------------

template <typename Float> void acmacs::chart::Stress<Float>::gradient_with_unmovable(const Float* first, const Float* last, Float* gradient_first) const
{
    std::vector<bool> unmovable(parameters_.number_of_points, false);
    for (const auto p_no: parameters_.unmovable)
        unmovable[p_no] = true;
    std::vector<bool> unmovable_in_the_last_dimension(parameters_.number_of_points, false);
    for (const auto p_no: parameters_.unmovable_in_the_last_dimension)
        unmovable_in_the_last_dimension[p_no] = true;

    std::for_each(gradient_first, gradient_first + (last - first), [](Float& val) { val = 0; });

    auto update = [first,gradient_first,num_dim=number_of_dimensions_,&unmovable,&unmovable_in_the_last_dimension](const auto& entry, Float inc_base) {
        using diff_t = typename std::vector<Float>::difference_type;
        auto p1f = [p=static_cast<diff_t>(entry.point_1 * num_dim)] (auto b) { return b + p; };
        auto p2f = [p=static_cast<diff_t>(entry.point_2 * num_dim)] (auto b) { return b + p; };
        auto p1 = p1f(first);
        auto r1 = p1f(gradient_first);
        auto p2 = p2f(first);
        auto r2 = p2f(gradient_first);
        for (size_t d = 0; d < num_dim; ++d, ++p1, ++p2, ++r1, ++r2) {
            const Float inc = inc_base * (*p1 - *p2);
            if (!unmovable[entry.point_1] && (!unmovable_in_the_last_dimension[entry.point_1] || (d + 1) < num_dim))
                *r1 -= inc;
            if (!unmovable[entry.point_2] && (!unmovable_in_the_last_dimension[entry.point_2] || (d + 1) < num_dim))
                *r2 += inc;
        }
    };

    auto contribution_regular = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const Float map_dist = ::map_distance(first, entry, num_dim);
        const Float inc_base = (entry.table_distance - map_dist) * 2 / non_zero(map_dist);
        update(entry, inc_base);
    };
    auto contribution_less_than = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const Float map_dist = ::map_distance(first, entry, num_dim);
        const Float diff = entry.table_distance - map_dist + 1;
        const Float inc_base = (diff * 2 * acmacs::sigmoid(diff * SigmoidMutiplier<Float>())
                                + diff * diff * acmacs::d_sigmoid(diff * SigmoidMutiplier<Float>()) * SigmoidMutiplier<Float>()) / non_zero(map_dist);
        update(entry, inc_base);
    };

    std::for_each(table_distances().regular().begin(), table_distances().regular().end(), contribution_regular);
    std::for_each(table_distances().less_than().begin(), table_distances().less_than().end(), contribution_less_than);

} // acmacs::chart::Stress<Float>::gradient_with_unmovable

// ----------------------------------------------------------------------

template <typename Float> void acmacs::chart::Stress<Float>::set_coordinates_of_disconnected(Float* first, Float value) const
{
    for (auto p_no: parameters_.disconnected) {
        for (size_t dim = 0; dim < number_of_dimensions_; ++dim)
            *(first + p_no * number_of_dimensions_ + dim) = value;
    }

} // acmacs::chart::Stress<Float>::set_coordinates_of_disconnected

// ----------------------------------------------------------------------

template class acmacs::chart::Stress<float>;
template class acmacs::chart::Stress<double>;
template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Projection& projection, acmacs::chart::multiply_antigen_titer_until_column_adjust mult);
template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Projection& projection, acmacs::chart::multiply_antigen_titer_until_column_adjust mult);
template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular);
template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, bool a_dodgy_titer_is_regular);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
