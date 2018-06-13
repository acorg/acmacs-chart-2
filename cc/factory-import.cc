#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/ace-import.hh"
#include "acmacs-chart-2/acd1-import.hh"
#include "acmacs-chart-2/lispmds-import.hh"

// ----------------------------------------------------------------------

acmacs::chart::ChartP acmacs::chart::import_from_decompressed_data(std::string aData, Verify aVerify, report_time aReport)
{
    Timeit ti("reading chart from data: ", aReport);
    const std::string_view data_view(aData);
    if (acmacs::chart::is_ace(data_view))
        return acmacs::chart::ace_import(data_view, aVerify);
    if (acmacs::chart::is_acd1(data_view))
        return acmacs::chart::acd1_import(data_view, aVerify);
    if (acmacs::chart::is_lispmds(data_view))
        return acmacs::chart::lispmds_import(data_view, aVerify);
    throw import_error{"[acmacs::chart::import_from_data]: unrecognized file content"};

} // acmacs::chart::import_from_decompressed_data

// ----------------------------------------------------------------------

acmacs::chart::ChartP acmacs::chart::import_from_data(std::string_view aData, Verify aVerify, report_time aReport)
{
    return import_from_decompressed_data(acmacs::file::decompress_if_necessary(aData), aVerify, aReport);

} // acmacs::chart::import_from_data

// ----------------------------------------------------------------------

acmacs::chart::ChartP acmacs::chart::import_from_data(std::string aData, Verify aVerify, report_time aReport)
{
    return import_from_decompressed_data(acmacs::file::decompress_if_necessary(aData), aVerify, aReport);

} // acmacs::chart::import_from_data

// ----------------------------------------------------------------------

acmacs::chart::ChartP acmacs::chart::import_from_file(std::string aFilename, Verify aVerify, report_time aReport)
{
    Timeit ti("reading chart from " + aFilename + ": ", aReport);
    try {
        return import_from_decompressed_data(acmacs::file::read(aFilename), aVerify, report_time::No);
    }
    catch (acmacs::file::not_found&) {
        throw import_error{"[acmacs::chart::import_from_file]: file not found: " + aFilename};
    }
    catch (std::exception& err) {
        throw import_error{"[acmacs::chart::import_from_file]: unrecognized file content: " + aFilename + ": " + err.what()};
    }

} // acmacs::chart::import_from_file

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
