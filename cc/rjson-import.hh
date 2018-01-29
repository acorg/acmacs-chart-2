#pragma once

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <optional>

#include "acmacs-base/rjson.hh"
#include "acmacs-base/layout.hh"
#include "acmacs-chart-2/titers.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    template <typename Float> class TableDistances;
    class ColumnBases;
    class PointIndexList;

} // namespace acmacs::chart


namespace acmacs::chart::rjson_import
{
    inline size_t number_of_dimensions(const rjson::array& data) noexcept
    {
        for (const rjson::array& row: data) {
            if (!row.empty())
                return row.size();
        }
        return 0;

          // try {
          //     for (const rjson::array& row: data) {
          //         if (!row.empty())
          //             return row.size();
          //     }
          // }
          // catch (rjson::field_not_found&) {
          // }
          // catch (std::bad_variant_access&) {
          // }
          // return 0;
    }

// ----------------------------------------------------------------------

    class Layout : public acmacs::Layout
    {
     public:
        Layout(const rjson::array& aData);

        void set(size_t /*aPointNo*/, const acmacs::Coordinates& /*aCoordinates*/) override;

    }; // class Layout

// ----------------------------------------------------------------------

    template <typename Float> void update(const rjson::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
    extern template void update<float>(const rjson::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
    extern template void update<double>(const rjson::object& data, const std::string& list_key, const std::string& dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);

} // namespace acmacs::chart::rjson

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class RjsonTiters : public Titers
    {
     public:
        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override
            {
                if (auto [present, list] = data_.get_array_if(keys_.list); present)
                    return list[aAntigenNo][aSerumNo];
                else
                    return titer_in_d(data_[keys_.dict], aAntigenNo, aSerumNo);
            }

        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override
            {
                return titer_in_d(data_[keys_.layers][aLayerNo], aAntigenNo, aSerumNo);
            }

        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const override
            {
                return titers_for_layers(data_.get_or_empty_array(keys_.layers), aAntigenNo, aSerumNo);
            }

        size_t number_of_layers() const override { return data_.get_or_empty_array(keys_.layers).size(); }

        size_t number_of_antigens() const override
            {
                if (!number_of_antigens_) {
                    if (auto [present, list] = data_.get_array_if(keys_.list); present)
                        number_of_antigens_ = list.size();
                    else
                        number_of_antigens_ = static_cast<const rjson::array&>(data_[keys_.dict]).size();
                }
                return *number_of_antigens_;
            }

        size_t number_of_sera() const override;

        size_t number_of_non_dont_cares() const override;

          // support for fast exporting into ace, if source was ace or acd1
        const rjson::array& rjson_list_list() const override { const rjson::array& r = data_.get_or_empty_array(keys_.list); if (r.empty()) throw data_not_available{"no \"" + keys_.list + "\""}; return r; }
        const rjson::array& rjson_list_dict() const override { const rjson::array& r = data_.get_or_empty_array(keys_.dict); if (r.empty()) throw data_not_available{"no \"" + keys_.dict + "\""}; return r; }
        const rjson::array& rjson_layers() const override { const rjson::array& r = data_.get_or_empty_array(keys_.layers); if (r.empty()) throw data_not_available{"no \"" + keys_.layers + "\""}; return r; }

        void update(TableDistances<float>& table_distances, const ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const override
            {
                if (number_of_sera())
                    rjson_import::update(data_, keys_.list, keys_.dict, table_distances, column_bases, parameters, number_of_antigens() + number_of_sera());
                else
                    throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
            }

        void update(TableDistances<double>& table_distances, const ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const override
            {
                if (number_of_sera())
                    rjson_import::update(data_, keys_.list, keys_.dict, table_distances, column_bases, parameters, number_of_antigens() + number_of_sera());
                else
                    throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
            }

     protected:
        struct Keys
        {
            std::string list;
            std::string dict;
            std::string layers;
        };

        RjsonTiters(const rjson::object& data, const Keys& keys) : data_{data}, keys_{keys} {}

        // const rjson::object& data() const { return data_; }
        const rjson::object& layer(size_t aLayerNo) const { return rjson_layers()[aLayerNo]; }

        Titer titer_in_d(const rjson::array& aSource, size_t aAntigenNo, size_t aSerumNo) const
            {
                try {
                    return aSource[aAntigenNo][std::to_string(aSerumNo)];
                }
                catch (rjson::field_not_found&) {
                    return "*";
                }
            }

        std::vector<Titer> titers_for_layers(const rjson::array& layers, size_t aAntigenNo, size_t aSerumNo) const;

        const rjson::object& data() const { return data_; }

     private:
        const rjson::object& data_;
        const Keys& keys_;
        mutable std::optional<size_t> number_of_antigens_;
        mutable std::optional<size_t> number_of_sera_;

    }; // class RjsonTiters

// ----------------------------------------------------------------------

    class RjsonProjection : public Projection
    {
      public:
        std::optional<double> stored_stress() const override { return data_.get<double>(keys_.stress); }
        std::shared_ptr<Layout> layout() const override { if (!layout_) layout_ = std::make_shared<rjson_import::Layout>(data_.get_or_empty_array(keys_.layout)); return layout_; }
        size_t number_of_dimensions() const override { return rjson_import::number_of_dimensions(data_.get_or_empty_array(keys_.layout)); }
        std::string comment() const override { return data_.get_or_default(keys_.comment, ""); }
        size_t number_of_points() const override { return data_.get_or_empty_array(keys_.layout).size(); }
        PointIndexList disconnected() const override;

     protected:
        struct Keys
        {
            std::string stress;
            std::string layout;
            std::string comment;
        };

        RjsonProjection(const Chart& chart, const rjson::object& data, const Keys& keys) : Projection(chart), data_{data}, keys_{keys} {}
        RjsonProjection(const Chart& chart, const rjson::object& data, const Keys& keys, size_t projection_no) : RjsonProjection(chart, data, keys) { set_projection_no(projection_no); }

        const rjson::object& data() const { return data_; }

        virtual PointIndexList make_disconnected() const = 0;

     private:
        const rjson::object& data_;
        const Keys& keys_;
        mutable std::shared_ptr<Layout> layout_;

    }; // class RjsonProjection

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
