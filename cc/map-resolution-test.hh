#pragma once

#include <vector>
#include <iostream>

#include "acmacs-base/statistics.hh"
#include "acmacs-chart-2/optimize-options.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class Layout;

    namespace chart
    {
        class ChartModify;

        namespace map_resolution_test_data
        {
            //  additional projection in each replicate, first full table is
            //  relaxed, then titers dont-cared and the best projection
            //  relaxed again from already found starting coordinates.
            enum class relax_from_full_table { no, yes };

            // converting titers to dont-care may change column bases, force
            // master chart column bases
            enum class column_bases_from_master { no, yes };

            struct Parameters
            {
                std::vector<number_of_dimensions_t> number_of_dimensions{number_of_dimensions_t{1}, number_of_dimensions_t{2}, number_of_dimensions_t{3}, number_of_dimensions_t{4},
                                                                         number_of_dimensions_t{5}};
                number_of_optimizations_t number_of_optimizations{100UL};
                size_t number_of_random_replicates_for_each_proportion{25};
                std::vector<double> proportions_to_dont_care{0.1, 0.2, 0.3};
                acmacs::chart::MinimumColumnBasis minimum_column_basis{};
                enum column_bases_from_master column_bases_from_master { column_bases_from_master::yes };
                enum optimization_precision optimization_precision { optimization_precision::rough };
                enum relax_from_full_table relax_from_full_table { relax_from_full_table::no };
                std::string save_charts_to;
            };

            // ----------------------------------------------------------------------

            struct PredictionsSummary
            {
                PredictionsSummary(number_of_dimensions_t a_number_of_dimensions, double a_proportion_to_dont_care, statistics::StandardDeviation a_av_abs_error,
                                   statistics::StandardDeviation a_sd_error, statistics::StandardDeviation a_correlations, statistics::StandardDeviation a_r2, size_t a_number_of_samples)
                    : number_of_dimensions{a_number_of_dimensions}, proportion_to_dont_care{a_proportion_to_dont_care}, av_abs_error{a_av_abs_error}, sd_error{a_sd_error},
                      correlations{a_correlations}, r2{a_r2}, number_of_samples{a_number_of_samples}
                {
                }

                const number_of_dimensions_t number_of_dimensions;
                const double proportion_to_dont_care;
                const statistics::StandardDeviation av_abs_error;
                const statistics::StandardDeviation sd_error;
                const statistics::StandardDeviation correlations;
                const statistics::StandardDeviation r2;
                const size_t number_of_samples;
            };

            std::ostream& operator << (std::ostream& out, const PredictionsSummary& predictions_summary);

            class Results
            {
              public:
                Results(const Parameters& param) : parameters_{param} {}

                const auto& predictions() const { return predictions_; }
                auto& predictions() { return predictions_; }

              private:
                const Parameters parameters_;
                std::vector<PredictionsSummary> predictions_;

                friend std::ostream& operator << (std::ostream& out, const Results& results);

            };

            std::ostream& operator << (std::ostream& out, const Results& results);

            // ----------------------------------------------------------------------

            struct Predictions
            {
                double av_abs_error;
                double sd_error;
                double correlation;
                statistics::SimpleLinearRegression linear_regression;
                size_t number_of_samples;
            };

            std::ostream& operator << (std::ostream& out, const Predictions& predictions);

            struct PredictionErrorForTiter
            {
                PredictionErrorForTiter(size_t ag, size_t sr, double er) : antigen{ag}, serum{sr}, error{er} {}
                size_t antigen;
                size_t serum;
                double error;
            };

            struct ReplicateStat
            {
                std::vector<double> master_distances;
                std::vector<double> predicted_distances;
                std::vector<PredictionErrorForTiter> prediction_errors_for_titers;
            };

        } // namespace map_resolution_test_data

        map_resolution_test_data::Results map_resolution_test(ChartModify& chart, const map_resolution_test_data::Parameters& parameters);


    } // namespace chart

} // namespace acmacs

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::chart::map_resolution_test_data::relax_from_full_table> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::chart::map_resolution_test_data::relax_from_full_table& rel, FormatContext& ctx)
    {
        using namespace acmacs::chart::map_resolution_test_data;
        switch (rel) {
          case relax_from_full_table::no:
              return fmt::format_to(ctx.out(), "no");
          case relax_from_full_table::yes:
              return fmt::format_to(ctx.out(), "yes");
        }
        return fmt::format_to(ctx.out(), "unknown"); // g++9
    }
};

template <> struct fmt::formatter<acmacs::chart::map_resolution_test_data::column_bases_from_master> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::chart::map_resolution_test_data::column_bases_from_master& rel, FormatContext& ctx)
    {
        using namespace acmacs::chart::map_resolution_test_data;
        switch (rel) {
          case column_bases_from_master::no:
              return fmt::format_to(ctx.out(), "no");
          case column_bases_from_master::yes:
              return fmt::format_to(ctx.out(), "yes");
        }
        return fmt::format_to(ctx.out(), "unknown"); // g++9
    }
};

template <> struct fmt::formatter<acmacs::chart::map_resolution_test_data::Parameters> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::chart::map_resolution_test_data::Parameters& param, FormatContext& ctx)
    {
        fmt::format_to(ctx.out(), "map resolution test parameters\n");
        fmt::format_to(ctx.out(), "  number_of_dimensions:                            {}\n", param.number_of_dimensions);
        fmt::format_to(ctx.out(), "  number_of_optimizations:                         {}\n", param.number_of_optimizations);
        fmt::format_to(ctx.out(), "  number_of_random_replicates_for_each_proportion: {}\n", param.number_of_random_replicates_for_each_proportion);
        fmt::format_to(ctx.out(), "  proportions_to_dont_care:                        {}\n", param.proportions_to_dont_care);
        fmt::format_to(ctx.out(), "  minimum_column_basis:                            {}\n", param.minimum_column_basis);
        fmt::format_to(ctx.out(), "  column_bases_from_master:                        {}\n", param.column_bases_from_master);
        fmt::format_to(ctx.out(), "  optimization_precision:                          {}\n", param.optimization_precision);
        fmt::format_to(ctx.out(), "  relax_from_full_table:                           {}\n",   param.relax_from_full_table);
        fmt::format_to(ctx.out(), "  save_charts_to:                                  {}",   param.relax_from_full_table);
        return ctx.out();
    }
};

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
