#pragma once

#include "acmacs-base/transformation.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

namespace acmacs { class Layout; }

namespace acmacs::chart
{
    class Projection;
    class CommonAntigensSera;

    class ProcrustesData
    {
     public:
        ProcrustesData(number_of_dimensions_t number_of_dimensions) : transformation(number_of_dimensions) {}
        ProcrustesData(const ProcrustesData&) = default;
        ProcrustesData(ProcrustesData&&) = default;
        Transformation transformation;
        double scale{1};
        double rms{0};
        std::shared_ptr<Layout> secondary_transformed;

        std::shared_ptr<Layout> apply(const acmacs::Layout& source) const;

    }; // class ProcrustesData

    enum class procrustes_scaling_t { no, yes };

    ProcrustesData procrustes(const Projection& primary, const Projection& secondary, const std::vector<CommonAntigensSera::common_t>& common, procrustes_scaling_t scaling);

    // ----------------------------------------------------------------------
    // avidity test support
    // ----------------------------------------------------------------------

    struct ProcrustesSummary
    {
        ProcrustesSummary(size_t number_of_antigens, size_t number_of_sera) : antigen_distances(number_of_antigens), serum_distances(number_of_sera), antigens_by_distance(number_of_antigens) {}

        double average_distance{0.0};
        double longest_distance{0.0};
        std::vector<double> antigen_distances;   // from primary to secondary points
        std::vector<double> serum_distances;       // from primary to secondary points
        std::vector<size_t> antigens_by_distance; // indices antigens sorted by distance (longest first)
        double test_antigen_angle{0.0};           // angle between primary and secondary point for the antigen being tested
        double distance_vaccine_to_test_antigen{0.0};  // in secondary points
        double angle_vaccine_to_test_antigen{0.0};     // in secondary points
    };

    struct ProcrustesSummaryParameters
    {
        size_t number_of_antigens;
        size_t antigen_being_tested;
        std::optional<size_t> vaccine_antigen;
    };

      // Computes some summary information for procrustes results (used by routine_diagnostics.LowAvidityTest):
      // - the average procrustes distances for all antigens (number_of_antigens) excluding antigen_being_tested -> average_distance
      // - procrustes distances for all antigens -> antigen_distances
      // - procrustes distances for all sera -> serum_distances
      // - list of antigens indices sorted by distance moved -> antigens_by_distance
      // - angle at which test antigen was moved (vector from primary to secondary point) -> test_antigen_angle
      // - distance between vaccine_antigen and antigen_being_tested in the secondary points
      // - angle of vector from vaccine_antigen to antigen_being_tested in the secondary points
    ProcrustesSummary procrustes_summary(const acmacs::Layout& primary, const acmacs::Layout& transformed_secondary, const ProcrustesSummaryParameters& parameters);

} // namespace acmacs::chart

/// ----------------------------------------------------------------------
