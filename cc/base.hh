#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "acmacs-base/rjson.hh"

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

            inline const std::string& data() { return mData; }
            inline operator const std::string&() { return mData; }

         private:
            std::string mData;

        }; // class string_data

// ----------------------------------------------------------------------

        class string_list_data
        {
         public:
            inline string_list_data() = default;
            inline string_list_data(const rjson::value& aSrc) : mData(static_cast<const rjson::array&>(aSrc).begin(), static_cast<const rjson::array&>(aSrc).end()) {}
            // inline string_list_data(const std::string& aSrc) : mData{aSrc} {}
            // inline string_list_data(std::string&& aSrc) : mData{std::move(aSrc)} {}

            inline const std::vector<std::string>& data() { return mData; }
            inline operator const std::vector<std::string>&() { return mData; }

         private:
            std::vector<std::string> mData;

        }; // class string_data
    } // namespace internal

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
