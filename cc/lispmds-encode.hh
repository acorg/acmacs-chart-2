#pragma once

#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    std::string lispmds_encode(std::string aName);
    std::string lispmds_antigen_name_encode(const Name& aName, const Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations);
    std::string lispmds_serum_name_encode(const Name& aName, const Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId);
    std::string lispmds_decode(std::string aName);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
