#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/lispmds-export.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::export_factory(const Chart& aChart, acmacs::chart::export_format aFormat, std::string_view aProgramName, report_time aReport)
{
    Timeit ti("exporting chart: ", aReport);
    switch (aFormat) {
      case export_format::ace:
          return export_ace(aChart, aProgramName, 1);
      case export_format::save:
          return export_lispmds(aChart, aProgramName);
    }
    return {};

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------

void acmacs::chart::export_factory(const Chart& aChart, std::string_view aFilename, std::string_view aProgramName, report_time aReport)
{
    Timeit ti(fmt::format("writing chart to {}: ", aFilename), aReport);
    auto force_compression = acmacs::file::force_compression::no;
    std::string data;
    if (fs::path(aFilename).extension() == ".ace") {
        force_compression = acmacs::file::force_compression::yes;
        data = export_factory(aChart, export_format::ace, aProgramName, report_time::no);
    }
    else if (fs::path(aFilename).extension() == ".save") {
        force_compression = acmacs::file::force_compression::no;
        data = export_factory(aChart, export_format::save, aProgramName, report_time::no);
    }
    else if (aFilename.size() > 8 && aFilename.substr(aFilename.size() - 8) == ".save.xz") {
        force_compression = acmacs::file::force_compression::yes;
        data = export_factory(aChart, export_format::save, aProgramName, report_time::no);
    }
    else if (aFilename.size() > 8 && aFilename.substr(aFilename.size() - 8) == ".save.gz") {
        force_compression = acmacs::file::force_compression::yes;
        data = export_factory(aChart, export_format::save, aProgramName, report_time::no);
    }
    else
        throw import_error{fmt::format("[acmacs::chart::export_factory]: cannot infer export format from extension of {}", aFilename)};

    if (data.empty())
        throw export_error{fmt::format("No data to write to {}", aFilename)};

    Timeit ti_file(fmt::format("writing {}: ", aFilename), aReport);
    acmacs::file::write(aFilename, data, force_compression);

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
