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

        bool empty() const
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

        void set_logged(size_t antigen_no, double logged_adjust)
        {
            if (size() <= antigen_no)
                throw std::runtime_error{AD_FORMAT("set_logged: cannot set adjust for AG {} to {}: too few elements in the AvidityAdjusts list ({})", antigen_no, logged_adjust, size())};
            operator[](antigen_no) = std::exp2(logged_adjust);
        }

        void resize(size_t number_of_points)
            {
                if (size() == 0)
                    get().resize(number_of_points, 1.0);
                else if (size() != number_of_points)
                    throw std::runtime_error{AD_FORMAT("attempt to resize AvidityAdjusts from {} to {}", size(), number_of_points)};
            }

    }; // class AvidityAdjusts

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
