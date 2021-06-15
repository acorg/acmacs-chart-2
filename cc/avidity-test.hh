#pragma once

#include "acmacs-base/fmt.hh"
#include "acmacs-base/point-coordinates.hh"
#include "acmacs-chart-2/column-bases.hh"
#include "acmacs-chart-2/optimize-options.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class ChartModify;
    class ProjectionModify;
    struct optimization_options;
    class AvidityAdjusts;

    namespace avidity
    {
        struct MostMoved
        {
            size_t antigen_no;
            double distance;
        };

        constexpr const size_t number_of_most_moved_antigens{5};

        struct PerAdjust
        {
            double logged_adjust;
            double distance_test_antigen;
            double angle_test_antigen;
            double average_procrustes_distances_except_test_antigen;
            PointCoordinates final_coordinates;
            double stress_diff;
            std::array<MostMoved, number_of_most_moved_antigens> most_moved;
        };

        struct Result
        {
            // Result(size_t a_antigen_no, PointCoordinates) : antigen_no{a_antigen_no} {}

            size_t antigen_no;
            double best_logged_adjust;
            PointCoordinates original;
            std::vector<PerAdjust> adjusts;

            const PerAdjust& best_adjust() const
            {
                if (const auto found = std::find_if(std::begin(adjusts), std::end(adjusts), [best = best_logged_adjust](const auto& en) { return float_equal(best, en.logged_adjust); });
                    found != std::end(adjusts))
                    return *found;
                else
                    throw std::runtime_error{AD_FORMAT("avidity: no best adjust entry for {} (internal error)", best_logged_adjust)};
            }

            void post_process();
        };

        struct Results
        {
            double original_stress;
            std::vector<Result> results;

            const Result& get(size_t antigen_no) const
            {
                if (const auto found = std::find_if(std::begin(results), std::end(results), [antigen_no](const auto& en) { return en.antigen_no == antigen_no; }); found != std::end(results))
                    return *found;
                else
                    throw std::runtime_error{AD_FORMAT("avidity: no result entry for AG {} (internal error)", antigen_no)};
            }

            void post_process();
        };

        struct Settings
        {
            double step{1.0};
            double min_adjust{-6.0};
            double max_adjust{6.0};
            size_t threads{0};
        };

        // test all antigens
        Results test(ChartModify& chart, size_t projection_no, const Settings& settings, const optimization_options& options);
        // test some antigens
        Results test(ChartModify& chart, size_t projection_no, const std::vector<size_t>& antigens_to_test, const Settings& settings, const optimization_options& options);

        // adds new projection with antigens moved to their better positions and avidity data changed
        // returns new projection
        std::shared_ptr<ProjectionModify> move_antigens(ChartModify& chart, size_t projection_no, const Results& avidity_results);

        // test steps
        Result test(ChartModify& chart, const ProjectionModify& original_projection, size_t antigen_no, const Settings& settings, const optimization_options& options);
        PerAdjust test(ChartModify& chart, const ProjectionModify& original_projection, size_t antigen_no, double logged_adjust, const optimization_options& options, bool add_new_projection_to_chart = false);

        // relax chart from scratch with avidity adjusts
        void relax(ChartModify& chart, number_of_optimizations_t number_of_optimizations, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, const AvidityAdjusts& avidity_adjusts, const optimization_options& options);

    } // namespace avidity
} // namespace acmacs::chart

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::chart::avidity::PerAdjust> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::avidity::PerAdjust& per_adjust, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{:4.1f}  diff:{:8.4f} dist:{:7.4f} angle:{:7.4f} aver_pc_dist:{:7.4f}", per_adjust.logged_adjust, per_adjust.stress_diff, per_adjust.distance_test_antigen,
                         per_adjust.angle_test_antigen, per_adjust.average_procrustes_distances_except_test_antigen);
    }
};

template <> struct fmt::formatter<acmacs::chart::avidity::Result> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::avidity::Result& result, FormatCtx& ctx)
    {
        format_to(ctx.out(), "AG {}\n    {}\n", result.antigen_no, result.best_adjust());
        for (const auto& en : result.adjusts)
            format_to(ctx.out(), "        {}\n", en);
        return ctx.out();
    }
};

template <> struct fmt::formatter<acmacs::chart::avidity::Results> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::avidity::Results& results, FormatCtx& ctx)
    {
        for (const auto& result : results.results)
            format_to(ctx.out(), "{}\n", result);
        return ctx.out();
    }
};


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
