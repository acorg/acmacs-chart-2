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

template <typename Float> Float acmacs::chart::Stress<Float>::value(const std::vector<Float>& aArgument) const
{
    using diff_t = typename decltype(aArgument.begin())::difference_type;

    constexpr const Float SigmoidMutiplier{10};

    auto map_distance = [&aArgument,num_dim=number_of_dimensions_](const auto& index) {
        return acmacs::vector_math::distance<Float>(
            aArgument.begin() + static_cast<diff_t>(num_dim * index.first),
            aArgument.begin() + static_cast<diff_t>(num_dim * (index.first + 1)),
            aArgument.begin() + static_cast<diff_t>(num_dim * index.second));
    };
    auto contribution_regular = [map_distance](Float table_distance, const auto& index) {
        const Float diff = table_distance - map_distance(index);
        return diff * diff;
    };
    auto contribution_less_than = [map_distance](Float table_distance, const auto& index) {
        const Float diff = table_distance - map_distance(index) + 1;
        return diff * diff * acmacs::sigmoid(diff * SigmoidMutiplier);
    };

    return std::transform_reduce(table_distances().regular_distances().begin(), table_distances().regular_distances().end(), table_distances().regular_indexes().begin(), Float{0}, std::plus<>(), contribution_regular)
            + std::transform_reduce(table_distances().less_than_distances().begin(), table_distances().less_than_distances().end(), table_distances().less_than_indexes().begin(), Float{0}, std::plus<>(), contribution_less_than);

} // acmacs::chart::Stress<Float>::value

// template <typename Float> Float acmacs::chart::Stress<Float>::value(const std::vector<Float>& aArgument) const
// {
//     using diff_t = typename decltype(aArgument.begin())::difference_type;
//     using index_t = decltype(table_distances().regular_indexes().begin());

//     constexpr const Float SigmoidMutiplier{10};

//     auto map_distance = [&aArgument,num_dim=this->number_of_dimensions_](const index_t& index) {
//         return acmacs::vector_math::distance<Float>(
//             aArgument.begin() + static_cast<diff_t>(num_dim * index->first),
//             aArgument.begin() + static_cast<diff_t>(num_dim * (index->first + 1)),
//             aArgument.begin() + static_cast<diff_t>(num_dim * index->second));
//     };

//     auto contribution_regular = [map_distance,index=table_distances().regular_indexes().begin()](Float sum, Float table_distance) mutable {
//         const Float diff = table_distance - map_distance(index++);
//         return sum + diff * diff;
//     };

//     auto contribution_less_than = [map_distance,index=table_distances().less_than_indexes().begin()](Float sum, Float table_distance) mutable {
//         const Float diff = table_distance - map_distance(index++) + 1;
//         return sum + diff * diff * sigmoid(diff * SigmoidMutiplier);
//     };

//     return std::accumulate(table_distances().regular_distances().begin(), table_distances().regular_distances().end(), Float{0}, contribution_regular)
//             + std::accumulate(table_distances().less_than_distances().begin(), table_distances().less_than_distances().end(), Float{0}, contribution_less_than);

// } // acmacs::chart::Stress<Float>::value

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

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
