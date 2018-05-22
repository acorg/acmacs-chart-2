#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/lispmds-export.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::export_factory(const Chart& aChart, acmacs::chart::export_format aFormat, std::string aProgramName, report_time aReport)
{
    Timeit ti("exporting chart: ", aReport);
    switch (aFormat) {
      case export_format::ace:
          return ace_export(aChart, aProgramName, 1);
      case export_format::save:
          return lispmds_export(aChart, aProgramName);
    }
    return {};

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------

void acmacs::chart::export_factory(const Chart& aChart, std::string aFilename, std::string aProgramName, report_time aReport)
{
    Timeit ti("writing chart to " + aFilename + ": ", aReport);
    auto force_compression = acmacs::file::ForceCompression::No;
    std::string data;
    if (fs::path(aFilename).extension() == ".ace") {
        force_compression = acmacs::file::ForceCompression::Yes;
        data = export_factory(aChart, export_format::ace, aProgramName, report_time::No);
    }
    else if (fs::path(aFilename).extension() == ".save") {
        force_compression = acmacs::file::ForceCompression::No;
        data = export_factory(aChart, export_format::save, aProgramName, report_time::No);
    }
    else if (aFilename.size() > 8 && aFilename.substr(aFilename.size() - 8) == ".save.xz") {
        force_compression = acmacs::file::ForceCompression::Yes;
        data = export_factory(aChart, export_format::save, aProgramName, report_time::No);
    }
    else if (aFilename.size() > 8 && aFilename.substr(aFilename.size() - 8) == ".save.gz") {
        force_compression = acmacs::file::ForceCompression::Yes;
        data = export_factory(aChart, export_format::save, aProgramName, report_time::No);
    }
    else
        throw import_error{"[acmacs::chart::export_factory]: cannot infer export format from extension of " + aFilename};

    if (data.empty())
        throw export_error("No data to write to " + aFilename);

    Timeit ti_file("writing " + aFilename + ": ", aReport);
    acmacs::file::write(aFilename, data, force_compression);

} // acmacs::chart::export_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
