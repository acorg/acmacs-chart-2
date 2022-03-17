#pragma once

#include "acmacs-base/flat-map.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class merge_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    enum class projection_merge_t { type1, type2, type3, type4, type5 }; // see ../doc/merge-types.org
    enum class remove_distinct { no, yes };
    enum class combine_cheating_assays { no, yes };

    struct MergeSettings
    {
        MergeSettings() = default;
        MergeSettings(CommonAntigensSera::match_level_t a_match_level, projection_merge_t a_projection_merge = projection_merge_t::type1,
                      combine_cheating_assays a_combine_cheating_assays = combine_cheating_assays::no, remove_distinct a_remove_distinct = remove_distinct::no)
            : match_level{a_match_level}, projection_merge{a_projection_merge}, combine_cheating_assays_{a_combine_cheating_assays}, remove_distinct_{a_remove_distinct}
        {
        }
        MergeSettings(projection_merge_t a_projection_merge) : projection_merge{a_projection_merge} {}

        CommonAntigensSera::match_level_t match_level{CommonAntigensSera::match_level_t::automatic};
        projection_merge_t projection_merge{projection_merge_t::type1};
        combine_cheating_assays combine_cheating_assays_{combine_cheating_assays::no};
        remove_distinct remove_distinct_{remove_distinct::no};
    };

    struct MergeReport
    {
        struct target_index_common_t
        {
            // target_index_common_t() = default;
            // target_index_common_t& operator=(size_t a_index) { index = a_index; return *this; }
            // target_index_common_t& operator=(const target_index_common_t& src) { index = src.index; common = true; return *this; }
            size_t index{static_cast<size_t>(-1)};
            bool common{false};
        };

        using index_mapping_t = small_map_with_unique_keys_t<size_t, target_index_common_t>; // primary/secondary index -> (target index, if common)

        MergeReport(const Chart& primary, const Chart& secondary, const MergeSettings& settings);

        std::string titer_merge_report(const ChartModify& chart) const;
        std::string titer_merge_report_common_only(const ChartModify& chart) const;
        std::string titer_merge_diagnostics(const ChartModify& chart, const PointIndexList& antigens, const PointIndexList& sera, int max_field_size) const;
        Indexes secondary_antigens_to_merge(const Chart& primary, const Chart& secondary, const MergeSettings& settings) const;

        CommonAntigensSera::match_level_t match_level;
        CommonAntigensSera common;
        index_mapping_t antigens_primary_target, antigens_secondary_target, sera_primary_target, sera_secondary_target;
        size_t target_antigens = 0, target_sera = 0;
        std::unique_ptr<TitersModify::titer_merge_report> titer_report;
    };

    std::pair<ChartModifyP, MergeReport> merge(const Chart& chart1, const Chart& chart2, const MergeSettings& settings = {});
}

template <> struct fmt::formatter<acmacs::chart::MergeReport::target_index_common_t> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::MergeReport::target_index_common_t& value, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "[{}{}]", value.index, value.common ? ",common" : "");
    }
};

// ----------------------------------------------------------------------
