#include "acmacs-base/read-file.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/lispmds-export.hh"
#include "acmacs-chart-2/text-export.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::export_factory(const Chart& chart, acmacs::chart::export_format format, std::string_view program_name, report_time report)
{
    Timeit ti("exporting chart: ", report);
    switch (format) {
      case export_format::ace:
          return export_ace(chart, program_name, 1);
      case export_format::save:
          return export_lispmds(chart, program_name);
      case export_format::text:
          return export_text(chart);
      case export_format::text_table:
          return export_table_to_text(chart);
    }
    return {};

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------

void acmacs::chart::export_factory(const Chart& chart, std::string_view filename, std::string_view program_name, report_time report)
{
    using namespace std::string_view_literals;
    Timeit ti(fmt::format("writing chart to {}: ", filename), report);

    std::string data;
    if (acmacs::string::endswith(filename, ".ace"sv))
        data = export_factory(chart, export_format::ace, program_name, report_time::no);
    else if (acmacs::string::endswith(filename, ".save"sv) || acmacs::string::endswith(filename, ".save.xz"sv) || acmacs::string::endswith(filename, ".save.gz"sv))
        data = export_factory(chart, export_format::save, program_name, report_time::no);
    else if (acmacs::string::endswith(filename, ".table.txt"sv) || acmacs::string::endswith(filename, ".table.txt.xz"sv) || acmacs::string::endswith(filename, ".table.txt.gz"sv) || acmacs::string::endswith(filename, ".table"sv) || acmacs::string::endswith(filename, ".table.xz"sv) || acmacs::string::endswith(filename, ".table.gz"sv))
        data = export_factory(chart, export_format::text_table, program_name, report_time::no);
    else if (acmacs::string::endswith(filename, ".txt"sv) || acmacs::string::endswith(filename, ".txt.xz"sv) || acmacs::string::endswith(filename, ".txt.gz"sv))
        data = export_factory(chart, export_format::text, program_name, report_time::no);
    else
        throw import_error{fmt::format("[acmacs::chart::export_factory]: cannot infer export format from extension of {}", filename)};

    if (data.empty())
        throw export_error{fmt::format("No data to write to {}", filename)};

    // Timeit ti_file(fmt::format("writing {}: ", filename), report);
    acmacs::file::write(filename, data, acmacs::string::endswith(filename, ".ace"sv) ? acmacs::file::force_compression::yes : acmacs::file::force_compression::no);

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
