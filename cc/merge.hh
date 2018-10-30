#pragma once

#include <tuple>

#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    struct MergeReport
    {
        MergeReport(const Chart& primary, const Chart& secondary, CommonAntigensSera::match_level_t a_match_level);

        CommonAntigensSera::match_level_t match_level;
        CommonAntigensSera common;
    };

    std::pair<ChartModifyP, MergeReport> merge(const Chart& chart1, const Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level = acmacs::chart::CommonAntigensSera::match_level_t::automatic);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
