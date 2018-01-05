#pragma once

#include "acmacs-chart-2/table-distances.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class LayoutInterface;

} // namespace acmacs

namespace acmacs::chart
{
    class Chart;
    class Projection;

    template <typename Float> class Stress
    {
     public:
        inline Stress(size_t number_of_dimensions, size_t /*number_of_antigens*/) : number_of_dimensions_(number_of_dimensions) /*, number_of_antigens_(number_of_antigens) */ {}

        Float value(const std::vector<Float>& aArgument) const;
        Float value(const acmacs::LayoutInterface& aLayout) const;
        std::vector<Float> gradient(const std::vector<Float>& aArgument) const;
        std::vector<Float> gradient(const acmacs::LayoutInterface& aLayout) const;

        inline const TableDistances<Float>& table_distances() const { return table_distances_; }
        inline TableDistances<Float>& table_distances() { return table_distances_; }

     private:
        const size_t number_of_dimensions_;
          // const size_t number_of_antigens_;
        TableDistances<Float> table_distances_;

    }; // class Stress

    template <typename Float> Stress<Float> stress_factory(const Chart& chart, const Projection& projection);

    extern template class Stress<float>;
    extern template class Stress<double>;
#if __GNUC__ == 7
      // g++7 does not like extern template below
#else
      // clang5 wants those externs (otherwise warning -Wundefined-func-template)
    extern template acmacs::chart::Stress<float> acmacs::chart::stress_factory<float>(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection);
    extern template acmacs::chart::Stress<double> acmacs::chart::stress_factory<double>(const acmacs::chart::Chart& chart, const acmacs::chart::Projection& projection);
#endif

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
