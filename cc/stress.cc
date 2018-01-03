#include "acmacs-base/range.hh"
#include "acmacs-base/vector-math.hh"
#include "acmacs-base/sigmoid.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const std::vector<Float>& aArgument) const
{
    using diff_t = typename decltype(aArgument.begin())::difference_type;
    using index_t = decltype(table_distances().regular_indexes().begin());

    constexpr const Float SigmoidMutiplier{10};

    auto map_distance = [&aArgument,this](const index_t index) {
        return acmacs::vector_math::distance<Float>(
            aArgument.begin() + static_cast<diff_t>(this->number_of_dimensions_ * index->first),
            aArgument.begin() + static_cast<diff_t>(this->number_of_dimensions_ * (index->first + 1)),
            aArgument.begin() + static_cast<diff_t>(this->number_of_dimensions_ * index->second));
    };

    auto contribution_regular = [map_distance,index=table_distances().regular_indexes().begin()](Float sum, Float table_distance) mutable {
        const Float diff = table_distance - map_distance(index++);
        return sum + diff * diff;
    };

    auto contribution_less_than = [map_distance,index=table_distances().less_than_indexes().begin()](Float sum, Float table_distance) mutable {
        const Float diff = table_distance - map_distance(index++) + 1;
        return sum + diff * diff * sigmoid(diff, SigmoidMutiplier);
    };

    return std::accumulate(table_distances().regular_distances().begin(), table_distances().regular_distances().end(), Float{0}, contribution_regular)
            + std::accumulate(table_distances().less_than_distances().begin(), table_distances().less_than_distances().end(), Float{0}, contribution_less_than);

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const acmacs::LayoutInterface& aLayout) const
{
    std::vector<Float> argument(aLayout.number_of_points() * aLayout.number_of_dimensions());
    for (auto p_no : acmacs::range(aLayout.number_of_points())) {
        for (auto d_no : acmacs::range(aLayout.number_of_dimensions())) {
            argument[p_no * aLayout.number_of_dimensions() + d_no] = static_cast<Float>(aLayout.coordinate(p_no, d_no));
        }
    }
    return value(argument);

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template class acmacs::chart::Stress<float>;
template class acmacs::chart::Stress<double>;

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
