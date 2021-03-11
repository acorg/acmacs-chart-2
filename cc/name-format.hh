#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    enum class collapse_spaces_t { no, yes };

    std::string collapse_spaces(std::string src, collapse_spaces_t cs);
    std::string format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no, collapse_spaces_t cs);
    std::string format_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t serum_no, collapse_spaces_t cs);

    std::string format_help();

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
