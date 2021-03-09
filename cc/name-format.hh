#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    std::string collapse_spaces(std::string src);
    std::string format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no);
    std::string format_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t serum_no);

    std::string format_help();

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
