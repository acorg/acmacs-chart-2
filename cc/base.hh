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
            inline T_list_data(const rjson::array& aSrc) : mData(aSrc.begin(), aSrc.end()) {}
            inline T_list_data(const rjson::value& aSrc) : T_list_data(static_cast<const rjson::array&>(aSrc)) {}

            inline bool empty() const { return mData.empty(); }
            inline size_t size() const { return mData.size(); }
            inline const std::vector<T>& data() const noexcept { return mData; }
            inline operator const std::vector<T>&() const noexcept { return mData; }
            inline auto begin() const { return mData.begin(); }
            inline auto end() const { return mData.end(); }

         private:
            std::vector<T> mData;

        }; // T_list_data<>

        class string_list_data : public T_list_data<std::string>
        {
         public:
            using T_list_data<std::string>::T_list_data;

            inline std::string join() const { return string::join(" ", begin(), end()); }

        }; // class string_list_data

        using index_list_data = T_list_data<size_t>;

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
