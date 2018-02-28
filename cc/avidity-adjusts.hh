#pragma once

#include "acmacs-chart-2/base.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class AvidityAdjusts : public internal::double_list_data
    {
     public:
        using internal::double_list_data::double_list_data;

        constexpr inline bool empty() const
        {
            return internal::double_list_data::empty() || std::all_of(begin(), end(), [](double val) -> bool { return float_equal(val, 1.0); });
        }

        std::vector<double> logged(size_t number_of_points) const
        {
            std::vector<double> logged_adjusts(number_of_points, 0.0);
            if (!empty())
                std::transform(begin(), end(), logged_adjusts.begin(), [](double adj) { return std::log2(adj); });
            return logged_adjusts;
        }

    }; // class AvidityAdjusts

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
