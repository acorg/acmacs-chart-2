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
        struct Titers : public std::vector<Titer>
        {
            using std::vector<Titer>::vector;

            size_t count_non_dontcare() const
            {
                return ranges::accumulate(ranges::views::transform(*this, [](const auto& titer) { return titer.is_dont_care() ? 0 : 1; }), 0ul);
            }

            size_t first_non_dontcare_index() const
            {
                return static_cast<size_t>(std::distance(std::find_if(begin(), end(), [](const auto& titer) { return !titer.is_dont_care(); }), begin()));
            }
            size_t last_non_dontcare_index() const
            {
                return static_cast<size_t>(std::distance(std::find_if(rbegin(), rend(), [](const auto& titer) { return !titer.is_dont_care(); }), rend()));
            }
        };

        class AntigenSerumData
        {
          public:
            // bool empty() const { return titer_per_table.empty(); }
            // size_t number_of_tables() const
            // {
            //     return std::accumulate(titer_per_table.begin(), titer_per_table.end(), 0U, [](size_t acc, auto& element) { return element.first.is_dont_care() ? acc : (acc + 1); });
            // }
            // size_t first_table_no() const
            // {
            //     for (auto [no, entry] : acmacs::enumerate(titer_per_table))
            //         if (!entry.first.is_dont_care())
            //             return no;
            //     return 0;
            // }

            acmacs::chart::Titer median_titer;
            int median_titer_index{-1};
            Titers titers;
        };

        using ASTable = std::vector<std::vector<AntigenSerumData>>;

        // collect titers from a chart
        void add(const Chart& chart);
        void collect_titer_set();

        ASTable make_antigen_serum_table(const std::vector<std::string>& antigens, const std::vector<std::string>& sera) const;

        std::vector<std::string> antigens(size_t min_tables) const;
        std::vector<std::string> sera(size_t min_tables) const;

      private:
        struct ASName
        {
            std::string antigen;
            std::string serum;
            bool operator<(const ASName& rhs) const { return antigen == rhs.antigen ? serum < rhs.serum : antigen < rhs.antigen; }
        };

        std::map<ASName, Titers> titers_;
        std::vector<std::string> table_names_;
        std::set<Titer> all_titers_;

        size_t add_table(const Chart& chart);

        // returns table indexes where at least one antigen/serum pair has a non-dontcare titer
        std::vector<size_t> tables_of_antigens_sera(const std::vector<std::string>& antigens, const std::vector<std::string>& sera) const;

    };
} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
