#pragma once

#include <iostream>
#include <vector>

#include "acmacs-chart-2/titers.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    template <typename Float> class TableDistances
    {
     public:
        struct Entry
        {
            Entry(size_t p1, size_t p2, Float dist) : point_1(p1), point_2(p2), table_distance{dist} {}
            size_t point_1;
            size_t point_2;
            Float table_distance;
        };

        using entries_t = std::vector<Entry>;

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

          // void report() const { std::cerr << "TableDistances regular: " << regular_.size() << "  less-than: " << less_than_.size() << '\n'; }

        const entries_t& regular() const { return regular_; }
        const entries_t& less_than() const { return less_than_; }

        struct EntryForPoint
        {
            EntryForPoint(size_t ap, Float td) : another_point(ap), table_distance(td) {}
            size_t another_point;
            Float table_distance;
        };
        using entries_for_point_t = std::vector<EntryForPoint>;

        static entries_for_point_t entries_for_point(const entries_t& source, size_t point_no)
            {
                entries_for_point_t result;
                for (const auto& src : source) {
                    if (src.point_1 == point_no)
                        result.emplace_back(src.point_2, src.table_distance);
                    else if (src.point_2 == point_no)
                        result.emplace_back(src.point_1, src.table_distance);
                }
                return result;
            }

        struct EntriesForPoint
        {
            EntriesForPoint(size_t point_no, const TableDistances<Float>& table_distances)
                : regular(entries_for_point(table_distances.regular(), point_no)), less_than(entries_for_point(table_distances.less_than(), point_no)) {}
            entries_for_point_t regular, less_than;
        };
        
        class IteratorForPoint
        {
          private:
            friend class TableDistances<Float>;

            using iterator_t = typename entries_t::const_iterator;
            IteratorForPoint(size_t point_no, iterator_t it, iterator_t last)
                : point_no_(point_no), current_(it), last_(last)
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

        IteratorForPoint begin_regular_for(size_t point_no) const { return IteratorForPoint(point_no, regular_.begin(), regular_.end()); }
        IteratorForPoint end_regular_for(size_t point_no) const { return IteratorForPoint(point_no, regular_.end(), regular_.end()); }
        IteratorForPoint begin_less_than_for(size_t point_no) const { return IteratorForPoint(point_no, less_than_.begin(), less_than_.end()); }
        IteratorForPoint end_less_than_for(size_t point_no) const { return IteratorForPoint(point_no, less_than_.end(), less_than_.end()); }

      private:
        bool dodgy_is_regular_ = false;
        entries_t regular_;
        entries_t less_than_;

        void add_value(Titer::Type type, size_t p1, size_t p2, Float value)
            {
                switch (type) {
                  case Titer::Dodgy:
                      if (!dodgy_is_regular_)
                          break;
                      [[fallthrough]];
                  case Titer::Regular:
                      regular_.emplace_back(p1, p2, value);
                      break;
                  case Titer::LessThan:
                      less_than_.emplace_back(p1, p2, value);
                      break;
                  case Titer::Invalid:
                  case Titer::DontCare:
                  case Titer::MoreThan:
                      break;
                }
            }

    }; // class TableDistances

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
