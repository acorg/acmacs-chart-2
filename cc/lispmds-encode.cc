// #include <regex>

#include "acmacs-base/string.hh"
#include "acmacs-chart/lispmds-encode.hh"

// ----------------------------------------------------------------------

// #include "acmacs-base/global-constructors-push.hh"
// static std::regex sNotValidSymbol{R"([ \(\)'])"};
// #include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_encode(std::string aName)
{
    std::string result;
    for (auto c: aName) {
        switch (c) {
          case ' ':
              result.append(1, '_');
              break;
          case '(':
          case ')':
          case '\'':
          case '$':             // tk tries to subst var when sees $
              break;
          case ',':
              result.append("..");
              break;
          case ':':             // : is a symbol module separator in lisp
          case '%':
              result.append("%" + string::to_hex_string(c, string::NotShowBase));
              break;
          default:
              result.append(1, c);
              break;
        }
    }
    return result;

} // acmacs::chart::lispmds_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_antigen_name_encode(const Name& aName, const Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations)
{
    std::string result = lispmds_encode(aName);
    if (!aReassortant.empty())
        result += "_r" + lispmds_encode(aReassortant);
    if (!aPassage.empty())
        result += "_p" + lispmds_encode(aPassage);
    for (const auto& anno: aAnnotations)
        result += "_a" + lispmds_encode(anno);
    return result;

} // acmacs::chart::lispmds_antigen_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_serum_name_encode(const Name& aName, const Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId)
{
    std::string result = lispmds_encode(aName);
    if (!aReassortant.empty())
        result += "_r" + lispmds_encode(aReassortant);
    for (const auto& anno: aAnnotations)
        result += "_a" + lispmds_encode(anno);
    if (!aSerumId.empty())
        result += "_i" + lispmds_encode(aSerumId);
    return result;

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
