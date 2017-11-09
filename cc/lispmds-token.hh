#pragma once

#include <variant>
#include <string>
#include <vector>
#include <stdexcept>

#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace acmacs::lispmds
{
    class type_mismatch : public std::runtime_error { public: using std::runtime_error::runtime_error; };

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

        inline operator bool() const { return mValue; }

     private:
        bool mValue;

    }; // class boolean

    class number
    {
     public:
        inline number() = default;
        inline number(std::string aValue) : mValue(aValue)
            {
                for (auto& c: mValue) {
                    switch (c) {
                      case 'd': case 'D':
                          c = 'e';
                          break;
                      default:
                          break;
                    }
                }
            }
        inline number(const std::string_view& aValue) : number(std::string{aValue}) {}

        inline operator double() const { return std::stod(mValue); }

     private:
        std::string mValue;

    }; // class number

    class string
    {
     public:
        inline string() = default;
        inline string(std::string aValue) : mValue(aValue) {}
        inline string(const std::string_view& aValue) : mValue(aValue) {}

        inline operator std::string() const { return mValue; }

     private:
        std::string mValue;

    }; // class string

    class symbol
    {
     public:
        inline symbol() = default;
        inline symbol(std::string aValue) : mValue(aValue) {}
        inline symbol(const std::string_view& aValue) : mValue(aValue) {}

        inline operator std::string() const { return mValue; }

     private:
        std::string mValue;

    }; // class symbol

    class keyword
    {
     public:
        inline keyword() = default;
        inline keyword(std::string aValue) : mValue(aValue) {}
        inline keyword(const std::string_view& aValue) : mValue(aValue) {}

        inline operator std::string() const { return mValue; }

     private:
        std::string mValue;

    }; // class keyword

    class list
    {
     public:
        inline list() = default;

        using iterator = decltype(std::declval<const std::vector<value>>().begin());
        using reverse_iterator = decltype(std::declval<const std::vector<value>>().rbegin());
        inline iterator begin() const { return mContent.begin(); }
        inline iterator end() const { return mContent.end(); }
        inline iterator begin() { return mContent.begin(); }
        inline iterator end() { return mContent.end(); }
        inline reverse_iterator rbegin() const { return mContent.rbegin(); }

        inline value& append(value&& to_add)
            {
                mContent.push_back(std::move(to_add));
                return mContent.back();
            }

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

        inline value& append(value&& to_add)
            {
                return std::visit([&](auto&& arg) -> value& {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, list>)
                        return arg.append(std::move(to_add));
                    else
                        throw type_mismatch{"not a lispmds::list, cannot append value"};
                    }, *this);
            }

    }; // class value

// ----------------------------------------------------------------------

    value parse_string(const std::string_view& aData);

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const lispmds::nil&)
    {
        return "NIL";
    }

    inline std::string to_string(const lispmds::boolean& val)
    {
        return val ? "t" : "f";
    }

    inline std::string to_string(const lispmds::number& val)
    {
        return acmacs::to_string(static_cast<double>(val));
    }

    inline std::string to_string(const lispmds::string& val)
    {
        return '"' + static_cast<std::string>(val) + '"';
    }

    inline std::string to_string(const lispmds::symbol& val)
    {
        return '\'' + static_cast<std::string>(val);
    }

    inline std::string to_string(const lispmds::keyword& val)
    {
        return static_cast<std::string>(val);
    }

    std::string to_string(const lispmds::value& val);

    inline std::string to_string(const lispmds::list& list)
    {
        std::string result{"(\n"};
        for (const lispmds::value& val: list) {
            result.append(to_string(val));
            result.append(1, '\n');
        }
        result.append(")\n");
        return result;
    }

    inline std::string to_string(const lispmds::value& val)
    {
        return std::visit([](auto&& arg) -> std::string { return to_string(arg); }, val);
    }

} // namespace acmacs

// ----------------------------------------------------------------------

inline std::ostream& operator<<(std::ostream& s, const acmacs::lispmds::value& val)
{
    return s << acmacs::to_string(val);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
