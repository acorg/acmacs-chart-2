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
        using index_t = std::pair<size_t, size_t>;
        using indexes_t = std::vector<index_t>;
        using distances_t = std::vector<Float>;

        inline void dodgy_is_regular(bool dodgy_is_regular) { dodgy_is_regular_ = dodgy_is_regular; }

        template <typename Source> inline void add(Titer::Type type, size_t p1, size_t p2, Source value) { add_value(type, p1, p2, static_cast<Float>(value)); }

        inline void report() const { std::cerr << "TableDistances regular: " << regular_.size() << "  less-than: " << less_than_.size() << '\n'; }

        inline const indexes_t& regular_indexes() const { return regular_indexes_; }
        inline const distances_t& regular_distances() const { return regular_; }
        inline const indexes_t& less_than_indexes() const { return less_than_indexes_; }
        inline const distances_t& less_than_distances() const { return less_than_; }

     private:
        bool dodgy_is_regular_ = false;
        indexes_t regular_indexes_;
        distances_t regular_;
        indexes_t less_than_indexes_;
        distances_t less_than_;

        void add_value(Titer::Type type, size_t p1, size_t p2, Float value)
            {
                switch (type) {
                  case Titer::Dodgy:
                      if (!dodgy_is_regular_)
                          break;
                      [[fallthrough]];
                  case Titer::Regular:
                      regular_indexes_.emplace_back(p1, p2);
                      regular_.push_back(value);
                      break;
                  case Titer::LessThan:
                      less_than_indexes_.emplace_back(p1, p2);
                      less_than_.push_back(value);
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
