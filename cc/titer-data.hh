#pragma once

#include <map>
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/titers.hh"

// reference-panel-plots support
// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    class TiterData
    {
      public:
        // collect titers from a chart
        void add(const Chart& chart);
        void collect_titer_set();

        std::vector<std::string> antigens(size_t min_tables) const;
        std::vector<std::string> sera(size_t min_tables) const;

      private:
        struct ASName
        {
            std::string antigen;
            std::string serum;
            bool operator<(const ASName& rhs) const { return antigen == rhs.antigen ? serum < rhs.serum : antigen < rhs.antigen; }
        };

        struct Titers : public std::vector<Titer>
        {
            using std::vector<Titer>::vector;

            size_t count_non_dontcare() const
            {
                return ranges::accumulate(ranges::views::transform(*this, [](const auto& titer) { return titer.is_dont_care() ? 0 : 1; }), 0ul);
            }
        };

        std::map<ASName, Titers> titers_;
        std::vector<std::string> table_names_;
        std::set<Titer> all_titers_;

        size_t add_table(const Chart& chart);
    };
} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
