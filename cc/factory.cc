#include "acmacs-base/read-file.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart/factory.hh"
#include "acmacs-chart/ace-import.hh"

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::Chart> acmacs::chart::factory(std::string aFilename, bool aVerify)
{
    Timeit ti("reading chart from " + aFilename + ": ");
    Timeit ti_file("reading " + aFilename + ": ");
    const auto data = acmacs_base::read_file(aFilename, true);
    ti_file.report();
    if (acmacs::chart::is_ace(data))
        return acmacs::chart::ace_import(data, aVerify);
    throw import_error{"[chart::factory]: unrecognized file content: " + aFilename};

} // acmacs::chart::factory

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
