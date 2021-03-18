#pragma once

#include "acmacs-chart-2/selected-antigens-sera.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    std::string export_text(const Chart& chart);
    std::string export_table_to_text(const Chart& chart, std::optional<size_t> just_layer = std::nullopt, bool sort = false);
    std::string export_info_to_text(const Chart& chart);

    std::string export_names_to_text(const Chart& chart, std::string_view format, const SelectedAntigens& antigens, const SelectedSera& sera);
    inline std::string export_names_to_text(std::shared_ptr<Chart> chart, std::string_view format) { return export_names_to_text(*chart, format, SelectedAntigens{chart}, SelectedSera{chart}); }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
