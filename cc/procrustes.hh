#pragma once

#include "acmacs-base/transformation.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

namespace acmacs { class LayoutInterface; class Layout; }

namespace acmacs::chart
{
    class Projection;
    class CommonAntigensSera;

    class ProcrustesData
    {
     public:
        ProcrustesData(size_t number_of_dimensions) : transformation(number_of_dimensions) {}
        TransformationTranslation transformation;
        double scale{1};
        double rms{0};
          // distance_summary = backend.ProcrustesDistancesSummaryResults

        std::shared_ptr<acmacs::Layout> apply(const acmacs::LayoutInterface& source) const;

    }; // class ProcrustesData

    enum class procrustes_scaling_t { no, yes };

    ProcrustesData procrustes(const Projection& primary, const Projection& secondary, const std::vector<CommonAntigensSera::common_t>& common, procrustes_scaling_t scaling);

} // namespace acmacs::chart

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
