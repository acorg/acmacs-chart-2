#pragma once

#include "acmacs-base/passage.hh"
#include "acmacs-chart-2/base.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Passage : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

        inline bool is_egg() const { return acmacs::passage::is_egg(data()); }
        inline bool is_cell() const { return acmacs::passage::is_cell(data()); }
        inline std::string without_date() const { return acmacs::passage::without_date(data()); }
        inline std::string passage_type() const { return is_egg() ? "egg" : "cell"; }

    }; // class Passage

// ----------------------------------------------------------------------

    class Reassortant : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Reassortant

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
