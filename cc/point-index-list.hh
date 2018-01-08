#pragma once

#include "acmacs-chart-2/base.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class PointIndexList : public internal::index_list_data
    {
     public:
        using internal::index_list_data::index_list_data;

    }; // class PointIndexList

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
