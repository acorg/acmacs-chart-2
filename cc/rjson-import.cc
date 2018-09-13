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
            number_of_sera_ = static_cast<const rjson::v1::array&>(list[0]).size();
        }
        else {
            const rjson::v1::array& d = data_[keys_.dict];
            auto max_index = [](const rjson::v1::object& obj) -> size_t {
                                 size_t result = 0;
                                 for (auto key_value: obj) {
                                     if (const size_t ind = std::stoul(std::string(key_value.first.str())); ind > result)
                                         result = ind;
                                 }
                                 return result;
                             };
            number_of_sera_ = max_index(*std::max_element(d.begin(), d.end(), [max_index](const rjson::v1::object& a, const rjson::v1::object& b) { return max_index(a) < max_index(b); })) + 1;
        }
    }
    return *number_of_sera_;

} // acmacs::chart::RjsonTiters::number_of_sera

// ----------------------------------------------------------------------

std::vector<acmacs::chart::Titer> acmacs::chart::RjsonTiters::titers_for_layers(const rjson::v1::array& layers, size_t aAntigenNo, size_t aSerumNo) const
{
    if (layers.empty())
        throw acmacs::chart::data_not_available("no layers");
    const auto serum_no = std::to_string(aSerumNo);
    std::vector<Titer> result;
    for (const rjson::v1::array& layer: layers) {
        const rjson::v1::object& for_ag = layer[aAntigenNo];
        try {
            result.push_back(for_ag[serum_no]);
        }
        catch (rjson::v1::field_not_found&) {
        }
    }
    return result;

} // acmacs::chart::RjsonTiters::titers_for_layers

// ----------------------------------------------------------------------

size_t acmacs::chart::RjsonTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    if (auto [present, list] = data_.get_array_if(keys_.list); present) {
        for (const rjson::v1::array& row: list) {
            for (const Titer titer: row) {
                if (!titer.is_dont_care())
                    ++result;
            }
        }
    }
    else {
        const rjson::v1::array& d = data_[keys_.dict];
        result = std::accumulate(d.begin(), d.end(), size_t{0}, [](size_t a, const rjson::v1::object& row) -> size_t { return a + row.size(); });
    }
    return result;

} // acmacs::chart::RjsonTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

namespace                       // to make class static in the module
{
    class TiterIteratorImplementationList : public acmacs::chart::TiterIterator::Implementation
    {
     public:
        TiterIteratorImplementationList(const rjson::v1::array& titer_data)
            : row_{titer_data.begin()}, last_row_{titer_data.end()}
            {
                init_list_column();
                data_.antigen = 0;
                data_.serum = 0;
                data_.titer = list_column_->str();
                if (data_.titer.is_dont_care())
                    operator++();
            }

        TiterIteratorImplementationList(size_t number_of_antigens)
            : acmacs::chart::TiterIterator::Implementation({}, number_of_antigens, 0) {}

        void operator++() override
            {
                while (row_ != last_row_) {
                    if (++list_column_ == last_list_column_) {
                        ++row_;
                        if (row_ != last_row_)
                            init_list_column();
                        ++data_.antigen;
                        data_.serum = 0;
                    }
                    else
                        ++data_.serum;
                    if (row_ != last_row_) {
                        data_.titer = list_column_->str();
                        if (!data_.titer.is_dont_care())
                            break;
                    }
                    else
                        data_.titer = acmacs::chart::Titer{};
                }
            }

     private:
        rjson::v1::array::iterator row_, last_row_;
        rjson::v1::array::iterator list_column_, last_list_column_;

        void init_list_column()
            {
                list_column_ = static_cast<const rjson::v1::array&>(*row_).begin();
                last_list_column_ = static_cast<const rjson::v1::array&>(*row_).end();
            }

    }; // class TiterIteratorImplementationList

    class TiterIteratorImplementationDict : public acmacs::chart::TiterIterator::Implementation
    {
     public:
        TiterIteratorImplementationDict(const rjson::v1::array& titer_data)
            : row_{titer_data.begin()}, last_row_{titer_data.end()}
            {
                init_dict_column();
                data_.antigen = 0;
                if (dict_column_ != last_dict_column_) {
                    data_.serum = std::stoul(dict_column_->first.str());
                    data_.titer = dict_column_->second.str();
                }
                else {
                    operator++();
                }
            }

            TiterIteratorImplementationDict(size_t number_of_antigens) : acmacs::chart::TiterIterator::Implementation({}, number_of_antigens, 0) {}

            void operator++() override
            {
                if (dict_column_ != last_dict_column_)
                    ++dict_column_;
                if (dict_column_ == last_dict_column_) {
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
        rjson::v1::array::iterator row_, last_row_;
        rjson::v1::object::const_iterator dict_column_, last_dict_column_;

        void init_dict_column()
            {
                dict_column_ = static_cast<const rjson::v1::object&>(*row_).begin();
                last_dict_column_ = static_cast<const rjson::v1::object&>(*row_).end();
            }

    }; // class TiterIteratorImplementationDict
}

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::RjsonTiters::begin() const
{
    if (auto [present, list] = data_.get_array_if(keys_.list); present)
        return {new TiterIteratorImplementationList(list)};
    else
        return {new TiterIteratorImplementationDict(static_cast<const rjson::v1::array&>(data_[keys_.dict]))};

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

acmacs::chart::rjson_import::Layout::Layout(const rjson::v1::array& aData)
    : acmacs::Layout(aData.size(), rjson_import::number_of_dimensions(aData))
{
    auto coord = Vec::begin();
    for (const rjson::v1::array& point : aData) {
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

template <typename Float> static void update_list(const rjson::v1::array& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            const rjson::v1::array& row = data[p1];
            for (auto serum_no : acmacs::range(row.size())) {
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.contains(p2)) {
                    table_distances.update(row[serum_no], p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_list

template <typename Float> static void update_dict(const rjson::v1::array& data, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            const rjson::v1::object& row = data[p1];
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

template <typename Float> void acmacs::chart::rjson_import::update(const rjson::v1::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points)
{
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (auto [present, list] = data.get_array_if(list_key); present)
        ::update_list(list, table_distances, column_bases, parameters, number_of_points);
    else
        ::update_dict(data[dict_key], table_distances, column_bases, parameters, number_of_points);

} // acmacs::chart::rjson_import::update

template void acmacs::chart::rjson_import::update<float>(const rjson::v1::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
template void acmacs::chart::rjson_import::update<double>(const rjson::v1::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);

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
