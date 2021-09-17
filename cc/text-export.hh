#pragma once

#include "acmacs-chart-2/selected-antigens-sera.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    enum class org_mode_separators_t { no, yes };

    std::string export_text(const Chart& chart);
    std::string export_table_to_text(const Chart& chart, std::optional<size_t> just_layer = std::nullopt, bool sort = false, org_mode_separators_t org_mode_separators = org_mode_separators_t::no);
    std::string export_info_to_text(const Chart& chart);

    template <typename SA, typename SS> std::string export_names_to_text(const Chart& chart, std::string_view format, const SA& antigens, const SS& sera)
    {
        fmt::memory_buffer out;
        for (const auto ag_no : antigens.indexes)
            fmt::format_to_mb(out, "{}", acmacs::chart::format_antigen(format, chart, ag_no, collapse_spaces_t::yes));
        fmt::format_to_mb(out, "\n");
        for (const auto sr_no : sera.indexes)
            fmt::format_to_mb(out, "{}", acmacs::chart::format_serum(format, chart, sr_no, collapse_spaces_t::yes));
        return fmt::to_string(out);
    }

    inline std::string export_names_to_text(std::shared_ptr<Chart> chart, std::string_view format) { return export_names_to_text(*chart, format, SelectedAntigens{chart}, SelectedSera{chart}); }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
