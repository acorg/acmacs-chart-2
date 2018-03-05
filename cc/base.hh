#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "acmacs-base/rjson.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/indexes.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace internal
    {
        class string_data : public std::string
        {
         public:
            string_data() = default;
            string_data(const std::string& src) : std::string(src) {}
            string_data(std::string&& src) : std::string(std::move(src)) {}
            string_data(std::string_view src) : std::string(src) {}
            string_data(const char* src) : std::string(src) {}
            string_data(const rjson::value& aSrc) : std::string(aSrc.str()) {}
            string_data(const string_data&) = default;
            string_data(string_data&&) = default;
            using std::string::operator=;
            string_data& operator=(const string_data&) = default;
            string_data& operator=(string_data&&) = default;
            int compare(const string_data& other) const { return ::string::compare(*this, other); }

        }; // class string_data

        inline bool operator==(const string_data& left, const char* right) { return ::string::compare(left, right) == 0; }
        inline bool operator!=(const string_data& left, const char* right) { return !operator==(left, right); }

// ----------------------------------------------------------------------

        template <typename T> class T_list_data
        {
         public:
            T_list_data() = default;
            T_list_data(size_t aSize) : mData(aSize) {}
            T_list_data(const rjson::array& aSrc) : mData(aSrc.begin(), aSrc.end()) {}
            T_list_data(const rjson::value& aSrc) : T_list_data(static_cast<const rjson::array&>(aSrc)) {}
            T_list_data(const std::vector<T>& aSrc) : mData(aSrc) {}
            template <typename Iter> T_list_data(Iter first, Iter last) : mData(static_cast<size_t>(last - first)) { std::transform(first, last, mData.begin(), [](const auto& src) -> T { return src; }); }
            template <typename Iter> T_list_data(Iter first, Iter last, std::function<T (const typename Iter::value_type&)> convert) : mData(static_cast<size_t>(last - first)) { std::transform(first, last, mData.begin(), convert); }

            constexpr bool empty() const { return mData.empty(); }
            constexpr size_t size() const { return mData.size(); }
            constexpr bool exist(const T& val) const { return std::find(mData.begin(), mData.end(), val) != mData.end(); }
            constexpr const std::vector<T>& data() const noexcept { return mData; }
            constexpr std::vector<T>& data() noexcept { return mData; }
            constexpr operator const std::vector<T>&() const noexcept { return mData; }
            constexpr T& operator[](size_t aIndex) { return mData.at(aIndex); }
            constexpr const T& operator[](size_t aIndex) const { return mData.at(aIndex); }
            constexpr auto begin() const { return mData.cbegin(); }
            constexpr auto end() const { return mData.cend(); }
            constexpr auto begin() { return mData.begin(); }
            constexpr auto end() { return mData.end(); }
            constexpr auto rbegin() { return mData.rbegin(); }
            constexpr auto rend() { return mData.rend(); }

            bool operator==(const T_list_data<T>& other) const
                {
                    if (size() != other.size())
                        return false;
                    for (size_t i = 0; i < size(); ++i) {
                        if (!(operator[](i) == other[i]))
                            return false;
                    }
                    return true;
                }
            bool operator!=(const T_list_data<T>& other) const { return ! operator==(other); }

            constexpr void push_back(const T& val) { mData.push_back(val); }
            constexpr void push_back(T&& val) { mData.push_back(std::forward<T>(val)); }
            constexpr typename std::vector<T>::iterator erase(typename std::vector<T>::iterator to_erase) { return mData.erase(to_erase); }

         private:
            std::vector<T> mData;

        }; // T_list_data<>

        template <> inline T_list_data<std::string>::T_list_data(const rjson::array& aSrc)
            : mData(aSrc.size())
        {
            std::transform(aSrc.begin(), aSrc.end(), mData.begin(), [](const auto& src) -> std::string { return src.str(); });
        }

        class string_list_data : public T_list_data<std::string>
        {
         public:
            using T_list_data<std::string>::T_list_data;

            std::string join() const { return ::string::join(" ", begin(), end()); }
            size_t total_length() const { return std::accumulate(begin(), end(), size_t{0}, [](size_t sum, const auto& element) { return sum + element.size(); }); }
            void push_back(const std::string& val) { if (!val.empty()) T_list_data<std::string>::push_back(val); }
            void push_back(std::string&& val) { if (!val.empty()) T_list_data<std::string>::push_back(std::move(val)); }

        }; // class string_list_data

        using double_list_data = T_list_data<double>;

        class index_list_data : public T_list_data<size_t>
        {
         public:
            using T_list_data<size_t>::T_list_data;

            void remove_indexes(const Indexes& to_remove, size_t base_index = 0)
                {
                    for (const auto index : to_remove) {
                        if (const auto found = std::find(begin(), end(), index + base_index); found != end())
                            erase(found);
                    }
                }
        };

// ----------------------------------------------------------------------

        template <typename Parent, typename Reference> class iterator
        {
         public:
            using reference = Reference;
            using pointer = typename std::add_pointer<Reference>::type;
            using value_type = typename std::remove_reference<Reference>::type;
            using difference_type = ssize_t;
            using iterator_category = std::random_access_iterator_tag;

            constexpr iterator& operator++() { ++mIndex; return *this; }
            constexpr iterator& operator+=(difference_type n) { mIndex += n; return *this; }
            constexpr iterator& operator-=(difference_type n) { mIndex -= n; return *this; }
            constexpr iterator operator-(difference_type n) { iterator temp = *this; return temp -= n; }
            constexpr difference_type operator-(const iterator& rhs) { return mIndex - rhs.mIndex; }
            constexpr bool operator==(const iterator& other) const { return &mParent == &other.mParent && mIndex == other.mIndex; }
            constexpr bool operator!=(const iterator& other) const { return &mParent != &other.mParent || mIndex != other.mIndex; }
            constexpr reference operator*() { return mParent[mIndex]; }
            constexpr size_t index() const { return mIndex; }
            constexpr bool operator<(const iterator& rhs) const { return mIndex < rhs.mIndex; }
            constexpr bool operator<=(const iterator& rhs) const { return mIndex <= rhs.mIndex; }
            constexpr bool operator>(const iterator& rhs) const { return mIndex > rhs.mIndex; }
            constexpr bool operator>=(const iterator& rhs) const { return mIndex >= rhs.mIndex; }

         private:
            iterator(const Parent& aParent, size_t aIndex) : mParent{aParent}, mIndex{aIndex} {}

            const Parent& mParent;
            size_t mIndex;

            friend Parent;
        };

    } // namespace internal

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const acmacs::chart::internal::string_data& value) { return value; }

    template <typename T> inline std::string to_string(const acmacs::chart::internal::T_list_data<T>& value) { return to_string(value.data()); }

    namespace chart::internal
    {
        template <typename T> inline std::ostream& operator<<(std::ostream& out, const T_list_data<T>& a) { return out << to_string(a); }
    }

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
