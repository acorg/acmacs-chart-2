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

std::vector<acmacs::chart::Titer> acmacs::chart::RjsonTiters::titers_for_layers(const rjson::value& layers, size_t aAntigenNo, size_t aSerumNo, include_dotcare inc) const
{
    if (layers.empty())
        throw acmacs::chart::data_not_available("no layers");
    std::vector<Titer> result;
    rjson::for_each(layers, [&result, aAntigenNo, aSerumNo, inc](const rjson::value& layer) {
        if (const auto& for_ag = layer[aAntigenNo]; !for_ag.empty()) {
            if (const auto& titer = for_ag[aSerumNo]; !titer.is_null())
                result.emplace_back(titer.to<std::string_view>());
            else if (inc == include_dotcare::yes)
                result.push_back({});
        }
    });
    return result;

} // acmacs::chart::RjsonTiters::titers_for_layers

// ----------------------------------------------------------------------

std::vector<size_t> acmacs::chart::RjsonTiters::layers_with_antigen(const rjson::value& layers, size_t aAntigenNo) const
{
    if (layers.empty())
        throw acmacs::chart::data_not_available("no layers");
    std::vector<size_t> result;
    rjson::for_each(layers, [&result, aAntigenNo, num_sera = number_of_sera()](const rjson::value& layer, size_t layer_no) {
        if (const auto& for_ag = layer[aAntigenNo]; !for_ag.empty()) {
            for (size_t serum_no = 0; serum_no < num_sera; ++serum_no) {
                if (const auto& titer = for_ag[serum_no]; !titer.is_null()) {
                    result.push_back(layer_no);
                    break;
                }
            }
        }
    });
    return result;

} // acmacs::chart::RjsonTiters::layers_with_antigen

// ----------------------------------------------------------------------

