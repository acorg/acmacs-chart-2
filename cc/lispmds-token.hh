#pragma once

#include <variant>
#include <string>
#include <vector>

// ----------------------------------------------------------------------

namespace acmacs::lispmds
{
    class value;

    class nil
    {
     public:
        inline nil() = default;

    }; // class nil

    class boolean
    {
     public:
        inline boolean() = default;

     private:
        bool mValue;

    }; // class boolean

    class number
    {
     public:
        inline number() = default;

     private:
        std::string mValue;

    }; // class number

    class string
    {
     public:
        inline string() = default;

     private:
        std::string mValue;

    }; // class string

    class symbol
    {
     public:
        inline symbol() = default;

     private:
        std::string mValue;

    }; // class symbol

    class keyword
    {
     public:
        inline keyword() = default;

     private:
        std::string mValue;

    }; // class keyword

    class list
    {
     public:
        inline list() = default;

     private:
        std::vector<value> mContent;

    }; // class list

    using value_base = std::variant<nil, boolean, number, string, symbol, keyword, list>; // nil must be the first alternative, it is the default value;

    class value : public value_base
    {
     public:
        using value_base::operator=;
        using value_base::value_base;
        inline value(const value&) = default; // otherwise it is deleted
          //inline value(const value& aSrc) : value_base(aSrc) { std::cerr << "rjson::value copy " << aSrc.to_json() << '\n'; }
        inline value(value&&) = default;
          // inline value(value&& aSrc) : value_base(std::move(aSrc)) { std::cerr << "rjson::value move " << to_json() << '\n'; }
        inline value& operator=(const value&) = default; // otherwise it is deleted
        inline value& operator=(value&&) = default;
          // inline ~value() { std::cerr << "DEBUG: ~value " << to_json() << DEBUG_LINE_FUNC << '\n'; }

    }; // class value

// ----------------------------------------------------------------------

    value parse_string(const std::string_view& aData);

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
