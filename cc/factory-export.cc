#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart/factory-export.hh"
#include "acmacs-chart/ace-export.hh"

// ----------------------------------------------------------------------

void acmacs::chart::export_factory(std::shared_ptr<acmacs::chart::Chart> aChart, std::string aFilename, std::string aProgramName)
{
    Timeit ti("writing chart to " + aFilename + ": ");
    auto force_compression = acmacs_base::ForceCompression::No;
    std::string data;
    if (fs::path(aFilename).extension() == ".ace") {
        force_compression = acmacs_base::ForceCompression::Yes;
        data = ace_export(aChart, aProgramName);
    }
    else
        throw import_error{"[acmacs::chart::export_factory]: cannot infer export format from extension of " + aFilename};

    if (data.empty())
        throw export_error("No data to write to " + aFilename);

    Timeit ti_file("writing " + aFilename + ": ");
    acmacs_base::write_file(aFilename, data, force_compression);

} // acmacs::chart::import_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
