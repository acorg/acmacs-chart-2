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
            inline Entry(size_t p1, size_t p2, Float dist) : point_1(p1), point_2(p2), table_distance{dist} {}
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

          // inline void report() const { std::cerr << "TableDistances regular: " << regular_.size() << "  less-than: " << less_than_.size() << '\n'; }

        const entries_t& regular() const { return regular_; }
        const entries_t& less_than() const { return less_than_; }

        class IteratorForPoint
        {
          private:
            friend class TableDistances<Float>;

            using iterator_t = typename entries_t::const_iterator;
            IteratorForPoint(size_t point_no, iterator_t first_regular, iterator_t last_regular, iterator_t first_less_than, iterator_t last_less_than)
                : point_no_(point_no), current_(first_regular), last_regular_(last_regular), first_less_than_(first_less_than), last_less_than_(last_less_than)
            {
                if (current_->point_1 != point_no_ && current_->point_2 != point_no_)
                    operator++();
            }
            IteratorForPoint(size_t point_no, iterator_t last_less_than)
                : point_no_(point_no), current_(last_less_than), last_regular_(last_less_than), first_less_than_(last_less_than), last_less_than_(last_less_than) {}

            void inc()
                {
                    if (current_ != last_less_than_) {
                        ++current_;
                        if (current_ == last_regular_)
                            current_ = first_less_than_;
                    }
                }

            size_t point_no_;
            iterator_t current_, last_regular_, first_less_than_, last_less_than_;

          public:
            bool operator==(const IteratorForPoint& rhs) const { return current_ == rhs.current_; }
            bool operator!=(const IteratorForPoint& rhs) const { return !operator==(rhs); }
            const Entry& operator*() const { return *current_; }
            const Entry* operator->() const { return &*current_; }

            const IteratorForPoint& operator++()
                {
                    for (inc(); current_ != last_less_than_ && current_->point_1 != point_no_ && current_->point_2 != point_no_; inc());
                    return *this;
                }

        }; // class IteratorForPoint

        IteratorForPoint begin_for(size_t point_no) const { return IteratorForPoint(point_no, regular_.begin(), regular_.end(), less_than_.begin(), less_than_.end()); }
        IteratorForPoint end_for(size_t point_no) const { return IteratorForPoint(point_no, less_than_.end()); }

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
