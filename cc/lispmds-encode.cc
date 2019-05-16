#include <cctype>
#include <regex>

#include "acmacs-base/string.hh"
#include "acmacs-chart-2/lispmds-encode.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

static std::string sEncodedSignature_strain_name_lc{"!b"};
static std::string sEncodedSignature_strain_name_uc{"!B"};
static std::string sEncodedSignature_table_name_lc{"@b"};
static std::string sEncodedSignature_table_name_uc{"@B"};

static std::string sEncodedSignature_strain_name_lc_v1{"/a"};
static std::string sEncodedSignature_strain_name_uc_v1{"/A"};
static std::string sEncodedSignature_table_name_lc_v1{"@a"};
static std::string sEncodedSignature_table_name_uc_v1{"@A"};

#pragma GCC diagnostic pop

inline bool strain_name_encoded(const std::string& name)
{
    auto eqc = [](std::string_view tail) { return tail == sEncodedSignature_strain_name_lc || tail == sEncodedSignature_strain_name_uc || tail == sEncodedSignature_strain_name_lc_v1 || tail == sEncodedSignature_strain_name_uc_v1; };
    return name.size() > 2 && eqc(std::string_view(name.data() + name.size() - 2, 2));
}

inline bool table_name_encoded(const std::string& name)
{
    auto eqc = [](std::string_view tail) { return tail == sEncodedSignature_table_name_lc || tail == sEncodedSignature_table_name_uc || tail == sEncodedSignature_table_name_lc_v1 || tail == sEncodedSignature_table_name_uc_v1; };
    return name.size() > 2 && eqc(std::string_view(name.data() + name.size() - 2, 2));
}

#include "acmacs-base/global-constructors-push.hh"
static std::regex sFluASubtype{"AH([0-9]+N[0-9]+).*"};
static std::regex sPassageDate{".+ ([12][90][0-9][0-9]-[0-2][0-9]-[0-3][0-9])"};
#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

