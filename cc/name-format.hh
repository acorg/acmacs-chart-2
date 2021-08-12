#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
    class Antigens;
    class Sera;

    enum class collapse_spaces_t { no, yes };

    std::string collapse_spaces(std::string src, collapse_spaces_t cs);
    std::string format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no, collapse_spaces_t cs);
    std::string format_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t serum_no, collapse_spaces_t cs);

    template <typename AgSr> std::string format_antigen_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t no, collapse_spaces_t cs)
    {
        if constexpr (std::is_base_of_v<acmacs::chart::Antigens, AgSr>)
            return format_antigen(pattern, chart, no, cs);
        else if constexpr (std::is_base_of_v<acmacs::chart::Sera, AgSr>)
            return format_serum(pattern, chart, no, cs);
        else
            static_assert(std::is_same_v<AgSr, acmacs::chart::Sera>);
    }

    template <typename AgSr> void format_antigen_serum(fmt::memory_buffer& out, std::string_view pattern, const acmacs::chart::Chart& chart, size_t no, collapse_spaces_t cs)
    {
        if constexpr (std::is_base_of_v<acmacs::chart::Antigens, AgSr>)
            fmt::format_to_mb(out, fmt::runtime(format_antigen(pattern, chart, no, cs)));
        else if constexpr (std::is_base_of_v<acmacs::chart::Sera, AgSr>)
            fmt::format_to_mb(out, fmt::runtime(format_serum(pattern, chart, no, cs)));
        else
            static_assert(std::is_same_v<AgSr, acmacs::chart::Sera>);
    }

    std::string format_point(std::string_view pattern, const acmacs::chart::Chart& chart, size_t point_no, collapse_spaces_t cs);

    std::string format_help();

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
