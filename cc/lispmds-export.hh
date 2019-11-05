#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    std::string export_lispmds(const Chart& aChart, std::string_view aProgramName);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
