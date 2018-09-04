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
            size_t number_of_sera = 0;
            rjson::for_each(data_[keys_.dict], [&number_of_sera](const auto& row) { number_of_sera = std::max(number_of_sera, row.max_index()); });
            number_of_sera_ = number_of_sera;
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
    rjson::for_each(layers, [&result,aAntigenNo,aSerumNo](const auto& layer) {
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
        rjson::for_each(list, [&result](const auto& row) {
            rjson::for_each(row, [&result](const auto& titer) {
                if (!Titer(titer).is_dont_care())
                    ++result;
            });
        });
    }
    else {
        rjson::for_each(data_[keys_.dict], [&result](const auto& row) { result += row.size(); });
    }
    return result;

} // acmacs::chart::RjsonTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

namespace                       // to make class static in the module
{
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

        // TiterIteratorImplementationList(size_t number_of_antigens)
        //     : acmacs::chart::TiterIterator::Implementation({}, number_of_antigens, 0) {}

        void operator++() override
            {
                while (data_.antigen < number_of_rows()) {
                    if (++data_.serum == row().size()) {
                        ++data_.antigen;
                        data_.serum = 0;
                    }
                    else
                        ++data_.serum;
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
            : titer_data_{titer_data}
            // : row_{titer_data.begin()}, last_row_{titer_data.end()}
            {
                if (!row().empty()) {
                    data_.antigen = 0;
                    for (data_.serum = 0; ; ++data_.serum) {
                        if (const auto& titer = row()[data_.serum]; !titer.is_null()) {
                            data_.titer = titer;
                            break;
                        }
                        if (data_.serum > 1000000)
                            throw std::runtime_error("internal error in " + DEBUG_LINE_FUNC_S);
                    }
                }
                else
                    operator++();
            }

        // TiterIteratorImplementationDict(size_t number_of_antigens)
        //     : acmacs::chart::TiterIterator::Implementation({}, number_of_antigens, 0) {}

        void operator++() override
            {
                if (++dict_column_ == last_dict_column_) {
                    for (++row_, ++data_.antigen; row_ != last_row_ && row_->empty(); ++row_, ++data_.antigen);
                    if (row_ != last_row_)
                        init_dict_column();
                }
                if (row_ != last_row_) {
                    data_.serum = std::stoul(dict_column_->first.str());
                    data_.titer = dict_column_->second.str();
                }
                else {
                    data_.serum = 0;
                    data_.titer = acmacs::chart::Titer{};
                }
            }

     private:
        const rjson::value& titer_data_;
        // rjson::array::iterator row_, last_row_;
        // rjson::object::const_iterator dict_column_, last_dict_column_;

        const rjson::value& row() const { return titer_data_[data_.antigen]; }
        const rjson::value& titer() const { return titer_data_[data_.antigen][data_.serum]; }
        size_t number_of_rows() const { return titer_data_.size(); }

        // void init_dict_column()
        //     {
        //         dict_column_ = static_cast<const rjson::object&>(*row_).begin();
        //         last_dict_column_ = static_cast<const rjson::object&>(*row_).end();
        //     }

    }; // class TiterIteratorImplementationDict
}

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::RjsonTiters::begin() const
{
    if (auto [present, list] = data_.get_array_if(keys_.list); present)
        return {new TiterIteratorImplementationList(list)};
    else
        return {new TiterIteratorImplementationDict(static_cast<const rjson::array&>(data_[keys_.dict]))};

} // acmacs::chart::RjsonTiters::begin

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::RjsonTiters::end() const
{
    if (auto [present, list] = data_.get_array_if(keys_.list); present)
        return {new TiterIteratorImplementationList(number_of_antigens())};
    else
        return {new TiterIteratorImplementationDict(number_of_antigens())};

} // acmacs::chart::RjsonTiters::end

// ----------------------------------------------------------------------

acmacs::chart::rjson_import::Layout::Layout(const rjson::value& aData)
    : acmacs::Layout(aData.size(), rjson_import::number_of_dimensions(aData))
{
    auto coord = Vec::begin();
    for (const rjson::array& point : aData) {
        if (point.size() == number_of_dimensions())
            std::transform(point.begin(), point.end(), coord, [](const auto& coordinate) -> double { return coordinate; });
        else if (!point.empty())
            throw invalid_data("rjson_import::Layout: point has invalid number of coordinates: " + std::to_string(point.size()) + ", expected 0 or " + std::to_string(number_of_dimensions()));
        coord += static_cast<decltype(coord)::difference_type>(number_of_dimensions());
    }

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
            const rjson::value& row = data[p1];
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
            const rjson::object& row = data[p1];
            for (auto [serum_no_s, titer_s] : row) {
                const auto serum_no = std::stoul(std::string(serum_no_s.str()));
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.contains(p2)) {
                    table_distances.update(titer_s, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_dict

template <typename Float> void acmacs::chart::rjson_import::update(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points)
{
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (auto [present, list] = data.get_array_if(list_key); present)
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
