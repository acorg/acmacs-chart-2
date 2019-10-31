#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    enum class lispmds_encoding_signature { no, strain_name, table_name };

    std::string lispmds_encode(std::string_view aName, lispmds_encoding_signature signature = lispmds_encoding_signature::strain_name);
    std::string lispmds_table_name_encode(std::string_view name);
    std::string lispmds_antigen_name_encode(const acmacs::virus::name_t& aName, const acmacs::virus::Reassortant& aReassortant, const acmacs::virus::Passage& aPassage, const Annotations& aAnnotations, lispmds_encoding_signature signature = lispmds_encoding_signature::strain_name);
    std::string lispmds_serum_name_encode(const acmacs::virus::name_t& aName, const acmacs::virus::Reassortant& aReassortant, const Annotations& aAnnotations, const SerumId& aSerumId, lispmds_encoding_signature signature = lispmds_encoding_signature::strain_name);
    std::string lispmds_decode(std::string_view aName);
    void lispmds_antigen_name_decode(std::string_view aSource, acmacs::virus::name_t& aName, acmacs::virus::Reassortant& aReassortant, acmacs::virus::Passage& aPassage, Annotations& aAnnotations);
    void lispmds_serum_name_decode(std::string_view aSource, acmacs::virus::name_t& aName, acmacs::virus::Reassortant& aReassortant, Annotations& aAnnotations, SerumId& aSerumId);

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
