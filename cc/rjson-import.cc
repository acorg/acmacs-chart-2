#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/rjson-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

std::vector<acmacs::chart::Titer> acmacs::chart::rjson_import::titers_for_layers(const rjson::array& layers, size_t aAntigenNo, size_t aSerumNo)
{
    if (layers.empty())
        throw acmacs::chart::data_not_available("no layers");
    const auto serum_no = std::to_string(aSerumNo);
    std::vector<Titer> result;
    for (const rjson::array& layer: layers) {
        const rjson::object& for_ag = layer[aAntigenNo];
        try {
            result.push_back(for_ag[serum_no]);
        }
        catch (rjson::field_not_found&) {
        }
    }
    return result;

} // acmacs::chart::rjson_import::titers_for_layers

// ----------------------------------------------------------------------

size_t acmacs::chart::rjson_import::number_of_sera(const rjson::object& data, const char* list_key, const char* dict_key)
{
    if (auto [present, list] = data.get_array_if(list_key); present) {
        return static_cast<const rjson::array&>(list[0]).size();
    }
    else {
        const rjson::array& d = data[dict_key];
        auto max_index = [](const rjson::object& obj) -> size_t {
                             size_t result = 0;
                             for (auto key_value: obj) {
                                 if (const size_t ind = std::stoul(key_value.first); ind > result)
                                     result = ind;
                             }
                             return result;
                         };
        return max_index(*std::max_element(d.begin(), d.end(), [max_index](const rjson::object& a, const rjson::object& b) { return max_index(a) < max_index(b); })) + 1;
    }

} // acmacs::chart::rjson_import::number_of_sera

// ----------------------------------------------------------------------

size_t acmacs::chart::rjson_import::number_of_non_dont_cares(const rjson::object& data, const char* list_key, const char* dict_key)
{
    size_t result = 0;
    if (auto [present, list] = data.get_array_if(list_key); present) {
        for (const rjson::array& row: list) {
            for (const Titer titer: row) {
                if (!titer.is_dont_care())
                    ++result;
            }
        }
    }
    else {
        const rjson::array& d = data[dict_key];
        result = std::accumulate(d.begin(), d.end(), 0U, [](size_t a, const rjson::object& row) -> size_t { return a + row.size(); });
    }
    return result;

} // acmacs::chart::rjson_import::number_of_non_dont_cares

// ----------------------------------------------------------------------

acmacs::chart::rjson_import::Layout::Layout(const rjson::array& aData)
    : acmacs::Layout(aData.size(), rjson_import::number_of_dimensions(aData))
{
    auto coord = begin();
    for (const rjson::array& point : aData) {
        if (point.size() == number_of_dimensions())
            std::transform(point.begin(), point.end(), coord, [](const auto& coordinate) -> double { return coordinate; });
        else if (!point.empty())
            throw invalid_data("rjson_import::Layout: point has invalid number of coordinates: " + std::to_string(point.size()) + ", expected 0 or " + std::to_string(number_of_dimensions()));
        coord += static_cast<decltype(coord)::difference_type>(number_of_dimensions());
    }

} // acmacs::chart::rjson_import::Layout::Layout

// ----------------------------------------------------------------------

void acmacs::chart::rjson_import::Layout::set(size_t /*aPointNo*/, const acmacs::Coordinates& /*aCoordinates*/)
{
    throw acmacs::chart::chart_is_read_only{"rjson_import::Layout::set: cannot modify"};
}

// ----------------------------------------------------------------------

template <typename Float> static void update_list(const rjson::array& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::PointIndexList& disconnected, bool multiply_antigen_titer_until_column_adjust)
{
    for (auto p1 : acmacs::range(data.size())) {
        if (!disconnected.exist(p1)) {
            const rjson::array& row = data[p1];
            for (auto serum_no : acmacs::range(row.size())) {
                if (!disconnected.exist(serum_no + data.size())) {
                    const acmacs::chart::Titer titer{row[serum_no]};
                    try {
                        auto distance = column_bases.column_basis(serum_no) - titer.logged();
                        if (distance < 0 && multiply_antigen_titer_until_column_adjust)
                            distance = 0;
                        table_distances.add(titer.type(), p1, serum_no + data.size(), distance);
                    }
                    catch (acmacs::chart::invalid_titer&) {
                          // ignore dont-care
                    }
                }
            }
        }
    }

} // update_list

template <typename Float> static void update_dict(const rjson::array& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::PointIndexList& disconnected, bool multiply_antigen_titer_until_column_adjust)
{
    for (auto p1 : acmacs::range(data.size())) {
        if (!disconnected.exist(p1)) {
            const rjson::object& row = data[p1];
            for (auto [serum_no_s, titer_s] : row) {
                if (const auto serum_no = std::stoul(serum_no_s); !disconnected.exist(serum_no + data.size())) {
                    const acmacs::chart::Titer titer{titer_s};
                    try {
                        auto distance = column_bases.column_basis(serum_no) - titer.logged();
                        if (distance < 0 && multiply_antigen_titer_until_column_adjust)
                            distance = 0;
                        table_distances.add(titer.type(), p1, serum_no + data.size(), distance);
                    }
                    catch (acmacs::chart::invalid_titer&) {
                          // ignore dont-care
                    }
                }
            }
        }
    }

} // update_dict

template <typename Float> void acmacs::chart::rjson_import::update(const rjson::object& data, const char* list_key, const char* dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust)
{
    table_distances.dodgy_is_regular(dodgy_titer_is_regular);
    if (auto [present, list] = data.get_array_if(list_key); present)
        ::update_list(list, table_distances, column_bases, disconnected, multiply_antigen_titer_until_column_adjust);
    else
        ::update_dict(data[dict_key], table_distances, column_bases, disconnected, multiply_antigen_titer_until_column_adjust);

} // acmacs::chart::rjson_import::update

template void acmacs::chart::rjson_import::update<float>(const rjson::object& data, const char* list_key, const char* dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust);
template void acmacs::chart::rjson_import::update<double>(const rjson::object& data, const char* list_key, const char* dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
