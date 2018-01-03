#include "acmacs-base/range.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const std::vector<Float>& aArgument) const
{
    return 0;

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template <typename Float> Float acmacs::chart::Stress<Float>::value(const acmacs::LayoutInterface& aLayout) const
{
    std::vector<Float> argument(aLayout.number_of_points() * aLayout.number_of_dimensions());
    for (auto p_no : acmacs::range(aLayout.number_of_points())) {
        for (auto d_no : acmacs::range(aLayout.number_of_dimensions())) {
            argument[p_no * aLayout.number_of_dimensions() + d_no] = static_cast<Float>(aLayout.coordinate(p_no, d_no));
        }
    }
    return value(argument);

} // acmacs::chart::Stress<Float>::value

// ----------------------------------------------------------------------

template class acmacs::chart::Stress<float>;
template class acmacs::chart::Stress<double>;

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
