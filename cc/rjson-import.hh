#pragma once

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <optional>

#include "acmacs-base/rjson2.hh"
namespace rjson = rjson2;
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
    inline size_t number_of_dimensions(const rjson::value& data) noexcept
    {
        if (const auto& non_empty = rjson::find_if(data, [](const auto& val) -> bool { return !val.empty(); }); !non_empty.is_null())
            return non_empty.size();
        else
            return 0;
    }

// ----------------------------------------------------------------------

    class Layout : public acmacs::Layout
    {
     public:
        Layout(const rjson::value& aData);

        void set(size_t /*aPointNo*/, const acmacs::Vector& /*aCoordinates*/) override;

    }; // class Layout

// ----------------------------------------------------------------------

    template <typename Float> void update(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<Float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
    extern template void update<float>(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);
    extern template void update<double>(const rjson::value& data, const std::string& list_key, const std::string& dict_key, TableDistances<double>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters, size_t number_of_points);

} // namespace acmacs::chart::rjson

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class RjsonTiters : public Titers
    {
     public:
        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override
            {
                if (const auto& list = data_[keys_.list]; !list.is_null())
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
                return titers_for_layers(data_[keys_.layers], aAntigenNo, aSerumNo);
            }

        size_t number_of_layers() const override { return data_[keys_.layers].size(); }

        size_t number_of_antigens() const override
            {
                if (!number_of_antigens_) {
                    if (const auto& list = data_[keys_.list]; !list.is_null())
                        number_of_antigens_ = list.size();
                    else
                        number_of_antigens_ = data_[keys_.dict].size();
                }
                return *number_of_antigens_;
            }

        size_t number_of_sera() const override;

        size_t number_of_non_dont_cares() const override;

          // support for fast exporting into ace, if source was ace or acd1
        const rjson::value& rjson_list_list() const override { if (const auto& list = data_[keys_.list]; !list.empty()) return list; else throw data_not_available{"no \"" + keys_.list + "\""}; }
        const rjson::value& rjson_list_dict() const override { if (const auto& dict = data_[keys_.dict]; !dict.empty()) return dict; else throw data_not_available{"no \"" + keys_.dict + "\""}; }
        const rjson::value& rjson_layers() const override { if (const auto& layers = data_[keys_.layers]; !layers.empty()) return layers; else throw data_not_available{"no \"" + keys_.layers + "\""}; }

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

        TiterIterator begin() const override;
        TiterIterator end() const override;

     protected:
        struct Keys
        {
            std::string list;
            std::string dict;
            std::string layers;
        };

        RjsonTiters(const rjson::value& data, const Keys& keys) : data_{data}, keys_{keys} {}

        // const rjson::value& data() const { return data_; }
        const rjson::value& layer(size_t aLayerNo) const { return rjson_layers()[aLayerNo]; }

        Titer titer_in_d(const rjson::value& aSource, size_t aAntigenNo, size_t aSerumNo) const
            {
                if (const auto& row = aSource[aAntigenNo]; !row.is_null())
                    if (const auto& titer = row[aSerumNo]; !titer.is_null())
                        return titer;
                return {};
            }

        std::vector<Titer> titers_for_layers(const rjson::value& layers, size_t aAntigenNo, size_t aSerumNo) const;

        const rjson::value& data() const { return data_; }

     private:
        const rjson::value& data_;
        const Keys& keys_;
        mutable std::optional<size_t> number_of_antigens_;
        mutable std::optional<size_t> number_of_sera_;

    }; // class RjsonTiters

// ----------------------------------------------------------------------

    class RjsonProjection : public Projection
    {
      public:
        std::optional<double> stored_stress() const override { if (const auto& stress = data_[keys_.stress]; !stress.is_null()) return stress; else return {}; }
        std::shared_ptr<Layout> layout() const override { if (!layout_) layout_ = std::make_shared<rjson_import::Layout>(data_[keys_.layout]); return layout_; }
        size_t number_of_dimensions() const override { return rjson_import::number_of_dimensions(data_[keys_.layout]); }
        std::string comment() const override { if (const auto& comment = data_[keys_.comment]; !comment.is_null()) return comment; else return ""; }
        size_t number_of_points() const override { return data_[keys_.layout].size(); }
        PointIndexList disconnected() const override;

     protected:
        struct Keys
        {
            std::string stress;
            std::string layout;
            std::string comment;
        };

        RjsonProjection(const Chart& chart, const rjson::value& data, const Keys& keys) : Projection(chart), data_{data}, keys_{keys} {}
        RjsonProjection(const Chart& chart, const rjson::value& data, const Keys& keys, size_t projection_no) : RjsonProjection(chart, data, keys) { set_projection_no(projection_no); }

        const rjson::value& data() const { return data_; }

        virtual PointIndexList make_disconnected() const = 0;

     private:
        const rjson::value& data_;
        const Keys& keys_;
        mutable std::shared_ptr<Layout> layout_;

    }; // class RjsonProjection

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
