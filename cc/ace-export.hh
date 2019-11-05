#pragma once

#include <string>
#include "acmacs-base/rjson.hh"
#include "acmacs-base/data-formatter.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    rjson::value export_ace_to_rjson(const Chart& aChart, std::string_view aProgramName);
    std::string export_ace(const Chart& aChart, std::string_view aProgramName, size_t aIndent);

    template <typename DF> std::string export_layout(const Chart& aChart, size_t aProjectionNo = 0);
    extern template std::string export_layout<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
    extern template std::string export_layout<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

    template <typename DF> std::string export_table_map_distances(const Chart& aChart, size_t aProjectionNo = 0);
    extern template std::string export_table_map_distances<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
    extern template std::string export_table_map_distances<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

    template <typename DF> std::string export_distances_between_all_points(const Chart& aChart, size_t aProjectionNo = 0);
    extern template std::string export_distances_between_all_points<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
    extern template std::string export_distances_between_all_points<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

    template <typename DF> std::string export_error_lines(const Chart& aChart, size_t aProjectionNo = 0);
    extern template std::string export_error_lines<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
    extern template std::string export_error_lines<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
