#pragma once

#include <map>
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/titers.hh"

// reference-panel-plots support
// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    class ReferencePanelPlotData
    {
      public:
        static inline bool is_dont_care(const Titer& titer) { return titer.is_dont_care(); }
        static inline bool is_not_dont_care(const Titer& titer) { return !titer.is_dont_care(); }

        struct Titers : public std::vector<Titer>
        {
            using std::vector<Titer>::vector;
            Titers(const Titers&) = default;
            Titers& operator=(const Titers&) = default;

            size_t count_non_dontcare() const
            {
                return ranges::accumulate(ranges::views::transform(*this, [](const auto& titer) { return titer.is_dont_care() ? 0 : 1; }), 0ul);
            }

            size_t first_non_dontcare_index() const
            {
                return static_cast<size_t>(std::distance(begin(), std::find_if(begin(), end(), is_not_dont_care)));
            }
            size_t last_non_dontcare_index() const
            {
                return static_cast<size_t>(std::distance(std::find_if(rbegin(), rend(), is_not_dont_care), rend()));
            }
        };

        struct AntigenSerumData
        {
            acmacs::chart::Titer median_titer{};
            Titers titers;

            void find_median();
        };

        struct ASTable
        {
            using Data = std::vector<std::vector<AntigenSerumData>>;

            std::vector<std::string> antigens;
            std::vector<std::string> sera;
            Data data;
        };

        // collect titers from a chart
        void
        add(const Chart& chart);
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

        std::vector<size_t> tables_of_antigens_sera(const std::vector<std::string>& antigens, const std::vector<std::string>& sera) const;

    };
} // namespace acmacs::chart

template <> struct fmt::formatter<acmacs::chart::ReferencePanelPlotData::Titers> : fmt::formatter<std::vector<acmacs::chart::Titer>>
{
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
