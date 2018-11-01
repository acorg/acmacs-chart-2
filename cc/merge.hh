#pragma once

#include <tuple>
#include <map>

#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    struct MergeSettings
    {
        MergeSettings() = default;
        MergeSettings(CommonAntigensSera::match_level_t a_match_level) : match_level{a_match_level} {}
        CommonAntigensSera::match_level_t match_level = CommonAntigensSera::match_level_t::automatic;
        bool remove_distinct = true;
    };
    
    struct MergeReport
    {
        struct target_index_common_t
        {
            target_index_common_t() = default;
            target_index_common_t& operator=(size_t a_index) { index = a_index; return *this; }
            target_index_common_t& operator=(const target_index_common_t& src) { index = src.index; common = true; return *this; }
            size_t index;
            bool common = false;
        };

        using index_mapping_t = std::map<size_t, target_index_common_t>; // primary/secondary index -> (target index, if common)

        MergeReport(const Chart& primary, const Chart& secondary, const MergeSettings& settings);

        CommonAntigensSera::match_level_t match_level;
        CommonAntigensSera common;
        index_mapping_t antigens_primary_target, antigens_secondary_target, sera_primary_target, sera_secondary_target;
        size_t target_antigens = 0, target_sera = 0;
    };

    inline std::ostream& operator<<(std::ostream& out, const MergeReport::target_index_common_t& entry) { return out << '[' << entry.index << (entry.common ? ",common" : "") << ']'; }

    std::pair<ChartModifyP, MergeReport> merge(const Chart& chart1, const Chart& chart2, const MergeSettings& settings = {});
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
