#pragma once

#include "acmacs-base/transformation.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Projection;
    class CommonAntigensSera;

    class ProcrustesData
    {
     public:
        TransformationTranslation transformation;
        double rms;
          // distance_summary = backend.ProcrustesDistancesSummaryResults

    }; // class ProcrustesData

    ProcrustesData procrustes(const Projection& primary, const Projection& secondary, const CommonAntigensSera& common);

} // namespace acmacs::chart

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
