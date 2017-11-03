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
            inline string_data(const rjson::value& aSrc) : mData{static_cast<std::string>(aSrc)} {}

            inline const std::string& data() const noexcept { return mData; }
            inline operator const std::string&() const noexcept { return mData; }

         private:
            std::string mData;

        }; // class string_data

// ----------------------------------------------------------------------

        class string_list_data
        {
         public:
            inline string_list_data() = default;
            inline string_list_data(const rjson::array& aSrc) : mData(aSrc.begin(), aSrc.end()) {}
            inline string_list_data(const rjson::value& aSrc) : string_list_data(static_cast<const rjson::array&>(aSrc)) {}

            inline const std::vector<std::string>& data() const noexcept { return mData; }
            inline operator const std::vector<std::string>&() const noexcept { return mData; }
            inline auto begin() const { return mData.begin(); }
            inline auto end() const { return mData.end(); }

            inline std::string join() const { return string::join(" ", begin(), end()); }

         private:
            std::vector<std::string> mData;

        }; // class string_list_data

// ----------------------------------------------------------------------

        class index_list_data
        {
         public:
            inline index_list_data() = default;
            inline index_list_data(const rjson::array& aSrc) : mData(aSrc.begin(), aSrc.end()) {}
            inline index_list_data(const rjson::value& aSrc) : index_list_data(static_cast<const rjson::array&>(aSrc)) {}

            inline const std::vector<size_t>& data() const noexcept { return mData; }
            inline operator const std::vector<size_t>&() const noexcept { return mData; }
            inline auto begin() const { return mData.begin(); }
            inline auto end() const { return mData.end(); }

         private:
            std::vector<size_t> mData;

        }; // class index_list_data

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