size_t acmacs::chart::RjsonTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    if (const auto& list = data_[keys_.list]; !list.is_null()) {
        rjson::for_each(list, [&result](const rjson::value& row) {
            rjson::for_each(row, [&result](const rjson::value& titer) {
                if (!Titer(titer.to<std::string_view>()).is_dont_care())
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

namespace
{
    class TiterGetterExistingBase : public acmacs::chart::TiterIterator::TiterGetter
    {
      public:
        TiterGetterExistingBase(const rjson::value& titer_data) : titer_data_{titer_data} {}

        void last(acmacs::chart::TiterIterator::Data& data) const override
        {
            data.antigen = titer_data_.size();
            data.serum = 0;
        }

      protected:
        bool valid(const acmacs::chart::Titer& titer) const { return !titer.is_dont_care(); }
        size_t number_of_rows() const { return titer_data_.size(); }
        const rjson::value& row(acmacs::chart::TiterIterator::Data& data) const { return titer_data_[data.antigen]; }
        const rjson::value& titer(acmacs::chart::TiterIterator::Data& data) const { return titer_data_[data.antigen][data.serum]; }

      private:
        const rjson::value& titer_data_;
    };

    class TiterGetterExistingList : public TiterGetterExistingBase
    {
      public:
        using TiterGetterExistingBase::TiterGetterExistingBase;

        void first(acmacs::chart::TiterIterator::Data& data) const override
        {
            data.antigen = 0;
            data.serum = 0;
            data.titer = acmacs::chart::Titer{titer(data).to<std::string_view>()};
            if (!valid(data.titer))
                next(data);
        }

        void next(acmacs::chart::TiterIterator::Data& data) const override
        {
            while (data.antigen < number_of_rows()) {
                ++data.serum;
                if (data.serum == row(data).size()) {
                    ++data.antigen;
                    data.serum = 0;
                }
                if (data.antigen < number_of_rows()) {
                    data.titer = acmacs::chart::Titer{titer(data).to<std::string_view>()};
                    if (valid(data.titer))
                        break;
                }
                else
                    data.titer = acmacs::chart::Titer{};
            }
        }
    };

    class TiterGetterExistingDict : public TiterGetterExistingBase
    {
      public:
        using TiterGetterExistingBase::TiterGetterExistingBase;

        void first(acmacs::chart::TiterIterator::Data& data) const override
        {
            for (data.antigen = 0; data.antigen < number_of_rows() && row(data).empty(); ++data.antigen)
                ;
            if (data.antigen < number_of_rows())
                populate_sera(data);
        }

        void next(acmacs::chart::TiterIterator::Data& data) const override
        {
            ++serum_;
            if (serum_ == sera_.end()) {
                for (++data.antigen; data.antigen < number_of_rows() && row(data).empty(); ++data.antigen)
                    ;
                if (data.antigen < number_of_rows()) {
                    populate_sera(data);
                }
                else {
                    data.serum = 0;
                    data.titer = acmacs::chart::Titer{};
                }
            }
            else {
                data.serum = *serum_;
                data.titer = acmacs::chart::Titer{titer(data).to<std::string_view>()};
            }
        }

      private:
        mutable std::vector<size_t> sera_;
        mutable std::vector<size_t>::const_iterator serum_;

        void populate_sera(acmacs::chart::TiterIterator::Data& data) const
        {
            rjson::transform(row(data), sera_, [](const rjson::object::value_type& kv) -> size_t { return std::stoul(kv.first); });
            std::sort(sera_.begin(), sera_.end());
            serum_ = sera_.begin();
            data.serum = *serum_;
            data.titer = acmacs::chart::Titer{titer(data).to<std::string_view>()};
        }
    };
} // namespace

// ----------------------------------------------------------------------

acmacs::chart::TiterIteratorMaker acmacs::chart::RjsonTiters::titers_existing() const
{
    if (const auto& list = data_[keys_.list]; !list.is_null())
        return acmacs::chart::TiterIteratorMaker(std::make_shared<TiterGetterExistingList>(list));
    else
        return acmacs::chart::TiterIteratorMaker(std::make_shared<TiterGetterExistingDict>(data_[keys_.dict]));

} // acmacs::chart::RjsonTiters::titers_existing

// ----------------------------------------------------------------------


acmacs::chart::TiterIteratorMaker acmacs::chart::RjsonTiters::titers_existing_from_layer(size_t layer_no) const
{
    return acmacs::chart::TiterIteratorMaker(std::make_shared<TiterGetterExistingDict>(data_[keys_.layers][layer_no]));

} // acmacs::chart::RjsonTiters::titers_existing

// ----------------------------------------------------------------------


acmacs::chart::rjson_import::Layout::Layout(const rjson::value& aData)
    : acmacs::Layout(aData.size(), rjson_import::number_of_dimensions(aData))
{
    auto coord = Vec::begin();
    rjson::for_each(aData, [&coord,num_dim=number_of_dimensions()](const rjson::value& point) {
        if (point.size() == *num_dim)
            rjson::transform(point, coord, [](const rjson::value& coordinate) -> double { return coordinate.to<double>(); });
        else if (!point.empty())
            throw invalid_data("rjson_import::Layout: point has invalid number of coordinates: " + std::to_string(point.size()) + ", expected 0 or " + acmacs::to_string(num_dim));
        coord += static_cast<decltype(coord)::difference_type>(*num_dim);
    });

} // acmacs::chart::rjson_import::Layout::Layout

// ----------------------------------------------------------------------

static void update_list(const rjson::value& data, acmacs::chart::TableDistances& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            const auto& row = data[p1];
            for (auto serum_no : acmacs::range(row.size())) {
                const auto p2 = serum_no + data.size();
                if (!parameters.disconnected.contains(p2)) {
                    table_distances.update(acmacs::chart::Titer{row[serum_no].to<std::string_view>()}, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
                }
            }
        }
    }

} // update_list

static void update_dict(const rjson::value& data, acmacs::chart::TableDistances& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters, size_t number_of_points)
{
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    for (auto p1 : acmacs::range(data.size())) {
        if (!parameters.disconnected.contains(p1)) {
            rjson::for_each(data[p1], [num_antigens=data.size(),p1,&parameters,&table_distances,&column_bases,&logged_adjusts](std::string_view field_name, const rjson::value& field_value) {
                const auto serum_no = std::stoul(field_name);
                const auto p2 = serum_no + num_antigens;
                if (!parameters.disconnected.contains(p2))
                    table_distances.update(acmacs::chart::Titer{field_value.to<std::string_view>()}, p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.mult);
            });
        }
    }

} // update_dict

void acmacs::chart::rjson_import::update(const rjson::value& data, std::string_view list_key, const std::string& dict_key, TableDistances& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points)
{
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (const auto& list = data[list_key]; !list.is_null())
        ::update_list(list, table_distances, column_bases, parameters, number_of_points);
    else
        ::update_dict(data[dict_key], table_distances, column_bases, parameters, number_of_points);

} // acmacs::chart::rjson_import::update

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::chart::RjsonProjection::disconnected() const
{
    auto result = make_disconnected();
    if (result->empty()) {
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
