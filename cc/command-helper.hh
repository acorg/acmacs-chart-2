#pragma once

#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/point-index-list.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace detail
    {
        inline void extend(PointIndexList& target, std::vector<size_t>&& source, size_t aIncrementEach, size_t aMax)
        {
            for (const auto no : source) {
                if (no >= aMax)
                    throw std::runtime_error("invalid index " + acmacs::to_string(no) + ", expected in range 0.." + acmacs::to_string(aMax - 1) + " inclusive");
                target.insert(no + aIncrementEach);
            }
        }
    } // namespace detail

    inline DisconnectedPoints get_disconnected(std::string_view antigens, std::string_view sera, size_t number_of_antigens, size_t number_of_sera)
    {
        acmacs::chart::DisconnectedPoints points;
        if (!antigens.empty())
            detail::extend(points, acmacs::string::split_into_size_t(antigens), 0, number_of_antigens + number_of_sera);
        if (!sera.empty())
            detail::extend(points, acmacs::string::split_into_size_t(sera), number_of_antigens, number_of_sera);
        return points;
    }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
