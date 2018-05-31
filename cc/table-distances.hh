#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include "acmacs-base/layout.hh"
#include "acmacs-chart-2/titers.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace internal
    {
        template <typename Float> class DistancesBase
        {
          public:
            struct Entry
            {
                Entry(size_t p1, size_t p2, Float dist) : point_1(p1), point_2(p2), distance{dist} {}
                size_t point_1;
                size_t point_2;
                Float distance;
            };

            using entries_t = std::vector<Entry>;

            const entries_t& regular() const { return regular_; }
            entries_t& regular() { return regular_; }
            const entries_t& less_than() const { return less_than_; }
            entries_t& less_than() { return less_than_; }

            class IteratorForPoint
            {
              private:
                friend class DistancesBase<Float>;

                using iterator_t = typename entries_t::const_iterator;
                IteratorForPoint(size_t point_no, iterator_t it, iterator_t last) : point_no_(point_no), current_(it), last_(last)
                {
                    if (current_ != last_ && current_->point_1 != point_no_ && current_->point_2 != point_no_)
                        operator++();
                }

                size_t point_no_;
                iterator_t current_, last_;

              public:
                bool operator==(const IteratorForPoint& rhs) const { return current_ == rhs.current_; }
                bool operator!=(const IteratorForPoint& rhs) const { return !operator==(rhs); }
                const Entry& operator*() const { return *current_; }
                const Entry* operator->() const { return &*current_; }

                const IteratorForPoint& operator++()
                {
                    for (++current_; current_ != last_ && current_->point_1 != point_no_ && current_->point_2 != point_no_; ++current_)
                        ;
                    return *this;
                }

            }; // class IteratorForPoint

            IteratorForPoint begin_regular_for(size_t point_no) const { return IteratorForPoint(point_no, regular().begin(), regular().end()); }
            IteratorForPoint end_regular_for(size_t point_no) const { return IteratorForPoint(point_no, regular().end(), regular().end()); }
            IteratorForPoint begin_less_than_for(size_t point_no) const { return IteratorForPoint(point_no, less_than().begin(), less_than().end()); }
            IteratorForPoint end_less_than_for(size_t point_no) const { return IteratorForPoint(point_no, less_than().end(), less_than().end()); }

          private:
            entries_t regular_;
            entries_t less_than_;

        }; // class DistancesBase<Float>

    } // namespace internal

// ----------------------------------------------------------------------

    template <typename Float> class TableDistances : public internal::DistancesBase<Float>
    {
     public:
        using entries_t = typename internal::DistancesBase<Float>::entries_t;
        using internal::DistancesBase<Float>::regular;
        using internal::DistancesBase<Float>::less_than;

        void dodgy_is_regular(bool dodgy_is_regular) { dodgy_is_regular_ = dodgy_is_regular; }

        void update(const acmacs::chart::Titer& titer, size_t p1, size_t p2, double column_basis, double adjust, multiply_antigen_titer_until_column_adjust mult)
        {
            try {
                auto distance = column_basis - titer.logged() - adjust;
                if (distance < 0 && mult == multiply_antigen_titer_until_column_adjust::yes)
                    distance = 0;
                add_value(titer.type(), p1, p2, static_cast<Float>(distance));
            }
            catch (acmacs::chart::invalid_titer&) {
                // ignore dont-care
            }
        }

        // void report() const { std::cerr << "TableDistances regular: " << regular().size() << "  less-than: " << less_than().size() << '\n'; }

        struct EntryForPoint
        {
            EntryForPoint(size_t ap, Float td) : another_point(ap), distance(td) {}
            size_t another_point;
            Float distance;
        };
        using entries_for_point_t = std::vector<EntryForPoint>;

        static entries_for_point_t entries_for_point(const entries_t& source, size_t point_no)
        {
            entries_for_point_t result;
            for (const auto& src : source) {
                if (src.point_1 == point_no)
                    result.emplace_back(src.point_2, src.distance);
                else if (src.point_2 == point_no)
                    result.emplace_back(src.point_1, src.distance);
            }
            return result;
        }

        struct EntriesForPoint
        {
            EntriesForPoint(size_t point_no, const TableDistances<Float>& table_distances)
                : regular(entries_for_point(table_distances.regular(), point_no)), less_than(entries_for_point(table_distances.less_than(), point_no))
            {
            }
            entries_for_point_t regular, less_than;
        };

      private:
        bool dodgy_is_regular_ = false;

        void add_value(Titer::Type type, size_t p1, size_t p2, Float value)
        {
            switch (type) {
                case Titer::Dodgy:
                    if (!dodgy_is_regular_)
                        break;
                    [[fallthrough]];
                case Titer::Regular:
                    regular().emplace_back(p1, p2, value);
                    break;
                case Titer::LessThan:
                    less_than().emplace_back(p1, p2, value);
                    break;
                case Titer::Invalid:
                case Titer::DontCare:
                case Titer::MoreThan:
                    break;
            }
        }

    }; // class TableDistances

    // ----------------------------------------------------------------------

    class MapDistances : public internal::DistancesBase<double>
    {
     public:
       MapDistances(const LayoutInterface& layout, const TableDistances<double> table_distances)
       {
           auto make_map_distance = [&layout](const auto& table_distance_entry) -> Entry {
               return {table_distance_entry.point_1, table_distance_entry.point_2, layout.distance(table_distance_entry.point_1, table_distance_entry.point_2)};
           };
           std::transform(table_distances.regular().begin(), table_distances.regular().end(), std::back_inserter(regular()), make_map_distance);
           std::transform(table_distances.less_than().begin(), table_distances.less_than().end(), std::back_inserter(less_than()), make_map_distance);
       }

    }; // class MapDistances

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
