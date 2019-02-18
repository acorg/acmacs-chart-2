#pragma once

#include <cmath>
#include <vector>

// ----------------------------------------------------------------------

namespace acmacs
{
    class LayoutInterface;
    class Layout;
}

namespace acmacs::chart
{
    template <typename Float> class Stress;
    class PointIndexList;

    class Blobs
    {
      public:
        Blobs(double stress_diff, size_t number_of_drections = 36, double stress_diff_precision = 1e-5)
            : stress_diff_{stress_diff}, number_of_drections_{number_of_drections}, stress_diff_precision_{stress_diff_precision}, angle_step_{2.0 * M_PI / number_of_drections} {}

        void calculate(const acmacs::LayoutInterface& layout, const Stress<double>& stress);
        void calculate(const acmacs::LayoutInterface& layout, const PointIndexList& points, const Stress<double>& stress);

      private:
        const double stress_diff_;
        const size_t number_of_drections_;
        const double stress_diff_precision_;
        const double angle_step_;
        std::vector<std::pair<size_t, std::vector<double>>> result_; // point_no, blob_data

        void calculate_for_point(size_t point_no, acmacs::Layout& layout, const Stress<double>& stress, double initial_stress);

    }; // class Blobs

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
