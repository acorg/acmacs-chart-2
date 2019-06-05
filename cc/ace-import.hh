#pragma once

#include "acmacs-chart-2/chart.hh"
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
        AceChart(rjson::value&& aSrc) : data_{std::move(aSrc)} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        bool is_merge() const override;

        void verify_data(Verify aVerify) const;

          // to obtain extension fields (e.g. group_sets, gui data)
        const rjson::value& extension_field(std::string field_name) const override;
        const rjson::value& extension_fields() const override;

     private:
        rjson::value data_;
        mutable ace::name_index_t mAntigenNameIndex;
        mutable ProjectionsP projections_;

    }; // class AceChart

    bool is_ace(const std::string_view& aData);
    ChartP ace_import(const std::string_view& aData, Verify aVerify);

// ----------------------------------------------------------------------

    class AceInfo : public Info
    {
      public:
        AceInfo(const rjson::value& aData) : data_{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        Virus       virus(Compute aCompute = Compute::No) const override { return make_field("v", "+", aCompute); }
        VirusType   virus_type(Compute aCompute = Compute::Yes) const override { return make_field("V", "+", aCompute); }
        std::string subset(Compute aCompute = Compute::No) const override { return make_field("s", "+", aCompute); }
        Assay       assay(Compute aCompute = Compute::No) const override { return make_field("A", "+", aCompute); }
        Lab         lab(Compute aCompute = Compute::No, FixLab fix = FixLab::yes) const override { return fix_lab_name(make_field("l", "+", aCompute), fix); }
        RbcSpecies  rbc_species(Compute aCompute = Compute::No) const override { return make_field("r", "+", aCompute); }
        TableDate   date(Compute aCompute = Compute::No) const override;
        size_t number_of_sources() const override { return data_["S"].size(); }
        InfoP source(size_t aSourceNo) const override { return std::make_shared<AceInfo>(data_["S"][aSourceNo]); }

     private:
        const rjson::value& data_;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class AceInfo

// ----------------------------------------------------------------------

    class AceAntigen : public Antigen
    {
      public:
        AceAntigen(const rjson::value& aData) : data_{aData} {}

        Name name() const override { return data_["N"]; }
        Date date() const override { return data_["D"].get_or_default(""); }
        acmacs::virus::Passage passage() const override { return acmacs::virus::Passage{data_["P"].get_or_default("")}; }
        BLineage lineage() const override;
        acmacs::virus::Reassortant reassortant() const override { return acmacs::virus::Reassortant{data_["R"].get_or_default("")}; }
        LabIds lab_ids() const override { return data_["l"]; }
        Clades clades() const override { return data_["c"]; }
        Annotations annotations() const override { return data_["a"]; }
        bool reference() const override { return data_["S"].get_or_default("").find("R") != std::string::npos; }

     private:
        const rjson::value& data_;

    }; // class AceAntigen

// ----------------------------------------------------------------------

    class AceSerum : public Serum
    {
      public:
        AceSerum(const rjson::value& aData) : data_{aData} {}

        Name name() const override { return data_["N"]; }
        acmacs::virus::Passage passage() const override { return acmacs::virus::Passage{data_["P"].get_or_default("")}; }
        BLineage lineage() const override;
        acmacs::virus::Reassortant reassortant() const override { return acmacs::virus::Reassortant{data_["R"].get_or_default("")}; }
        Annotations annotations() const override { return data_["a"]; }
        SerumId serum_id() const override { return data_["I"].get_or_default(""); }
        SerumSpecies serum_species() const override { return data_["s"].get_or_default(""); }
        PointIndexList homologous_antigens() const override { return data_["h"]; }
        void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::value&>(data_)["h"] = rjson::array(ags.begin(), ags.end()); }

     private:
        const rjson::value& data_;

    }; // class AceSerum

// ----------------------------------------------------------------------

    class AceAntigens : public Antigens
    {
     public:
        AceAntigens(const rjson::value& aData, ace::name_index_t& aAntigenNameIndex) : data_{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        size_t size() const override { return data_.size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<AceAntigen>(data_[aIndex]); }
        std::optional<size_t> find_by_full_name(std::string_view aFullName) const override;

     private:
        const rjson::value& data_;
        ace::name_index_t& mAntigenNameIndex;

        void make_name_index() const;

    }; // class AceAntigens

// ----------------------------------------------------------------------

    class AceSera : public Sera
    {
      public:
        AceSera(const rjson::value& aData) : data_{aData} {}

        size_t size() const override { return data_.size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<AceSerum>(data_[aIndex]); }

     private:
        const rjson::value& data_;

    }; // class AceSera

// ----------------------------------------------------------------------

    class AceTiters : public RjsonTiters
    {
      public:
        AceTiters(const rjson::value& data) : RjsonTiters(data, s_keys_) {}

     private:
        static const Keys s_keys_;

    }; // class AceTiters

// ----------------------------------------------------------------------

    class AceColumnBases : public ColumnBases
    {
      public:
        AceColumnBases(const rjson::value& data) : data_{data} {}
        AceColumnBases(const rjson::value& data, MinimumColumnBasis minimum_column_basis) : data_{data}, minimum_column_basis_{minimum_column_basis} {}

        double column_basis(size_t aSerumNo) const override { return minimum_column_basis_.apply(data_[aSerumNo]); }
        size_t size() const override { return data_.size(); }

     private:
        const rjson::value& data_;
        MinimumColumnBasis minimum_column_basis_;

    }; // class AceColumnBases

// ----------------------------------------------------------------------

    class AceProjection : public RjsonProjection
    {
      public:
        AceProjection(const Chart& chart, const rjson::value& aData) : RjsonProjection(chart, aData, s_keys_) {}
        AceProjection(const Chart& chart, const rjson::value& aData, size_t projection_no) : RjsonProjection(chart, aData, s_keys_, projection_no) {}

        MinimumColumnBasis minimum_column_basis() const override { return data()["m"].get_or_default("none"); }
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        enum dodgy_titer_is_regular dodgy_titer_is_regular() const override { return data()["d"].get_or_default(false) ? dodgy_titer_is_regular::yes : dodgy_titer_is_regular::no; }
        double stress_diff_to_stop() const override { return data()["d"].get_or_default(0.0); }
        PointIndexList unmovable() const override { return data()["U"]; }
        PointIndexList unmovable_in_the_last_dimension() const override { return data()["u"]; }
        AvidityAdjusts avidity_adjusts() const override { return data()["f"]; }

     protected:
        PointIndexList make_disconnected() const override { return data()["D"]; }

     private:
        static const Keys s_keys_;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AceProjections : public Projections
    {
      public:
        AceProjections(const Chart& chart, const rjson::value& aData) : Projections(chart), data_{aData}, projections_(aData.size(), nullptr) {}

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<AceProjection>(chart(), data_[aIndex], aIndex);
                return projections_[aIndex];
            }

     private:
        const rjson::value& data_;
        mutable std::vector<ProjectionP> projections_;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AcePlotSpec : public PlotSpec
    {
      public:
        AcePlotSpec(const rjson::value& aData, const AceChart& aChart) : data_{aData}, mChart{aChart} {}

        bool empty() const override { return data_.empty(); }
        DrawingOrder drawing_order() const override { return data_["d"]; }
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;
        size_t number_of_points() const override;

     private:
        const rjson::value& data_;
        const AceChart& mChart;

        PointStyle extract(const rjson::value& aSrc, size_t aPointNo, size_t aStyleNo) const;
        void label_style(PointStyle& aStyle, const rjson::value& aData) const;

    }; // class AcePlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
