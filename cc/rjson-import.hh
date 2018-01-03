#pragma once

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

#include "acmacs-base/rjson.hh"
#include "acmacs-chart-2/titers.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    template <typename Float> class TableDistances;
    class ColumnBases;
    class PointIndexList;

} // namespace acmacs::chart


namespace acmacs::chart::rjson_import
{

    inline Titer titer_in_d(const rjson::array& aSource, size_t aAntigenNo, size_t aSerumNo)
    {
        try {
            return aSource[aAntigenNo][std::to_string(aSerumNo)];
        }
        catch (rjson::field_not_found&) {
            return "*";
        }
    }

// ----------------------------------------------------------------------

    std::vector<Titer> titers_for_layers(const rjson::array& layers, size_t aAntigenNo, size_t aSerumNo);
    size_t number_of_sera(const rjson::object& data, const char* list_key, const char* dict_key);
    size_t number_of_non_dont_cares(const rjson::object& data, const char* list_key, const char* dict_key);

// ----------------------------------------------------------------------

    inline size_t number_of_dimensions(const rjson::array& data) noexcept
    {
        try {
            for (const rjson::array& row: data) {
                if (!row.empty())
                    return row.size();
            }
        }
        catch (rjson::field_not_found&) {
        }
        catch (std::bad_variant_access&) {
        }
        return 0;
    }

// ----------------------------------------------------------------------

    inline size_t number_of_antigens(const rjson::object& data, const char* list_key, const char* dict_key)
    {
        if (auto [present, list] = data.get_array_if(list_key); present) {
            return list.size();
        }
        else {
            return static_cast<const rjson::array&>(data[dict_key]).size();
        }
    }

// ----------------------------------------------------------------------

    template <typename Float> void update(const rjson::object& data, const char* list_key, const char* dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust);
    extern template void update<float>(const rjson::object& data, const char* list_key, const char* dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust);
    extern template void update<double>(const rjson::object& data, const char* list_key, const char* dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust);

} // namespace acmacs::chart::rjson

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
