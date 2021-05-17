#pragma once

#include <map>
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

      private:
        struct ASName
        {
            std::string antigen;
            std::string serum;
            bool operator<(const ASName& rhs) const { return antigen == rhs.antigen ? serum < rhs.serum : antigen < rhs.antigen; }
        };

        std::map<ASName, std::vector<acmacs::chart::Titer>> titers_;
        std::vector<std::string> table_names_;

        size_t add_table(const Chart& chart);
    };
} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
