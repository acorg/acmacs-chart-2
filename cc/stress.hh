#pragma once

#include "acmacs-chart-2/table-distances.hh"
#include "acmacs-chart-2/point-index-list.hh"
#include "acmacs-chart-2/avidity-adjusts.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class LayoutInterface;

} // namespace acmacs

namespace acmacs::chart
{
    class Chart;
    class Projection;

    struct StressParameters
    {
        inline StressParameters(PointIndexList&& a_unmovable, PointIndexList&& a_disconnected, PointIndexList&& a_unmovable_in_the_last_dimension, bool a_multiply_antigen_titer_until_column_adjust, AvidityAdjusts&& a_avidity_adjusts, bool a_dodgy_titer_is_regular)
            : unmovable(std::move(a_unmovable)), disconnected(std::move(a_disconnected)),
              unmovable_in_the_last_dimension(std::move(a_unmovable_in_the_last_dimension)), multiply_antigen_titer_until_column_adjust(a_multiply_antigen_titer_until_column_adjust),
              avidity_adjusts(std::move(a_avidity_adjusts)), dodgy_titer_is_regular(a_dodgy_titer_is_regular) {}

        PointIndexList unmovable;
        PointIndexList disconnected;
        PointIndexList unmovable_in_the_last_dimension;
        bool multiply_antigen_titer_until_column_adjust;
        AvidityAdjusts avidity_adjusts;
        bool dodgy_titer_is_regular;

    }; // struct StressParameters

    template <typename Float> class Stress
    {
     public:
        Stress(const Projection& projection, bool multiply_antigen_titer_until_column_adjust);

        Float value(const std::vector<Float>& aArgument) const;
        Float value(const acmacs::LayoutInterface& aLayout) const;
        std::vector<Float> gradient(const std::vector<Float>& aArgument) const;
        std::vector<Float> gradient(const acmacs::LayoutInterface& aLayout) const;

        inline const TableDistances<Float>& table_distances() const { return table_distances_; }
        inline TableDistances<Float>& table_distances() { return table_distances_; }
        inline const StressParameters& parameters() const { return parameters_; }

     private:
        const size_t number_of_dimensions_;
        TableDistances<Float> table_distances_;
        StressParameters parameters_;

    }; // class Stress

    template <typename Float> Stress<Float> stress_factory(const Chart& chart, const Projection& projection, bool multiply_antigen_titer_until_column_adjust);

    extern template class Stress<float>;
    extern template class Stress<double>;
#if __GNUC__ == 7
      // g++7 does not like extern template below
#else
      // clang5 wants those externs (otherwise warning -Wundefined-func-template)
    extern template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection, bool multiply_antigen_titer_until_column_adjust);
    extern template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection, bool multiply_antigen_titer_until_column_adjust);
#endif

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
