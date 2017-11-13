#include <regex>

#include "acmacs-base/string.hh"
#include "acmacs-chart/lispmds-encode.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
static std::regex sNotValidSymbol{R"([ \(\)'])"};
#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_encode(std::string aName)
{
    if (std::regex_search(aName, sNotValidSymbol))
        aName = '|' + aName + '|';
    return aName;

} // acmacs::chart::lispmds_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_antigen_name_encode(const Name& aName, const Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations)
{
    return lispmds_encode(string::join(" ", {aName, aReassortant, aPassage, string::join(" ", aAnnotations)}));

} // acmacs::chart::lispmds_antigen_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_serum_name_encode(const Name& aName, const Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId)
{
    return lispmds_encode(string::join(" ", {aName, aReassortant, string::join(" ", aAnnotations), aSerumId}));

} // acmacs::chart::lispmds_serum_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_decode(std::string aName)
{
    return aName;

} // acmacs::chart::lispmds_decode

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
