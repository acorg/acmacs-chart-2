#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    std::string export_ace(const Chart& aChart, std::string aProgramName, size_t aIndent);
    std::string export_layout(const Chart& aChart, std::string field_separator, std::string field_encloser, size_t aProjectionNo = 0);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
