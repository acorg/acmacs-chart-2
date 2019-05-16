#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    enum class lispmds_encoding_signature { no, strain_name, table_name };

    std::string lispmds_encode(std::string aName, lispmds_encoding_signature signature = lispmds_encoding_signature::strain_name);
    std::string lispmds_table_name_encode(std::string name);
    std::string lispmds_antigen_name_encode(const Name& aName, const acmacs::virus::Reassortant& aReassortant, const Passage& aPassage, const Annotations& aAnnotations, lispmds_encoding_signature signature = lispmds_encoding_signature::strain_name);
    std::string lispmds_serum_name_encode(const Name& aName, const acmacs::virus::Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId, lispmds_encoding_signature signature = lispmds_encoding_signature::strain_name);
    std::string lispmds_decode(std::string aName);
    void lispmds_antigen_name_decode(std::string aSource, Name& aName, acmacs::virus::Reassortant& aReassortant, Passage& aPassage, Annotations& aAnnotations);
    void lispmds_serum_name_decode(std::string aSource, Name& aName, acmacs::virus::Reassortant& aReassortant, Annotations& aAnnotations, SerumId& aSerumId);

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
