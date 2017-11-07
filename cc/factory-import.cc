#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart/factory-import.hh"
#include "acmacs-chart/ace-import.hh"
#include "acmacs-chart/acd1-import.hh"

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::Chart> acmacs::chart::import_factory(std::string aFilename, Verify aVerify)
{
    Timeit ti("reading chart from " + aFilename + ": ");
    Timeit ti_file("reading " + aFilename + ": ");
    const auto data = acmacs_base::read_file(aFilename, true);
    ti_file.report();
    if (acmacs::chart::is_ace(data))
        return acmacs::chart::ace_import(data, aVerify);
    if (acmacs::chart::is_acd1(data))
        return acmacs::chart::acd1_import(data, aVerify);
    throw import_error{"[acmacs::chart::import_factory]: unrecognized file content: " + aFilename};

} // acmacs::chart::import_factory

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
