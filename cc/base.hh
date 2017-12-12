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
            inline string_data(const std::string_view& aSrc) : mData{aSrc} {}
            inline string_data(std::string&& aSrc) : mData{std::move(aSrc)} {}
            inline string_data(const char* aSrc) : mData{aSrc} {}
            inline string_data(const rjson::value& aSrc) : mData{aSrc.str()} {}
            inline string_data(const string_data& aSrc) = default;
            inline string_data(string_data&& aSrc) = default;
            inline string_data& operator=(const std::string& aSrc) { mData = aSrc; return *this; }
            inline string_data& operator=(std::string&& aSrc) { mData = std::move(aSrc); return *this; }
            inline string_data& operator=(const string_data& aSrc) = default;
            inline string_data& operator=(string_data&& aSrc) = default;

            inline bool empty() const noexcept { return mData.empty(); }
            inline size_t size() const noexcept { return mData.size(); }
            inline char operator[](size_t index) const noexcept { return mData[index]; }
            inline bool operator == (const string_data& other) const { return data() == other.data(); }
            inline bool operator != (const string_data& other) const { return !operator==(other); }
            inline auto find(const char* s) const noexcept { return mData.find(s); }

            constexpr inline const std::string& data() const noexcept { return mData; }
            constexpr inline operator const std::string&() const noexcept { return mData; }

         private:
            std::string mData;

        }; // class string_data

        inline std::ostream& operator << (std::ostream& out, const string_data& a) { return out << a.data(); }

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

            constexpr inline bool empty() const { return mData.empty(); }
            constexpr inline size_t size() const { return mData.size(); }
            constexpr inline const std::vector<T>& data() const noexcept { return mData; }
            constexpr inline std::vector<T>& data() noexcept { return mData; }
            constexpr inline operator const std::vector<T>&() const noexcept { return mData; }
            constexpr inline T& operator[](size_t aIndex) { return mData.at(aIndex); }
            constexpr inline const T& operator[](size_t aIndex) const { return mData.at(aIndex); }
            constexpr inline auto begin() const { return mData.cbegin(); }
            constexpr inline auto end() const { return mData.cend(); }
            constexpr inline auto begin() { return mData.begin(); }
            constexpr inline auto end() { return mData.end(); }
            constexpr inline auto rbegin() { return mData.rbegin(); }
            constexpr inline auto rend() { return mData.rend(); }

            inline bool operator==(const T_list_data<T>& other) const
                {
                    if (size() != other.size())
                        return false;
                    for (size_t i = 0; i < size(); ++i) {
                        if (!(operator[](i) == other[i]))
                            return false;
                    }
                    return true;
                }
            inline bool operator!=(const T_list_data<T>& other) const { return ! operator==(other); }

            constexpr inline void push_back(const T& val) { mData.push_back(val); }
            constexpr inline void push_back(T&& val) { mData.push_back(std::forward<T>(val)); }

         private:
            std::vector<T> mData;

        }; // T_list_data<>

        template <> inline T_list_data<std::string>::T_list_data(const rjson::array& aSrc)
            : mData(aSrc.size())
        {
            std::transform(aSrc.begin(), aSrc.end(), mData.begin(), [](const auto& src) -> std::string { return src; });
        }

        class string_list_data : public T_list_data<std::string>
        {
         public:
            using T_list_data<std::string>::T_list_data;

            inline std::string join() const { return ::string::join(" ", begin(), end()); }
            inline void push_back(const std::string& val) { if (!val.empty()) T_list_data<std::string>::push_back(val); }
            inline void push_back(std::string&& val) { if (!val.empty()) T_list_data<std::string>::push_back(std::move(val)); }

        }; // class string_list_data

        using index_list_data = T_list_data<size_t>;
        using double_list_data = T_list_data<double>;

        template <typename T> inline std::ostream& operator << (std::ostream& out, const T_list_data<T>& a) { return ::operator<<(out, a.data()); }

// ----------------------------------------------------------------------

        template <typename Parent, typename Reference> class iterator
        {
         public:
            using reference = Reference;
            using pointer = typename std::add_pointer<Reference>::type;
            using value_type = typename std::remove_reference<Reference>::type;
            using difference_type = ssize_t;
            using iterator_category = std::random_access_iterator_tag;

            constexpr inline iterator& operator++() { ++mIndex; return *this; }
            constexpr inline iterator& operator+=(difference_type n) { mIndex += n; return *this; }
            constexpr inline iterator& operator-=(difference_type n) { mIndex -= n; return *this; }
            constexpr inline iterator operator-(difference_type n) { iterator temp = *this; return temp -= n; }
            constexpr inline difference_type operator-(const iterator& rhs) { return mIndex - rhs.mIndex; }
            constexpr inline bool operator==(const iterator& other) const { return &mParent == &other.mParent && mIndex == other.mIndex; }
            constexpr inline bool operator!=(const iterator& other) const { return &mParent != &other.mParent || mIndex != other.mIndex; }
            constexpr inline reference operator*() { return mParent[mIndex]; }
            constexpr inline size_t index() const { return mIndex; }
            constexpr inline bool operator<(const iterator& rhs) const { return mIndex < rhs.mIndex; }
            constexpr inline bool operator<=(const iterator& rhs) const { return mIndex <= rhs.mIndex; }
            constexpr inline bool operator>(const iterator& rhs) const { return mIndex > rhs.mIndex; }
            constexpr inline bool operator>=(const iterator& rhs) const { return mIndex >= rhs.mIndex; }

         private:
            inline iterator(const Parent& aParent, size_t aIndex) : mParent{aParent}, mIndex{aIndex} {}

            const Parent& mParent;
            size_t mIndex;

            friend Parent;
        };

    } // namespace internal

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    template <> inline std::string to_string(const acmacs::chart::internal::string_data& value)
    {
        return value.data();
    }

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
