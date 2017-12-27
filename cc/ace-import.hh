#pragma once

#include "acmacs-base/rjson.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/verify.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace ace
    {
        using name_index_t = std::map<std::string_view, std::vector<size_t>>;
    }

    class AceChart : public Chart
    {
      public:
        inline AceChart(rjson::value&& aSrc) : mData{std::move(aSrc)} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases() const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        bool is_merge() const override;

        void verify_data(Verify aVerify) const;

     private:
        rjson::value mData;
        mutable ace::name_index_t mAntigenNameIndex;

    }; // class AceChart

    bool is_ace(const std::string_view& aData);
    ChartP ace_import(const std::string_view& aData, Verify aVerify);

// ----------------------------------------------------------------------

    class AceInfo : public Info
    {
      public:
        inline AceInfo(const rjson::value& aData) : mData{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        inline std::string virus(Compute aCompute = Compute::No) const override { return make_field("v", "+", aCompute); }
        inline std::string virus_type(Compute aCompute = Compute::Yes) const override { return make_field("V", "+", aCompute); }
        inline std::string subset(Compute aCompute = Compute::No) const override { return make_field("s", "+", aCompute); }
        inline std::string assay(Compute aCompute = Compute::No) const override { return make_field("A", "+", aCompute); }
        inline std::string lab(Compute aCompute = Compute::No) const override { return make_field("l", "+", aCompute); }
        inline std::string rbc_species(Compute aCompute = Compute::No) const override { return make_field("r", "+", aCompute); }
        std::string date(Compute aCompute = Compute::No) const override;
        inline size_t number_of_sources() const override { return mData.get_or_empty_array("S").size(); }
        inline InfoP source(size_t aSourceNo) const override { return std::make_shared<AceInfo>(mData.get_or_empty_array("S")[aSourceNo]); }

     private:
        const rjson::value& mData;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class AceInfo

// ----------------------------------------------------------------------

    class AceAntigen : public Antigen
    {
      public:
        inline AceAntigen(const rjson::object& aData) : mData{aData} {}

        inline Name name() const override { return mData["N"]; }
        inline Date date() const override { return mData.get_or_default("D", ""); }
        inline Passage passage() const override { return mData.get_or_default("P", ""); }
        BLineage lineage() const override;
        inline Reassortant reassortant() const override { return mData.get_or_default("R", ""); }
        inline LabIds lab_ids() const override { return mData.get_or_empty_array("l"); }
        inline Clades clades() const override { return mData.get_or_empty_array("c"); }
        inline Annotations annotations() const override { return mData.get_or_empty_array("a"); }
        inline bool reference() const override { return mData.get_or_default("S", "").find("R") != std::string::npos; }

     private:
        const rjson::object& mData;

    }; // class AceAntigen

// ----------------------------------------------------------------------

    class AceSerum : public Serum
    {
      public:
        inline AceSerum(const rjson::object& aData) : mData{aData} {}

        inline Name name() const override { return mData["N"]; }
        inline Passage passage() const override { return mData.get_or_default("P", ""); }
        BLineage lineage() const override;
        inline Reassortant reassortant() const override { return mData.get_or_default("R", ""); }
        inline Annotations annotations() const override { return mData.get_or_empty_array("a"); }
        inline SerumId serum_id() const override { return mData.get_or_default("I", ""); }
        inline SerumSpecies serum_species() const override { return mData.get_or_default("s", ""); }
        inline PointIndexList homologous_antigens() const override { return mData.get_or_empty_array("h"); }
        inline void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::object&>(mData).set_field("h", rjson::array(rjson::array::use_iterator, ags.begin(), ags.end())); }

     private:
        const rjson::object& mData;

    }; // class AceSerum

// ----------------------------------------------------------------------

    class AceAntigens : public Antigens
    {
     public:
        inline AceAntigens(const rjson::array& aData, ace::name_index_t& aAntigenNameIndex) : mData{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        inline size_t size() const override { return mData.size(); }
        inline AntigenP operator[](size_t aIndex) const override { return std::make_shared<AceAntigen>(mData[aIndex]); }
        std::optional<size_t> find_by_full_name(std::string aFullName) const override;

     private:
        const rjson::array& mData;
        ace::name_index_t& mAntigenNameIndex;

        void make_name_index() const;

    }; // class AceAntigens

// ----------------------------------------------------------------------

    class AceSera : public Sera
    {
      public:
        inline AceSera(const rjson::array& aData) : mData{aData} {}

        inline size_t size() const override { return mData.size(); }
        inline SerumP operator[](size_t aIndex) const override { return std::make_shared<AceSerum>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceSera

// ----------------------------------------------------------------------

    class AceTiters : public Titers
    {
      public:
        inline AceTiters(const rjson::object& aData) : mData{aData} {}

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override;
        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const override;
        inline size_t number_of_layers() const override { return mData.get_or_empty_array("L").size(); }
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override;
        size_t number_of_non_dont_cares() const override;

          // support for fast exporting into ace, if source was ace or acd1
        inline const rjson::array& rjson_list_list() const override { const rjson::array& r = mData.get_or_empty_array("l"); if (r.empty()) throw data_not_available{"no \"l\""}; return r; }
        inline const rjson::array& rjson_list_dict() const override { const rjson::array& r = mData.get_or_empty_array("d"); if (r.empty()) throw data_not_available{"no \"d\""}; return r; }
        inline const rjson::array& rjson_layers() const override { const rjson::array& r = mData.get_or_empty_array("L"); if (r.empty()) throw data_not_available{"no \"L\""}; return r; }

     private:
        const rjson::object& mData;

        inline const rjson::object& layer(size_t aLayerNo) const { return rjson_layers()[aLayerNo]; }

        inline Titer titer_in_d(const rjson::array& aSource, size_t aAntigenNo, size_t aSerumNo) const
            {
                try {
                    return aSource[aAntigenNo][std::to_string(aSerumNo)];
                }
                catch (rjson::field_not_found&) {
                    return "*";
                }
            }

    }; // class AceTiters

// ----------------------------------------------------------------------

    class AceColumnBases : public ColumnBases
    {
      public:
        inline AceColumnBases(const rjson::array& aData) : mData{aData} {}

        inline double column_basis(size_t aSerumNo) const override { return mData[aSerumNo]; }
        inline size_t size() const override { return mData.size(); }

     private:
        const rjson::array& mData;

    }; // class AceColumnBases

// ----------------------------------------------------------------------

    class AceProjection : public Projection
    {
      public:
        inline AceProjection(const rjson::object& aData) : mData{aData} {}

        inline double stress() const override { return mData.get_or_default("s", 0.0); }
        std::shared_ptr<Layout> layout() const override;
        inline std::string comment() const override { return mData.get_or_default("c", ""); }
        inline size_t number_of_points() const override { return mData.get_or_empty_array("l").size(); }
        size_t number_of_dimensions() const override;
        inline MinimumColumnBasis minimum_column_basis() const override { return mData.get_or_default("m", "none"); }
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        inline bool dodgy_titer_is_regular() const override { return mData.get_or_default("d", false); }
        inline double stress_diff_to_stop() const override { return mData.get_or_default("d", 0.0); }
        inline PointIndexList unmovable() const override { return mData.get_or_empty_array("U"); }
        inline PointIndexList disconnected() const override { return mData.get_or_empty_array("D"); }
        inline PointIndexList unmovable_in_the_last_dimension() const override { return mData.get_or_empty_array("u"); }
        inline AvidityAdjusts avidity_adjusts() const override { return mData.get_or_empty_array("f"); }

     private:
        const rjson::object& mData;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AceProjections : public Projections
    {
      public:
        inline AceProjections(const rjson::array& aData) : mData{aData} {}

        inline bool empty() const override { return mData.empty(); }
        inline size_t size() const override { return mData.size(); }
        inline ProjectionP operator[](size_t aIndex) const override { return std::make_shared<AceProjection>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AcePlotSpec : public PlotSpec
    {
      public:
        inline AcePlotSpec(const rjson::object& aData, const AceChart& aChart) : mData{aData}, mChart{aChart} {}

        inline bool empty() const override { return mData.empty(); }
        inline DrawingOrder drawing_order() const override { return mData.get_or_empty_array("d"); }
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;

     private:
        const rjson::object& mData;
        const AceChart& mChart;

        PointStyle extract(const rjson::object& aSrc, size_t aPointNo, size_t aStyleNo) const;
        void label_style(PointStyle& aStyle, const rjson::object& aData) const;

    }; // class AcePlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
