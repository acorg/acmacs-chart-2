#pragma once

#include <string>
#include <optional>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    std::string export_text(const Chart& chart);
    std::string export_table_to_text(const Chart& chart, std::optional<size_t> just_layer = std::nullopt, bool sort=false);
    std::string export_info_to_text(const Chart& chart);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
