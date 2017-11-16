#include <cctype>
#include <regex>

#include "acmacs-base/string.hh"
#include "acmacs-chart-2/lispmds-encode.hh"

// ----------------------------------------------------------------------

static const char* sEncodedSignature = "/a";

#include "acmacs-base/global-constructors-push.hh"
static std::regex sFluASubtype{"AH([0-9]+N[0-9]+).*"};
static std::regex sPassageDate{".+ ([12][90][0-9][0-9]-[0-2][0-9]-[0-3][0-9])"};
#include "acmacs-base/diagnostics-pop.hh"

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
    if (aName.size() > 2 && aName.substr(aName.size() - 2) == sEncodedSignature) {
        std::string result;
        bool last_was_space = true;
        for (size_t pos = 0; pos < (aName.size() - 2); ++pos) {
            switch (aName[pos]) {
              case '.':
                  if (aName[pos + 1] == '.') {
                      result.append(1, ',');
                      ++pos;
                  }
                  else
                      result.append(1, aName[pos]);
                  break;
              case '_':
                  result.append(1, ' ');
                  break;
              case '%':
                  if (std::isxdigit(aName[pos + 1]) && std::isxdigit(aName[pos + 2])) {
                      result.append(1, static_cast<char>(std::stoul(aName.substr(pos + 1, 2), nullptr, 16)));
                      pos += 2;
                  }
                  else
                      result.append(1, aName[pos]);
                  break;
              case 'A':
                  if (last_was_space && aName[pos + 1] == 'H') {
                      std::smatch flu_a_match;
                      if (const std::string text(aName, pos); std::regex_match(text, flu_a_match, sFluASubtype)) {
                          result.append("A(H").append(flu_a_match.str(1)).append(1, ')');
                          pos += static_cast<size_t>(flu_a_match.length(1)) + 1;
                      }
                      else
                          result.append(1, aName[pos]);
                  }
                  else
                      result.append(1, aName[pos]);
                  break;
              default:
                  result.append(1, aName[pos]);
                  break;
            }
            last_was_space = result.back() == ' ';
        }
        return result;
    }
    else
        return aName;

} // acmacs::chart::lispmds_decode

// ----------------------------------------------------------------------

static inline std::vector<size_t> find_sep(std::string source)
{
    std::vector<size_t> sep_pos;
    for (size_t pos = 0; pos < (source.size() - 1); ++pos) {
        if (source[pos] == ' ') {
            switch (source[pos + 1]) {
              case 'r':
              case 'p':
              case 'a':
              case 'i':
                  sep_pos.push_back(pos);
                  ++pos;
                  break;
            }
        }
    }
    return sep_pos;
}

// ----------------------------------------------------------------------

static inline std::string fix_passage_date(std::string source)
{
    if (std::smatch passage_date; std::regex_match(source, passage_date, sPassageDate)) {
        source.insert(static_cast<size_t>(passage_date.position(1)), 1, '(');
        source.append(1, ')');
    }
    return source;
}

void acmacs::chart::lispmds_antigen_name_decode(std::string aSource, Name& aName, Reassortant& aReassortant, Passage& aPassage, Annotations& aAnnotations)
{
    if (aSource.size() > 2 && aSource.substr(aSource.size() - 2) == sEncodedSignature) {
        const std::string stage1 = lispmds_decode(aSource);
        auto sep_pos = find_sep(stage1);
        if (!sep_pos.empty()) {
            aName = stage1.substr(0, sep_pos[0]);
            for (size_t sep_no = 0; sep_no < sep_pos.size(); ++sep_no) {
                const size_t chunk_len = (sep_no + 1) < sep_pos.size() ? (sep_pos[sep_no + 1] - sep_pos[sep_no] - 2) : std::string::npos;
                switch (stage1[sep_pos[sep_no] + 1]) {
                  case 'r':
                      aReassortant = stage1.substr(sep_pos[sep_no] + 2, chunk_len);
                      break;
                  case 'p':
                      aPassage = fix_passage_date(stage1.substr(sep_pos[sep_no] + 2, chunk_len));
                      break;
                  case 'a':
                      aAnnotations.push_back(stage1.substr(sep_pos[sep_no] + 2, chunk_len));
                      break;
                }
            }
        }
        else
            aName = stage1;
    }
    else
        aName = aSource;

} // acmacs::chart::lispmds_antigen_name_decode

// ----------------------------------------------------------------------

void acmacs::chart::lispmds_serum_name_decode(std::string aSource, Name& aName, Reassortant& aReassortant, Annotations& aAnnotations, SerumId& aSerumId)
{
    if (aSource.size() > 2 && aSource.substr(aSource.size() - 2) == sEncodedSignature) {
        const std::string stage1 = lispmds_decode(aSource);
        auto sep_pos = find_sep(stage1);
        if (!sep_pos.empty()) {
            aName = stage1.substr(0, sep_pos[0]);
            for (size_t sep_no = 0; sep_no < sep_pos.size(); ++sep_no) {
                const size_t chunk_len = (sep_no + 1) < sep_pos.size() ? (sep_pos[sep_no + 1] - sep_pos[sep_no] - 2) : std::string::npos;
                switch (stage1[sep_pos[sep_no] + 1]) {
                  case 'r':
                      aReassortant = stage1.substr(sep_pos[sep_no] + 2, chunk_len);
                      break;
                  case 'i':
                      aSerumId = stage1.substr(sep_pos[sep_no] + 2, chunk_len);
                      break;
                  case 'a':
                      aAnnotations.push_back(stage1.substr(sep_pos[sep_no] + 2, chunk_len));
                      break;
                }
            }
        }
        else
            aName = stage1;
    }
    else
        aName = aSource;

} // acmacs::chart::lispmds_serum_name_decode

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
