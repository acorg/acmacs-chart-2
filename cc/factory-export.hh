#pragma once

#include <string>
#include <memory>

#include "acmacs-chart-2/verify.hh"
#include "acmacs-base/timeit.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    void export_factory(const Chart& chart, std::string_view filename, std::string_view program_name, report_time report = report_time::no);

    enum class export_format { ace, save, text, text_table };
    std::string export_factory(const Chart& chart, export_format format, std::string_view program_name, report_time report = report_time::no);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
