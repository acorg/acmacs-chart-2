#pragma once

#include "acmacs-chart-2/base.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
      // sorted list of indexes
    class PointIndexList : private detail::index_list_data
    {
     public:
        using difference_type = detail::index_list_data::difference_type;
        using detail::index_list_data::index_list_data;
        using detail::index_list_data::operator==;
        using detail::index_list_data::operator[];
        using detail::index_list_data::data;
        using detail::index_list_data::empty;
        using detail::index_list_data::size;
        using detail::index_list_data::erase;
        using detail::index_list_data::clear;
        using detail::index_list_data::begin;
        using detail::index_list_data::end;
        using detail::index_list_data::front;

        bool contains(size_t val) const
            {
                const auto found = std::lower_bound(begin(), end(), val);
                return found != end() && *found == val;
            }

        void insert(size_t val)
            {
                if (const auto found = std::lower_bound(begin(), end(), val); found == end() || *found != val)
                    data().insert(found, val);
            }

        void erase_except(size_t val)
            {
                const auto present = contains(val);
                clear();
                if (present)
                    insert(val);
            }

        void extend(const PointIndexList& source)
            {
                for (const auto no : source)
                    insert(no);
            }

        using detail::index_list_data::operator const std::vector<size_t>&;

    }; // class PointIndexList

} // namespace acmacs::chart

namespace acmacs
{
    inline std::string to_string(const acmacs::chart::PointIndexList& indexes) { return to_string(indexes.data()); }

} // namespace acmacs

namespace acmacs::chart
{
    inline std::ostream& operator<<(std::ostream& out, const PointIndexList& indexes) { return out << acmacs::to_string(indexes); }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
