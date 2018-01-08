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

        inline void dodgy_is_regular(bool dodgy_is_regular) { dodgy_is_regular_ = dodgy_is_regular; }

        void update(const acmacs::chart::Titer& titer, size_t p1, size_t p2, double column_basis, double adjust, bool multiply_antigen_titer_until_column_adjust)
            {
                try {
                    auto distance = column_basis - titer.logged() - adjust;
                    if (distance < 0 && multiply_antigen_titer_until_column_adjust)
                        distance = 0;
                    add_value(titer.type(), p1, p2, static_cast<Float>(distance));
                }
                catch (acmacs::chart::invalid_titer&) {
                      // ignore dont-care
                }
            }

          // inline void report() const { std::cerr << "TableDistances regular: " << regular_.size() << "  less-than: " << less_than_.size() << '\n'; }

        inline const entries_t& regular() const { return regular_; }
        inline const entries_t& less_than() const { return less_than_; }

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