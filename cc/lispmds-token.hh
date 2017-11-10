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
    class keyword_no_found : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class keyword_has_no_value : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class value;

    class nil
    {
     public:
        inline nil() = default;

    }; // class nil

    class boolean
    {
     public:
        inline boolean(bool aValue = false) : mValue{aValue} {}

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

    namespace internal
    {
        class string
        {
         public:
            inline string() = default;
            inline string(std::string aValue) : mValue(aValue) {}
            inline string(const std::string_view& aValue) : mValue(aValue) {}

            inline operator std::string() const { return mValue; }
            inline bool operator==(const string& s) const { return mValue == s.mValue; }
            inline bool operator!=(const string& s) const { return mValue != s.mValue; }
            inline bool operator==(std::string s) const { return mValue == s; }
            inline bool operator!=(std::string s) const { return mValue != s; }

         private:
            std::string mValue;

        }; // class string

    } // namespace internal

    class string : public internal::string { public: using internal::string::string; };

    class symbol : public internal::string { public: using internal::string::string; };

    class keyword : public internal::string { public: using internal::string::string; };

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

        inline const value& operator[](size_t aIndex) const
            {
                return mContent.at(aIndex);
            }

        const value& operator[](std::string aKeyword) const;

        inline size_t size() const
            {
                return mContent.size();
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

        value& append(value&& to_add);
        const value& operator[](size_t aIndex) const;
        const value& operator[](std::string aKeyword) const;
        size_t size() const;
        bool empty() const { return size() == 0; }

        inline operator const list&() const { return std::get<list>(*this); }
          // inline operator const number&() const { return std::get<number>(*this); } // leads to ambiguity for operator[]: bug in llvm 5.0?
        inline operator std::string() const
            {
                try {
                    return std::get<string>(*this);
                }
                catch (std::bad_variant_access&) {
                    return std::get<symbol>(*this);
                }
            }

    }; // class value

// ----------------------------------------------------------------------

    inline const value& list::operator[](std::string aKeyword) const
    {
        auto p = mContent.begin();
        while (p != mContent.end() && (!std::get_if<keyword>(&*p) || std::get<keyword>(*p) != aKeyword))
            ++p;
        if (p == mContent.end())
            throw keyword_no_found{aKeyword};
        ++p;
        if (p == mContent.end())
            throw keyword_has_no_value{aKeyword};
        return *p;
    }

// ----------------------------------------------------------------------

    value parse_string(const std::string_view& aData);

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// gcc-7 support
// ----------------------------------------------------------------------

#if __GNUC__ == 7
namespace std
{
      // gcc 7.2 wants those, if we derive from std::variant
    template<> struct variant_size<acmacs::lispmds::value> : variant_size<acmacs::lispmds::value_base> {};
    template<size_t _Np> struct variant_alternative<_Np, acmacs::lispmds::value> : variant_alternative<_Np, acmacs::lispmds::value_base> {};
}
#endif

// ----------------------------------------------------------------------

namespace acmacs::lispmds
{
      // gcc 7.2 wants the following functions to be defined here (not inside the class

    inline value& value::append(value&& to_add)
    {
        return std::visit([&](auto&& arg) -> value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.append(std::move(to_add));
            else
                throw type_mismatch{"not a lispmds::list, cannot append value"};
        }, *this);
    }

    inline const value& value::operator[](size_t aIndex) const
    {
        return std::visit([aIndex](const auto& arg) -> const value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg[aIndex];
            else
                throw type_mismatch{"not a lispmds::list, cannot use [index]"};
        }, *this);
    }

    inline const value& value::operator[](std::string aKeyword) const
    {
        return std::visit([aKeyword](auto&& arg) -> const value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg[aKeyword];
            else
                throw type_mismatch{"not a lispmds::list, cannot use [keyword]"};
        }, *this);
    }

    inline size_t value::size() const
    {
        return std::visit([](auto&& arg) -> size_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.size();
            else if constexpr (std::is_same_v<T, nil>)
                return 0;
            else
                throw type_mismatch{"not a lispmds::list, cannot use size()"};
        }, *this);
    }

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const lispmds::nil&)
    {
        return "nil";
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
