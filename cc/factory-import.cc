#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/ace-import.hh"
#include "acmacs-chart-2/acd1-import.hh"
#include "acmacs-chart-2/lispmds-import.hh"

// ----------------------------------------------------------------------

acmacs::chart::ChartP acmacs::chart::import_factory(std::string aFilename, Verify aVerify, report_time aReport)
{
    Timeit ti("reading chart from " + aFilename + ": ", aReport);
      // Timeit ti_file("reading " + aFilename + ": ");
    const std::string data = acmacs::file::read(aFilename);
    const std::string_view data_view(data);
      // ti_file.report();
    if (acmacs::chart::is_ace(data_view))
        return acmacs::chart::ace_import(data_view, aVerify);
    if (acmacs::chart::is_acd1(data_view))
        return acmacs::chart::acd1_import(data_view, aVerify);
    if (acmacs::chart::is_lispmds(data_view))
        return acmacs::chart::lispmds_import(data_view, aVerify);
    throw import_error{"[acmacs::chart::import_factory]: unrecognized file content: " + aFilename};

} // acmacs::chart::import_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