static inline std::string append_signature(std::string aSource, acmacs::chart::lispmds_encoding_signature signature)
{
    using namespace acmacs::chart;

    switch (signature) {
      case lispmds_encoding_signature::no:
          return aSource;
      case lispmds_encoding_signature::strain_name:
          return aSource + sEncodedSignature_strain_name_lc;
      case lispmds_encoding_signature::table_name:
          return aSource + sEncodedSignature_table_name_lc;
    }
    return aSource;
}

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_encode(std::string aName, lispmds_encoding_signature signature)
{
    std::string result;
    bool encoded = false;
    for (auto c : aName) {
        switch (c) {
            case ' ':
                result.append(1, '_');
                encoded = true;
                break;
            case '(':
            case ')':
                encoded = true;
                break;
            case ',':
                result.append("..");
                encoded = true;
                break;
            case '%': // we use it as a hex code prefix
            case ':': // : is a symbol module separator in lisp
            case '$': // tk tries to subst var when sees $
            case '?': // The "?" in strain names causes an issue with strain matching. (Blake 2018-06-11)
            case '"':
            case '\'':
            case '`':
            case ';':
            case '[':
            case ']':
            case '{':
            case '}':
            case '#': // special character in lisp that get expanded before the usual readers sees them
            case '|': // special character in lisp that get expanded before the usual readers sees them
                result.append("%" + acmacs::to_hex_string(c, acmacs::NotShowBase));
                encoded = true;
                break;
            case '/':
            case '~': // perhaps avoid in the table name
                if (signature == lispmds_encoding_signature::table_name) {
                    result.append("%" + acmacs::to_hex_string(c, acmacs::NotShowBase));
                    encoded = true;
                }
                else {
                    result.append(1, c);
                }
                break;
            case '+': // approved by Derek on 2018-06-11
            case '-':
            case '*':
            case '@':
            case '^':
            case '&':
            case '_':
            case '=':
            case '<':
            case '>':
            case '.':
            case '!':
            default:
                result.append(1, c);
                break;
        }
    }
    return encoded ? append_signature(result, signature) : result;

} // acmacs::chart::lispmds_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_table_name_encode(std::string name)
{
      // lispmds does not like / in the table name
      // it interprets / as being part of a file name when we doing procrustes (Blake 2018-06-11)
    return lispmds_encode(name, lispmds_encoding_signature::table_name);

} // acmacs::chart::lispmds_table_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_antigen_name_encode(const Name& aName, const acmacs::virus::Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations, lispmds_encoding_signature signature)
{
    std::string result = lispmds_encode(aName, lispmds_encoding_signature::no);
    if (!aReassortant.empty())
        result += "_r!" + lispmds_encode(aReassortant, lispmds_encoding_signature::no);
    if (!aPassage.empty())
        result += "_p!" + lispmds_encode(aPassage, lispmds_encoding_signature::no);
    for (const auto& anno: aAnnotations)
        result += "_a!" + lispmds_encode(anno, lispmds_encoding_signature::no);
    return append_signature(result, signature);

} // acmacs::chart::lispmds_antigen_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_serum_name_encode(const Name& aName, const acmacs::virus::Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId, lispmds_encoding_signature signature)
{
    std::string result = lispmds_encode(aName, lispmds_encoding_signature::no);
    if (!aReassortant.empty())
        result += "_r!" + lispmds_encode(aReassortant, lispmds_encoding_signature::no);
    for (const auto& anno: aAnnotations)
        result += "_a!" + lispmds_encode(anno, lispmds_encoding_signature::no);
    if (!aSerumId.empty())
        result += "_i!" + lispmds_encode(aSerumId, lispmds_encoding_signature::no);
    return append_signature(result, signature);

} // acmacs::chart::lispmds_serum_name_encode

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_decode(std::string aName)
{
    if (strain_name_encoded(aName) || table_name_encoded(aName)) {
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

static inline std::vector<size_t> find_sep(std::string source, bool v1)
{
    std::vector<size_t> sep_pos;
    for (size_t pos = 0; pos < (source.size() - 2); ++pos) {
        if (source[pos] == ' ' && (v1 || source[pos+2] == '!')) {
            switch (std::tolower(source[pos + 1])) {
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

void acmacs::chart::lispmds_antigen_name_decode(std::string aSource, Name& aName, acmacs::virus::Reassortant& aReassortant, Passage& aPassage, Annotations& aAnnotations)
{
    if (strain_name_encoded(aSource)) {
        const bool v1 = std::tolower(aSource.back()) == 'a';
        const size_t sep_len = v1 ? 2 : 3;
        const std::string stage1 = lispmds_decode(aSource);
        auto sep_pos = find_sep(stage1, v1);
        if (!sep_pos.empty()) {
            aName = stage1.substr(0, sep_pos[0]);
            for (size_t sep_no = 0; sep_no < sep_pos.size(); ++sep_no) {
                const size_t chunk_len = (sep_no + 1) < sep_pos.size() ? (sep_pos[sep_no + 1] - sep_pos[sep_no] - sep_len) : std::string::npos;
                switch (std::tolower(stage1[sep_pos[sep_no] + 1])) {
                  case 'r':
                      aReassortant = acmacs::virus::Reassortant{stage1.substr(sep_pos[sep_no] + sep_len, chunk_len)};
                      break;
                  case 'p':
                      aPassage = Passage{fix_passage_date(stage1.substr(sep_pos[sep_no] + sep_len, chunk_len))};
                      break;
                  case 'a':
                      aAnnotations.push_back(stage1.substr(sep_pos[sep_no] + sep_len, chunk_len));
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

void acmacs::chart::lispmds_serum_name_decode(std::string aSource, Name& aName, acmacs::virus::Reassortant& aReassortant, Annotations& aAnnotations, SerumId& aSerumId)
{
    if (strain_name_encoded(aSource)) {
        const bool v1 = std::tolower(aSource.back()) == 'a';
        const size_t sep_len = v1 ? 2 : 3;
        const std::string stage1 = lispmds_decode(aSource);
        auto sep_pos = find_sep(stage1, v1);
        if (!sep_pos.empty()) {
            aName = stage1.substr(0, sep_pos[0]);
            for (size_t sep_no = 0; sep_no < sep_pos.size(); ++sep_no) {
                const size_t chunk_len = (sep_no + 1) < sep_pos.size() ? (sep_pos[sep_no + 1] - sep_pos[sep_no] - sep_len) : std::string::npos;
                switch (std::tolower(stage1[sep_pos[sep_no] + 1])) {
                  case 'r':
                      aReassortant = acmacs::virus::Reassortant{stage1.substr(sep_pos[sep_no] + sep_len, chunk_len)};
                      break;
                  case 'i':
                      aSerumId = stage1.substr(sep_pos[sep_no] + sep_len, chunk_len);
                      break;
                  case 'a':
                      aAnnotations.push_back(stage1.substr(sep_pos[sep_no] + sep_len, chunk_len));
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
