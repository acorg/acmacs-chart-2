#include "acmacs-base/string.hh"
#include "acmacs-chart/lispmds-encode.hh"

// ----------------------------------------------------------------------

static const char* sEncodedSignature = "/a";

// ----------------------------------------------------------------------

static inline std::string append_signature(std::string aSource, bool add_signature)
{
    if (add_signature)
        return aSource + sEncodedSignature;
    else
        return aSource;
}

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_encode(std::string aName, bool add_signature)
{
    std::string result;
    for (auto c: aName) {
        switch (c) {
          case ' ':
              result.append(1, '_');
              break;
          case '(':
          case ')':
              break;
          case ',':
              result.append("..");
              break;
          case '\'':
          case ':':             // : is a symbol module separator in lisp
          case '%':
          case '$':             // tk tries to subst var when sees $
              result.append("%" + string::to_hex_string(c, string::NotShowBase));
              break;
          default:
              result.append(1, c);
              break;
        }
    }
    return append_signature(result, add_signature);

} // acmacs::chart::lispmds_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_antigen_name_encode(const Name& aName, const Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations, bool add_signature)
{
    std::string result = lispmds_encode(aName, false);
    if (!aReassortant.empty())
        result += "_r" + lispmds_encode(aReassortant, false);
    if (!aPassage.empty())
        result += "_p" + lispmds_encode(aPassage, false);
    for (const auto& anno: aAnnotations)
        result += "_a" + lispmds_encode(anno, false);
    return append_signature(result, add_signature);

} // acmacs::chart::lispmds_antigen_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_serum_name_encode(const Name& aName, const Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId, bool add_signature)
{
    std::string result = lispmds_encode(aName, false);
    if (!aReassortant.empty())
        result += "_r" + lispmds_encode(aReassortant, false);
    for (const auto& anno: aAnnotations)
        result += "_a" + lispmds_encode(anno, false);
    if (!aSerumId.empty())
        result += "_i" + lispmds_encode(aSerumId, false);
    return append_signature(result, add_signature);

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
