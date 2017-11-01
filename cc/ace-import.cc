#include <iostream>

#include "acmacs-chart/ace-import.hh"

using namespace acmacs::chart;

// ----------------------------------------------------------------------

std::shared_ptr<Chart> acmacs::chart::ace_import(const std::string_view& aData, bool aVerify)
{
    auto chart = std::make_shared<AceChart>(rjson::parse_string(aData));
    if (aVerify)
        chart->verify_data();
    return chart;

} // acmacs::chart::ace_import

// ----------------------------------------------------------------------

void AceChart::verify_data() const
{
    std::cerr << "WARNING: AceChart::verify_data not implemented\n";

} // AceChart::verify_data

// ----------------------------------------------------------------------

std::shared_ptr<Info> AceChart::info() const
{

} // AceChart::info

// ----------------------------------------------------------------------

std::shared_ptr<Antigens> AceChart::antigens() const
{

} // AceChart::antigens

// ----------------------------------------------------------------------

std::shared_ptr<Sera> AceChart::sera() const
{

} // AceChart::sera

// ----------------------------------------------------------------------

std::shared_ptr<Projections> AceChart::projections() const
{

} // AceChart::projections

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
