#pragma once

#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    std::string lispmds_encode(std::string aName, bool add_signature = true);
    std::string lispmds_antigen_name_encode(const Name& aName, const Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations, bool add_signature = true);
    std::string lispmds_serum_name_encode(const Name& aName, const Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId, bool add_signature = true);
    std::string lispmds_decode(std::string aName);
    void lispmds_antigen_name_decode(std::string aSource, Name& aName, Reassortant& aReassortant, Passage& aPassage, Annotations& aAnnotations);
    void lispmds_serum_name_decode(std::string aSource, Name& aName, Reassortant& aReassortant, Annotations& aAnnotations, SerumId& aSerumId);

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs::lispmds
{
    constexpr const double DS_SCALE{1.0}; // 3.0
    constexpr const double NS_SCALE{1.0}; // 0.5

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
