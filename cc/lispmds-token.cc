#include <iostream>
#include <cctype>
#include <stack>

using namespace std::string_literals;

#include "acmacs-base/string.hh"
#include "acmacs-chart/verify.hh"
#include "acmacs-chart/lispmds-token.hh"

// ----------------------------------------------------------------------

class Tokenizer
{
 public:
    inline Tokenizer(const std::string_view& aData) : mData{aData} {}

    enum Token : char { End = 'E', Symbol = '\'', Keyword = ':', Number = 'N', String = 'S', OpenList = '(', CloseList = ')'};

    using Result = std::pair<Token, std::string_view>;

    Result next();

    static acmacs::lispmds::value to_value(Token token, const std::string_view& aText);

 private:
    std::string_view mData;
    size_t mPos = 0;

    void skip_spaces();
    void skip_until_eol();
    Result extract_number();
    Result extract_until(char ending, Token result);
    Result extract_symbol(Token result);

}; // class Tokenizer


// ----------------------------------------------------------------------

acmacs::lispmds::value acmacs::lispmds::parse_string(const std::string_view& aData)
{
    Tokenizer tokenizer(aData);
    if (tokenizer.next().first != Tokenizer::OpenList)
        throw acmacs::chart::import_error("[lispmds]: '(' expected at the beginning of the file");
    if (auto [token, text] = tokenizer.next(); token != Tokenizer::Symbol || text != "MAKE-MASTER-MDS-WINDOW")
        throw acmacs::chart::import_error("[lispmds]: \"(MAKE-MASTER-MDS-WINDOW\" expected at the beginning of the file");
    value result{list{}};
    std::stack<value*> stack;
    stack.push(&result);
    for (auto [token, text] = tokenizer.next(); token != Tokenizer::End; std::tie(token, text) = tokenizer.next()) {
        switch (token) {
          case Tokenizer::OpenList:
              stack.push(&stack.top()->append(list{}));
              break;
          case Tokenizer::CloseList:
              stack.pop();
              break;
          case Tokenizer::End:
              throw acmacs::lispmds::type_mismatch{"unexpected end of input in acmacs::lispmds::parse_string"};
          case Tokenizer::Symbol:
          case Tokenizer::Keyword:
          case Tokenizer::Number:
          case Tokenizer::String:
              stack.top()->append(Tokenizer::to_value(token, text));
              break;
        }
    }
    if (!stack.empty())
        throw acmacs::chart::import_error("[lispmds]: unexpected end of input");
    std::cout << "Lispmds value: " << result << '\n';
    return result;

} // acmacs::lispmds::parse_string

// ----------------------------------------------------------------------

acmacs::lispmds::value Tokenizer::to_value(Tokenizer::Token token, const std::string_view& aText)
{
    switch (token) {
      case End:
      case OpenList:
      case CloseList:
          throw acmacs::lispmds::type_mismatch{"unexpected token in Tokenizer::to_value"};
      case Symbol:
          return acmacs::lispmds::symbol(aText);
      case Keyword:
          return acmacs::lispmds::keyword(aText);
      case Number:
          return acmacs::lispmds::number(aText);
      case String:
          return acmacs::lispmds::string(aText);
    }

} // Tokenizer::to_value

// ----------------------------------------------------------------------

inline void Tokenizer::skip_spaces()
{
    while (std::isspace(mData[mPos]) && mPos < mData.size())
        ++mPos;

} // Tokenizer::skip_spaces

inline void Tokenizer::skip_until_eol()
{
    while (mData[mPos] != '\n' && mPos < mData.size())
        ++mPos;

} // Tokenizer::skip_until_eol

// ----------------------------------------------------------------------

Tokenizer::Result Tokenizer::next()
{
    while (mPos < mData.size()) {
        skip_spaces();
        if (mPos < mData.size()) {
            switch (mData[mPos]) {
              case ';':
                  skip_until_eol();
                  break;
              case '(':
                  ++mPos;
                  return {OpenList, {}};
              case ')':
                  ++mPos;
                  return {CloseList, {}};
              case '+': case '-':
              case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                  return extract_number();
              case '\'':
              case '`':
                  ++mPos;
                  break;
              case '"':
                  return extract_until('"', String);
              case '|':
                  return extract_until('|', Symbol);
              case ',':
                  throw std::runtime_error("Unsupported symbol "s + mData[mPos] + " at pos " + acmacs::to_string(mPos));
              case ':':
                  return extract_symbol(Keyword);
              default:
                  return extract_symbol(Symbol);
            }
        }
    }
    return {End, {}};

} // Tokenizer::next_token

// ----------------------------------------------------------------------

Tokenizer::Result Tokenizer::extract_number()
{
    const size_t first = mPos++;
    for (bool cont = true; cont && mPos < mData.size(); ) {
        switch (mData[mPos]) {
          case '+': case '-':
          case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
          case '.': case 'E': case 'e': case 'D': case 'd':
              ++mPos;
              break;
          default:
              cont = false;
              break;
        }
    }
    return {Number, mData.substr(first, mPos - first)};

} // Tokenizer::extract_number

// ----------------------------------------------------------------------

Tokenizer::Result Tokenizer::extract_until(char ending, Token result)
{
    const size_t first = ++mPos;
    while (mPos < mData.size() && mData[mPos] != ending)
        ++mPos;
    if (mPos < mData.size())
        ++mPos;
    return {result, mData.substr(first, mPos - first - 1)};

} // Tokenizer::extract_until

// ----------------------------------------------------------------------

Tokenizer::Result Tokenizer::extract_symbol(Token result)
{
    const size_t first = mPos++;
    for (bool cont = true; cont && mPos < mData.size();) {
        switch (mData[mPos]) {
          case ' ': case '\n': case '\t': case '(': case ')': case ',': case '\'': case '`':
              cont = false;
              break;
          case '\\':
              ++mPos;
              if (mPos < mData.size())
                  ++mPos;
              break;
          case '|':
              ++mPos;
              while (mPos < mData.size() && mData[mPos] != '|')
                  ++mPos;
              if (mPos < mData.size())
                  ++mPos;
              break;
          default:
              ++mPos;
              break;
        }
    }
    return {result, mData.substr(first, mPos - first)};

} // Tokenizer::extract_symbol

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: