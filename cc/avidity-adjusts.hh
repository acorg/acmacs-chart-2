#pragma once

#include "acmacs-base/named-type.hh"
#include "acmacs-base/float.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class AvidityAdjusts : public acmacs::named_vector_t<double, struct chart_AvidityAdjusts_tag_t>
    {
     public:
        using acmacs::named_vector_t<double, struct chart_AvidityAdjusts_tag_t>::named_vector_t;
        AvidityAdjusts(const rjson::value& src) : acmacs::named_vector_t<double, struct chart_AvidityAdjusts_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        inline bool empty() const
        {
            return get().empty() || std::all_of(begin(), end(), [](double val) -> bool { return float_equal(val, 1.0); });
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
