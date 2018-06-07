#pragma once

#include "acmacs-chart-2/verify.hh"
#include "acmacs-chart-2/rjson-import.hh"

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
        AceChart(rjson::value&& aSrc) : mData{std::move(aSrc)} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        bool is_merge() const override;

        void verify_data(Verify aVerify) const;

     private:
        rjson::value mData;
        mutable ace::name_index_t mAntigenNameIndex;
        mutable ProjectionsP projections_;

    }; // class AceChart

    bool is_ace(const std::string_view& aData);
    ChartP ace_import(const std::string_view& aData, Verify aVerify);

// ----------------------------------------------------------------------

    class AceInfo : public Info
    {
      public:
        AceInfo(const rjson::value& aData) : mData{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        std::string virus(Compute aCompute = Compute::No) const override { return make_field("v", "+", aCompute); }
        std::string virus_type(Compute aCompute = Compute::Yes) const override { return make_field("V", "+", aCompute); }
        std::string subset(Compute aCompute = Compute::No) const override { return make_field("s", "+", aCompute); }
        std::string assay(Compute aCompute = Compute::No) const override { return make_field("A", "+", aCompute); }
        std::string lab(Compute aCompute = Compute::No) const override { return make_field("l", "+", aCompute); }
        std::string rbc_species(Compute aCompute = Compute::No) const override { return make_field("r", "+", aCompute); }
        std::string date(Compute aCompute = Compute::No) const override;
        size_t number_of_sources() const override { return mData.get_or_empty_array("S").size(); }
        InfoP source(size_t aSourceNo) const override { return std::make_shared<AceInfo>(mData.get_or_empty_array("S")[aSourceNo]); }

     private:
        const rjson::value& mData;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class AceInfo

// ----------------------------------------------------------------------

    class AceAntigen : public Antigen
    {
      public:
        AceAntigen(const rjson::object& aData) : mData{aData} {}

        Name name() const override { return mData["N"]; }
        Date date() const override { return mData.get_or_default("D", ""); }
        Passage passage() const override { return mData.get_or_default("P", ""); }
        BLineage lineage() const override;
        Reassortant reassortant() const override { return mData.get_or_default("R", ""); }
        LabIds lab_ids() const override { return mData.get_or_empty_array("l"); }
        Clades clades() const override { return mData.get_or_empty_array("c"); }
        Annotations annotations() const override { return mData.get_or_empty_array("a"); }
        bool reference() const override { return mData.get_or_default("S", "").find("R") != std::string::npos; }

     private:
        const rjson::object& mData;

    }; // class AceAntigen

// ----------------------------------------------------------------------

    class AceSerum : public Serum
    {
      public:
        AceSerum(const rjson::object& aData) : mData{aData} {}

        Name name() const override { return mData["N"]; }
        Passage passage() const override { return mData.get_or_default("P", ""); }
        BLineage lineage() const override;
        Reassortant reassortant() const override { return mData.get_or_default("R", ""); }
        Annotations annotations() const override { return mData.get_or_empty_array("a"); }
        SerumId serum_id() const override { return mData.get_or_default("I", ""); }
        SerumSpecies serum_species() const override { return mData.get_or_default("s", ""); }
        PointIndexList homologous_antigens() const override { return mData.get_or_empty_array("h"); }
        void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::object&>(mData).set_field("h", rjson::array(rjson::array::use_iterator, ags.begin(), ags.end())); }

     private:
        const rjson::object& mData;

    }; // class AceSerum

// ----------------------------------------------------------------------

    class AceAntigens : public Antigens
    {
     public:
        AceAntigens(const rjson::array& aData, ace::name_index_t& aAntigenNameIndex) : mData{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        size_t size() const override { return mData.size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<AceAntigen>(mData[aIndex]); }
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
        AceSera(const rjson::array& aData) : mData{aData} {}

        size_t size() const override { return mData.size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<AceSerum>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceSera

// ----------------------------------------------------------------------

    class AceTiters : public RjsonTiters
    {
      public:
        AceTiters(const rjson::object& data) : RjsonTiters(data, s_keys_) {}

     private:
        static const Keys s_keys_;

    }; // class AceTiters

// ----------------------------------------------------------------------

    class AceColumnBases : public ColumnBases
    {
      public:
        AceColumnBases(const rjson::array& data) : data_{data} {}
        AceColumnBases(const rjson::array& data, MinimumColumnBasis minimum_column_basis) : data_{data}, minimum_column_basis_{minimum_column_basis} {}

        double column_basis(size_t aSerumNo) const override { return minimum_column_basis_.apply(data_[aSerumNo]); }
        size_t size() const override { return data_.size(); }

     private:
        const rjson::array& data_;
        MinimumColumnBasis minimum_column_basis_;

    }; // class AceColumnBases

// ----------------------------------------------------------------------

    class AceProjection : public RjsonProjection
    {
      public:
        AceProjection(const Chart& chart, const rjson::object& aData) : RjsonProjection(chart, aData, s_keys_) {}
        AceProjection(const Chart& chart, const rjson::object& aData, size_t projection_no) : RjsonProjection(chart, aData, s_keys_, projection_no) {}

        // std::optional<double> stored_stress() const override { return mData.get<double>("s"); }
        // std::shared_ptr<Layout> layout() const override;
        // std::string comment() const override { return mData.get_or_default("c", ""); }
        // size_t number_of_points() const override { return mData.get_or_empty_array("l").size(); }
        // size_t number_of_dimensions() const override;
        MinimumColumnBasis minimum_column_basis() const override { return data().get_or_default("m", "none"); }
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        bool dodgy_titer_is_regular() const override { return data().get_or_default("d", false); }
        double stress_diff_to_stop() const override { return data().get_or_default("d", 0.0); }
        PointIndexList unmovable() const override { return data().get_or_empty_array("U"); }
        PointIndexList unmovable_in_the_last_dimension() const override { return data().get_or_empty_array("u"); }
        AvidityAdjusts avidity_adjusts() const override { return data().get_or_empty_array("f"); }

     protected:
        PointIndexList make_disconnected() const override { return data().get_or_empty_array("D"); }

     private:
        static const Keys s_keys_;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AceProjections : public Projections
    {
      public:
        AceProjections(const Chart& chart, const rjson::array& aData) : Projections(chart), mData{aData}, projections_(aData.size(), nullptr) {}

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<AceProjection>(chart(), mData[aIndex], aIndex);
                return projections_[aIndex];
            }

     private:
        const rjson::array& mData;
        mutable std::vector<ProjectionP> projections_;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AcePlotSpec : public PlotSpec
    {
      public:
        AcePlotSpec(const rjson::object& aData, const AceChart& aChart) : mData{aData}, mChart{aChart} {}

        bool empty() const override { return mData.empty(); }
        DrawingOrder drawing_order() const override { return mData.get_or_empty_array("d"); }
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;
        size_t number_of_points() const override;

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
