#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
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

inline bool endswith(std::string_view source, std::string_view ending)
{
    return source.size() >= ending.size() && source.substr(source.size() - ending.size()) == ending;
}

// ----------------------------------------------------------------------

void acmacs::chart::export_factory(const Chart& chart, std::string_view filename, std::string_view program_name, report_time report)
{
    Timeit ti(fmt::format("writing chart to {}: ", filename), report);

    std::string data;
    if (endswith(filename, ".ace"))
        data = export_factory(chart, export_format::ace, program_name, report_time::no);
    else if (endswith(filename, ".save") || endswith(filename, ".save.xz") || endswith(filename, ".save.gz"))
        data = export_factory(chart, export_format::save, program_name, report_time::no);
    else if (endswith(filename, ".table.txt") || endswith(filename, ".table.txt.xz") || endswith(filename, ".table.txt.gz") || endswith(filename, ".table") || endswith(filename, ".table.xz") || endswith(filename, ".table.gz"))
        data = export_factory(chart, export_format::text_table, program_name, report_time::no);
    else if (endswith(filename, ".txt") || endswith(filename, ".txt.xz") || endswith(filename, ".txt.gz"))
        data = export_factory(chart, export_format::text, program_name, report_time::no);
    else
        throw import_error{fmt::format("[acmacs::chart::export_factory]: cannot infer export format from extension of {}", filename)};

    if (data.empty())
        throw export_error{fmt::format("No data to write to {}", filename)};

    // Timeit ti_file(fmt::format("writing {}: ", filename), report);
    acmacs::file::write(filename, data, endswith(filename, ".ace") ? acmacs::file::force_compression::yes : acmacs::file::force_compression::no);

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
