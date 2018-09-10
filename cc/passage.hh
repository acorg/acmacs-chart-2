#pragma once

#include "acmacs-base/passage.hh"
#include "acmacs-chart-2/base.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Passage : public detail::string_data
    {
     public:
        using detail::string_data::string_data;

        bool is_egg() const { return acmacs::passage::is_egg(*this); }
        bool is_cell() const { return acmacs::passage::is_cell(*this); }
        std::string without_date() const { return acmacs::passage::without_date(*this); }
        std::string passage_type() const { return is_egg() ? "egg" : "cell"; }

    }; // class Passage

// ----------------------------------------------------------------------

    class Reassortant : public detail::string_data
    {
     public:
        using detail::string_data::string_data;

    }; // class Reassortant

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
