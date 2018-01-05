#include "acmacs-base/range.hh"
#include "acmacs-base/vector-math.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

// std::transform_reduce is not in g++-7.2 (nor in g++-8, see Parallelism TS in https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017)
#if __GNUC__ == 7
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

template <typename Float> acmacs::chart::Stress<Float> acmacs::chart::stress_factory(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection)
{
    const bool multiply_antigen_titer_until_column_adjust = true;
    Stress<Float> stress(projection.number_of_dimensions(), chart.number_of_antigens());
    auto cb = projection.forced_column_bases();
    if (!cb)
        cb = chart.column_bases(projection.minimum_column_basis());
    chart.titers()->update(stress.table_distances(), *cb, projection.disconnected(), projection.dodgy_titer_is_regular(), projection.avidity_adjusts(), multiply_antigen_titer_until_column_adjust);
      // stress.table_distances().report();
    return stress;

} // acmacs::chart::stress_factory

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const std::vector<Float>& aArgument) const
{
    using diff_t = typename decltype(aArgument.begin())::difference_type;

    constexpr const Float SigmoidMutiplier{10};

    auto map_distance = [&aArgument,num_dim=number_of_dimensions_](const auto& entry) {
        return acmacs::vector_math::distance<Float>(
            aArgument.begin() + static_cast<diff_t>(num_dim * entry.point_1),
            aArgument.begin() + static_cast<diff_t>(num_dim * (entry.point_1 + 1)),
            aArgument.begin() + static_cast<diff_t>(num_dim * entry.point_2));
    };
    auto contribution_regular = [map_distance](const auto& entry) {
        const Float diff = entry.table_distance - map_distance(entry);
        return diff * diff;
    };
    auto contribution_less_than = [map_distance,SigmoidMutiplier](const auto& entry) {
        const Float diff = entry.table_distance - map_distance(entry) + 1;
        return diff * diff * acmacs::sigmoid(diff * SigmoidMutiplier);
    };

    return std::transform_reduce(table_distances().regular().begin(), table_distances().regular().end(), Float{0}, std::plus<>(), contribution_regular)
            + std::transform_reduce(table_distances().less_than().begin(), table_distances().less_than().end(), Float{0}, std::plus<>(), contribution_less_than);

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const acmacs::LayoutInterface& aLayout) const
{
    if constexpr (std::is_same_v<Float, double>)
        return value(aLayout.as_flat_vector_double());
    else
        return value(aLayout.as_flat_vector_float());

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template class acmacs::chart::Stress<float>;
template class acmacs::chart::Stress<double>;
template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection);
template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
