#pragma once

#include <string>
#include <memory>

#include "acmacs-chart-2/verify.hh"
#include "acmacs-base/timeit.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    void export_factory(const Chart& aChart, std::string aFilename, std::string aProgramName, report_time aReport);
    inline void export_factory(const Chart& aChart, std::string_view aFilename, std::string aProgramName, report_time aReport) { export_factory(aChart, std::string(aFilename), aProgramName, aReport); }

    enum class export_format { ace, save };
    std::string export_factory(const Chart& aChart, export_format aFormat, std::string aProgramName, report_time aReport);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
