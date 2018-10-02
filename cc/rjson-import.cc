#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/rjson-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

size_t acmacs::chart::RjsonTiters::number_of_sera() const
{
    if (!number_of_sera_) {
        if (const auto& list = data_[keys_.list]; !list.is_null()) {
            number_of_sera_ = list[0].size();
        }
        else {
            size_t max_serum_index = 0;
            rjson::for_each(data_[keys_.dict], [&max_serum_index](const rjson::value& row) { max_serum_index = std::max(max_serum_index, row.max_index()); });
            number_of_sera_ = max_serum_index + 1;
        }
    }
    return *number_of_sera_;

} // acmacs::chart::RjsonTiters::number_of_sera

// ----------------------------------------------------------------------

std::vector<acmacs::chart::Titer> acmacs::chart::RjsonTiters::titers_for_layers(const rjson::value& layers, size_t aAntigenNo, size_t aSerumNo) const
{
    if (layers.empty())
        throw acmacs::chart::data_not_available("no layers");
    std::vector<Titer> result;
    rjson::for_each(layers, [&result,aAntigenNo,aSerumNo](const rjson::value& layer) {
        if (const auto& for_ag = layer[aAntigenNo]; !for_ag.empty())
            if (const auto& titer = for_ag[aSerumNo]; !titer.is_null())
                result.push_back(titer);
    });
    return result;

} // acmacs::chart::RjsonTiters::titers_for_layers

// ----------------------------------------------------------------------

size_t acmacs::chart::RjsonTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    if (const auto& list = data_[keys_.list]; !list.is_null()) {
        rjson::for_each(list, [&result](const rjson::value& row) {
            rjson::for_each(row, [&result](const rjson::value& titer) {
                if (!Titer(titer).is_dont_care())
                    ++result;
            });
        });
    }
    else {
        rjson::for_each(data_[keys_.dict], [&result](const rjson::value& row) { result += row.size(); });
    }
    return result;

} // acmacs::chart::RjsonTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

namespace                       // to make class static in the module
{
    enum TiterIteratorImplementationEnd_ { TiterIteratorImplementationEnd };

    class TiterIteratorImplementationList : public acmacs::chart::TiterIterator::Implementation
    {
     public:
        TiterIteratorImplementationList(const rjson::value& titer_data)
            : titer_data_{titer_data}
            {
                data_.antigen = 0;
                data_.serum = 0;
                data_.titer = titer();
                if (data_.titer.is_dont_care())
                    operator++();
            }

        TiterIteratorImplementationList(TiterIteratorImplementationEnd_, const rjson::value& titer_data)
            : acmacs::chart::TiterIterator::Implementation({}, titer_data.size(), 0), titer_data_{titer_data} {}

        void operator++() override
            {
                while (data_.antigen < number_of_rows()) {
                    ++data_.serum;
                    if (data_.serum == row().size()) {
                        ++data_.antigen;
                        data_.serum = 0;
                    }
                    if (data_.antigen < number_of_rows()) {
                        data_.titer = titer();
                        if (!data_.titer.is_dont_care())
                            break;
                    }
                    else
                        data_.titer = acmacs::chart::Titer{};
                }
            }

     private:
        const rjson::value& titer_data_;

        const rjson::value& row() const { return titer_data_[data_.antigen]; }
        const rjson::value& titer() const { return titer_data_[data_.antigen][data_.serum]; }
        size_t number_of_rows() const { return titer_data_.size(); }

    }; // class TiterIteratorImplementationList

    class TiterIteratorImplementationDict : public acmacs::chart::TiterIterator::Implementation
    {
     public:
        TiterIteratorImplementationDict(const rjson::value& titer_data)
            : titer_data_{titer_data}, serum_{sera_.begin()}
            {
                for (data_.antigen = 0; data_.antigen < number_of_rows() && row().empty(); ++data_.antigen);
                if (data_.antigen < number_of_rows())
                    populate_sera();
            }

        TiterIteratorImplementationDict(TiterIteratorImplementationEnd_, const rjson::value& titer_data)
            : acmacs::chart::TiterIterator::Implementation({}, titer_data.size(), 0), titer_data_{titer_data} {}

        void operator++() override
            {
                ++serum_;
                if (serum_ == sera_.end()) {
                    for (++data_.antigen; data_.antigen < number_of_rows() && row().empty(); ++data_.antigen);
                    if (data_.antigen < number_of_rows()) {
                        populate_sera();
                    }
                    else {
                        data_.serum = 0;
                        data_.titer = acmacs::chart::Titer{};
                    }
                }
                else {
                    data_.serum = *serum_;
                    data_.titer = titer();
                }
            }

