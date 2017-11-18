#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/lispmds-export.hh"

// ----------------------------------------------------------------------

void acmacs::chart::export_factory(const Chart& aChart, std::string aFilename, std::string aProgramName)
{
    Timeit ti("writing chart to " + aFilename + ": ");
    auto force_compression = acmacs::file::ForceCompression::No;
    std::string data;
    if (fs::path(aFilename).extension() == ".ace") {
        force_compression = acmacs::file::ForceCompression::Yes;
        data = ace_export(aChart, aProgramName);
    }
    else if (fs::path(aFilename).extension() == ".save") {
        force_compression = acmacs::file::ForceCompression::No;
        data = lispmds_export(aChart, aProgramName);
    }
    else if (aFilename.size() > 8 && aFilename.substr(aFilename.size() - 8) == ".save.xz") {
        force_compression = acmacs::file::ForceCompression::Yes;
        data = lispmds_export(aChart, aProgramName);
    }
    else
        throw import_error{"[acmacs::chart::export_factory]: cannot infer export format from extension of " + aFilename};

    if (data.empty())
        throw export_error("No data to write to " + aFilename);

    Timeit ti_file("writing " + aFilename + ": ");
    acmacs::file::write(aFilename, data, force_compression);

} // acmacs::chart::import_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
