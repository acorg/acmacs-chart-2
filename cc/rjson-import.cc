#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/rjson-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

size_t acmacs::chart::RjsonTiters::number_of_sera() const
{
    if (!number_of_sera_) {
        if (auto [present, list] = data_.get_array_if(keys_.list); present) {
            number_of_sera_ = static_cast<const rjson::array&>(list[0]).size();
        }
        else {
            const rjson::array& d = data_[keys_.dict];
            auto max_index = [](const rjson::object& obj) -> size_t {
                                 size_t result = 0;
                                 for (auto key_value: obj) {
                                     if (const size_t ind = std::stoul(std::string(key_value.first.str())); ind > result)
                                         result = ind;
                                 }
                                 return result;
                             };
            number_of_sera_ = max_index(*std::max_element(d.begin(), d.end(), [max_index](const rjson::object& a, const rjson::object& b) { return max_index(a) < max_index(b); })) + 1;
        }
    }
    return *number_of_sera_;

} // acmacs::chart::RjsonTiters::number_of_sera

// ----------------------------------------------------------------------

std::vector<acmacs::chart::Titer> acmacs::chart::RjsonTiters::titers_for_layers(const rjson::array& layers, size_t aAntigenNo, size_t aSerumNo) const
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

} // acmacs::chart::RjsonTiters::titers_for_layers

// ----------------------------------------------------------------------

size_t acmacs::chart::RjsonTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    if (auto [present, list] = data_.get_array_if(keys_.list); present) {
        for (const rjson::array& row: list) {
            for (const Titer titer: row) {
                if (!titer.is_dont_care())
                    ++result;
            }
        }
    }
    else {
        const rjson::array& d = data_[keys_.dict];
        result = std::accumulate(d.begin(), d.end(), 0U, [](size_t a, const rjson::object& row) -> size_t { return a + row.size(); });
    }
    return result;

} // acmacs::chart::RjsonTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

void acmacs::chart::RjsonTiters::begin(acmacs::chart::TiterIterator& iterator) const
{
    auto& data = iterator.data();
    if (auto [present, list] = data_.get_array_if(keys_.list); present) {
        data.antigen = 0;
        data.serum = 0;
        data.titer = list[0][0];
        if (data.titer.is_dont_care())
            next(iterator);
    }
    else {
        const rjson::array& d = data_[keys_.dict];
        data.antigen = 0;
        while (d[data.antigen].empty() && data.antigen < d.size()) {
            ++data.antigen;
        }
        if (data.antigen < d.size()) {
            const auto& first = static_cast<const rjson::object&>(d[data.antigen]).begin();
            data.serum = std::stoul(first->first.str());
            data.titer = first->second;
        }
        else {
            data.serum = 0;
            data.titer = "*";
        }
    }

} // acmacs::chart::RjsonTiters::begin

// ----------------------------------------------------------------------

void acmacs::chart::RjsonTiters::end(acmacs::chart::TiterIterator& iterator) const
{
    auto& data = iterator.data();
    data.antigen = number_of_antigens();
    data.serum = 0;
    data.titer = Titer("*");

} // acmacs::chart::RjsonTiters::end

// ----------------------------------------------------------------------

void acmacs::chart::RjsonTiters::next(acmacs::chart::TiterIterator& iterator) const
{
    auto& data = iterator.data();
    if (auto [present, list] = data_.get_array_if(keys_.list); present) {
        while (data.antigen < list.size()) {
            const rjson::array& row = list[data.antigen];
            if (++data.serum >= row.size()) {
                ++data.antigen;
                data.serum = 0;
            }
            if (data.antigen < list.size()) {
                data.titer = row[data.serum].str();
                if (!data.titer.is_dont_care())
                    return;
            }
        }
    }
    else {
        const auto num_sera = number_of_sera();
        const rjson::array& d = data_[keys_.dict];
        ++data.serum;
        while (data.antigen < d.size()) {
            for (const rjson::object& row = d[data.antigen]; data.serum < num_sera; ++data.serum) {
                try {
                    data.titer = row[std::to_string(data.serum)];
                    return;
                }
                catch (rjson::field_not_found&) {
                }
            }
            ++data.antigen;
            data.serum = 0;
        }
    }
    data.serum = 0;
    data.titer = Titer("*");

} // acmacs::chart::RjsonTiters::next

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

template <typename Float> static void update_list(const rjson::array& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.exist(p1)) {
            const rjson::array& row = data[p1];
            for (auto serum_no : acmacs::range(row.size())) {
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.exist(p2)) {
                    table_distances.update(row[serum_no], p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_list

template <typename Float> static void update_dict(const rjson::array& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.exist(p1)) {
            const rjson::object& row = data[p1];
            for (auto [serum_no_s, titer_s] : row) {
                const auto serum_no = std::stoul(std::string(serum_no_s.str()));
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.exist(p2)) {
                    table_distances.update(titer_s, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_dict

template <typename Float> void acmacs::chart::rjson_import::update(const rjson::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points)
{
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (auto [present, list] = data.get_array_if(list_key); present)
        ::update_list(list, table_distances, column_bases, parameters, number_of_points);
    else
        ::update_dict(data[dict_key], table_distances, column_bases, parameters, number_of_points);

} // acmacs::chart::rjson_import::update

template void acmacs::chart::rjson_import::update<float>(const rjson::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
template void acmacs::chart::rjson_import::update<double>(const rjson::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::chart::RjsonProjection::disconnected() const
{
    auto result = make_disconnected();
    if (result.empty()) {
          // infer disconnected from empty rows in layout
        auto lt = layout();
        for (size_t p_no = 0; p_no < lt->number_of_points(); ++p_no) {
            if (!lt->point_has_coordinates(p_no))
                result.push_back(p_no);
        }
    }
    return result;

} // acmacs::chart::RjsonProjection::disconnected

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