     private:
        const rjson::value& titer_data_;
        std::vector<size_t> sera_;
        std::vector<size_t>::const_iterator serum_;

        const rjson::value& row() const { return titer_data_[data_.antigen]; }
        const rjson::value& titer() const { return titer_data_[data_.antigen][data_.serum]; }
        size_t number_of_rows() const { return titer_data_.size(); }

        void populate_sera()
            {
                rjson::transform(row(), sera_, [](const rjson::object::value_type& kv) -> size_t { return std::stoul(kv.first); });
                std::sort(sera_.begin(), sera_.end());
                serum_ = sera_.begin();
                data_.serum = *serum_;
                data_.titer = titer();
            }

    }; // class TiterIteratorImplementationDict
}

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::RjsonTiters::begin() const
{
    if (const auto& list = data_[keys_.list]; !list.is_null())
        return {new TiterIteratorImplementationList(list)};
    else
        return {new TiterIteratorImplementationDict(data_[keys_.dict])};

} // acmacs::chart::RjsonTiters::begin

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::RjsonTiters::end() const
{
    if (const auto& list = data_[keys_.list]; !list.is_null())
        return {new TiterIteratorImplementationList(TiterIteratorImplementationEnd, list)};
    else
        return {new TiterIteratorImplementationDict(TiterIteratorImplementationEnd, data_[keys_.dict])};

} // acmacs::chart::RjsonTiters::end

// ----------------------------------------------------------------------

acmacs::chart::rjson_import::Layout::Layout(const rjson::value& aData)
    : acmacs::Layout(aData.size(), rjson_import::number_of_dimensions(aData))
{
    auto coord = Vec::begin();
    rjson::for_each(aData, [&coord,num_dim=number_of_dimensions()](const rjson::value& point) {
        if (point.size() == num_dim)
            rjson::transform(point, coord, [](const rjson::value& coordinate) -> double { return coordinate; });
        else if (!point.empty())
            throw invalid_data("rjson_import::Layout: point has invalid number of coordinates: " + std::to_string(point.size()) + ", expected 0 or " + std::to_string(num_dim));
        coord += static_cast<decltype(coord)::difference_type>(num_dim);
    });

} // acmacs::chart::rjson_import::Layout::Layout

// ----------------------------------------------------------------------

void acmacs::chart::rjson_import::Layout::set(size_t /*aPointNo*/, const acmacs::Vector& /*aCoordinates*/)
{
    throw acmacs::chart::chart_is_read_only{"rjson_import::Layout::set: cannot modify"};
}

// ----------------------------------------------------------------------

template <typename Float> static void update_list(const rjson::value& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            const auto& row = data[p1];
            for (auto serum_no : acmacs::range(row.size())) {
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.contains(p2)) {
                    table_distances.update(row[serum_no], p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_list

template <typename Float> static void update_dict(const rjson::value& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            rjson::for_each(data[p1], [num_antigens=data.size(),p1,&parameters,&table_distances,&column_bases,&logged_adjusts](const std::string& field_name, const rjson::value& field_value) {
                const auto serum_no = std::stoul(field_name);
                const auto p2 = serum_no + num_antigens;
                if (!parameters.disconnected.contains(p2))
                    table_distances.update(field_value, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
            });
        }
    }

} // update_dict

template <typename Float> void acmacs::chart::rjson_import::update(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points)
{
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (const auto& list = data[list_key]; !list.is_null())
        ::update_list(list, table_distances, column_bases, parameters, number_of_points);
    else
        ::update_dict(data[dict_key], table_distances, column_bases, parameters, number_of_points);

} // acmacs::chart::rjson_import::update

template void acmacs::chart::rjson_import::update<float>(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
template void acmacs::chart::rjson_import::update<double>(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::chart::RjsonProjection::disconnected() const
{
    auto result = make_disconnected();
    if (result.empty()) {
          // infer disconnected from empty rows in layout
        auto lt = layout();
        for (size_t p_no = 0; p_no < lt->number_of_points(); ++p_no) {
            if (!lt->point_has_coordinates(p_no))
                result.insert(p_no);
        }
    }
    return result;

} // acmacs::chart::RjsonProjection::disconnected

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
