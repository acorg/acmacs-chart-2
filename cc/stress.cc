#include "acmacs-base/range.hh"
#include "acmacs-base/vector-math.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart.hh"

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
