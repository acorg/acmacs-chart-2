#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "acmacs-base/rjson.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/stream.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace internal
    {
        class string_data
        {
         public:
            inline string_data() = default;
            inline string_data(const std::string& aSrc) : mData{aSrc} {}
            inline string_data(std::string&& aSrc) : mData{std::move(aSrc)} {}
            inline string_data(const char* aSrc) : mData{aSrc} {}
            inline string_data(const rjson::value& aSrc) : mData{static_cast<std::string>(aSrc)} {}

            inline const std::string& data() const noexcept { return mData; }
            inline operator const std::string&() const noexcept { return mData; }

         private:
            std::string mData;

        }; // class string_data

// ----------------------------------------------------------------------

        template <typename T> class T_list_data
        {
         public:
            inline T_list_data() = default;
            inline T_list_data(size_t aSize) : mData(aSize) {}
            inline T_list_data(const rjson::array& aSrc) : mData(aSrc.begin(), aSrc.end()) {}
            inline T_list_data(const rjson::value& aSrc) : T_list_data(static_cast<const rjson::array&>(aSrc)) {}
            inline T_list_data(const std::vector<T>& aSrc) : mData(aSrc) {}
            template <typename Iter> inline T_list_data(Iter first, Iter last) : mData(static_cast<size_t>(last - first)) { std::transform(first, last, mData.begin(), [](const auto& src) -> T { return src; }); }
            template <typename Iter> inline T_list_data(Iter first, Iter last, std::function<T (const typename Iter::value_type&)> convert) : mData(static_cast<size_t>(last - first)) { std::transform(first, last, mData.begin(), convert); }

            inline bool empty() const { return mData.empty(); }
            inline size_t size() const { return mData.size(); }
            inline const std::vector<T>& data() const noexcept { return mData; }
            inline operator const std::vector<T>&() const noexcept { return mData; }
            inline T& operator[](size_t aIndex) { return mData.at(aIndex); }
            inline auto begin() const { return mData.cbegin(); }
            inline auto end() const { return mData.cend(); }
            inline auto begin() { return mData.begin(); }
            inline auto end() { return mData.end(); }

            inline void push_back(const T& val) { mData.push_back(val); }
            inline void push_back(T&& val) { mData.push_back(std::forward<T>(val)); }

         private:
            std::vector<T> mData;

        }; // T_list_data<>

        class string_list_data : public T_list_data<std::string>
        {
         public:
            using T_list_data<std::string>::T_list_data;

            inline std::string join() const { return string::join(" ", begin(), end()); }
            inline void push_back(const std::string& val) { if (!val.empty()) T_list_data<std::string>::push_back(val); }
            inline void push_back(std::string&& val) { if (!val.empty()) T_list_data<std::string>::push_back(std::move(val)); }

        }; // class string_list_data

        using index_list_data = T_list_data<size_t>;
        using double_list_data = T_list_data<double>;

    } // namespace internal

} // namespace acmacs::chart

// ----------------------------------------------------------------------

inline std::ostream& operator << (std::ostream& out, const acmacs::chart::internal::string_data& a)
{
    return out << a.data();
}

inline std::ostream& operator << (std::ostream& out, const acmacs::chart::internal::string_list_data& a)
{
    return out << a.data();
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
