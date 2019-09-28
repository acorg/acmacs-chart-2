#pragma once

#include "acmacs-base/named-type.hh"
#include "acmacs-base/rjson.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
      // sorted list of indexes
    class PointIndexList : public acmacs::named_vector_t<size_t, struct chart_PointIndexList_tag_t>
    {
     public:
        using difference_type = std::vector<size_t>::difference_type;
        using base_t = acmacs::named_vector_t<size_t, struct chart_PointIndexList_tag_t>;

        using base_t::named_vector_t;
        PointIndexList(const rjson::value& src) : base_t::named_vector_t(src.size()) { rjson::copy(src, begin()); }
        template <typename Iter> PointIndexList(Iter first, Iter last, std::function<size_t (const typename Iter::value_type&)> convert) : base_t::named_vector_t(static_cast<size_t>(last - first)) { std::transform(first, last, begin(), convert); }

        bool contains(size_t val) const
            {
                const auto found = std::lower_bound(begin(), end(), val);
                return found != end() && *found == val;
            }

        void insert(size_t val)
            {
                if (const auto found = std::lower_bound(begin(), end(), val); found == end() || *found != val)
                    get().insert(found, val);
            }

        void erase_except(size_t val)
            {
                const auto present = contains(val);
                get().clear();
                if (present)
                    insert(val);
            }

        void extend(const PointIndexList& source)
            {
                for (const auto no : source)
                    insert(no);
            }

    }; // class PointIndexList

} // namespace acmacs::chart

namespace acmacs
{
    inline std::string to_string(const acmacs::chart::PointIndexList& indexes) { return to_string(*indexes); }

} // namespace acmacs

namespace acmacs::chart
{
    inline std::ostream& operator<<(std::ostream& out, const PointIndexList& indexes) { return out << acmacs::to_string(indexes); }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
