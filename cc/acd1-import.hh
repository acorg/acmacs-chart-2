#pragma once

#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/verify.hh"
#include "acmacs-chart-2/rjson-import.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace acd1
    {
        using name_index_t = std::map<std::string, std::vector<size_t>>;
    }

    class Acd1Chart : public Chart
    {
      public:
        Acd1Chart(rjson::value&& aSrc) : data_{std::move(aSrc)} {}

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
        rjson::value data_;
        mutable acd1::name_index_t mAntigenNameIndex;
        mutable ProjectionsP projections_;

    }; // class Acd1Chart

    inline bool is_acd1(const std::string_view& aData)
    {
        if (aData.size() < 100)
            return false;
        const auto start = aData.find("data = {");
        if (start == std::string_view::npos)
            return false;
        // const auto ver = aData.find("'version': 4,", start + 8);
        // if (ver == std::string_view::npos)
        //     return false;
        return true;
    }

    ChartP acd1_import(const std::string_view& aData, Verify aVerify);

// ----------------------------------------------------------------------

    class Acd1Info : public Info
    {
      public:
        Acd1Info(const rjson::value& aData) : data_{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        Virus       virus(Compute aCompute = Compute::No) const override { return make_field("virus", "+", aCompute); }
        VirusType   virus_type(Compute aCompute = Compute::Yes) const override { return make_field("virus_type", "+", aCompute); }
        std::string subset(Compute aCompute = Compute::No) const override { return make_field("virus_subset", "+", aCompute); }
        Assay       assay(Compute aCompute = Compute::No) const override { return make_field("assay", "+", aCompute); }
        Lab         lab(Compute aCompute = Compute::No, FixLab fix = FixLab::yes) const override { return fix_lab_name(make_field("lab", "+", aCompute), fix); }
        RbcSpecies  rbc_species(Compute aCompute = Compute::No) const override { return make_field("rbc_species", "+", aCompute); }
        TableDate   date(Compute aCompute = Compute::No) const override;
        size_t number_of_sources() const override { return data_["sources"].size(); }
        InfoP source(size_t aSourceNo) const override { return std::make_shared<Acd1Info>(data_["sources"][aSourceNo]); }

     private:
        const rjson::value& data_;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class Acd1Info

// ----------------------------------------------------------------------

    class Acd1Antigen : public Antigen
    {
      public:
        Acd1Antigen(const rjson::value& aData) : data_{aData} {}

        Name name() const override;
        Date date() const override { return data_["date"].get_or_default(""); }
        acmacs::virus::Passage passage() const override;
        BLineage lineage() const override;
        acmacs::virus::Reassortant reassortant() const override;
        LabIds lab_ids() const override;
        Clades clades() const override { return {}; /* not implemented */ }
        Annotations annotations() const override;
        bool reference() const override { return data_["reference"].get_or_default(false); }

     private:
        const rjson::value& data_;

    }; // class Acd1Antigen

// ----------------------------------------------------------------------

    class Acd1Serum : public Serum
    {
      public:
        Acd1Serum(const rjson::value& aData) : data_{aData} {}

        Name name() const override;
        acmacs::virus::Passage passage() const override;
        BLineage lineage() const override;
        acmacs::virus::Reassortant reassortant() const override;
        Annotations annotations() const override;
        SerumId serum_id() const override;
        SerumSpecies serum_species() const override { return data_["serum_species"].get_or_default(""); }
        PointIndexList homologous_antigens() const override { return data_["*homologous"]; }
        void set_homologous(const std::vector<size_t>& ags, acmacs::debug) const override { const_cast<rjson::value&>(data_)["*homologous"] = rjson::array(ags.begin(), ags.end()); }

     private:
        const rjson::value& data_;
        // PointIndexList mHomologous;

    }; // class Acd1Serum

// ----------------------------------------------------------------------

    class Acd1Antigens : public Antigens
    {
     public:
        Acd1Antigens(const rjson::value& aData, acd1::name_index_t& aAntigenNameIndex) : data_{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        size_t size() const override { return data_.size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<Acd1Antigen>(data_[aIndex]); }
        std::optional<size_t> find_by_full_name(std::string_view aFullName) const override;

     private:
        const rjson::value& data_;
        acd1::name_index_t& mAntigenNameIndex;

        void make_name_index() const;

    }; // class Acd1Antigens

// ----------------------------------------------------------------------

    class Acd1Sera : public Sera
    {
      public:
        Acd1Sera(const rjson::value& aData) : data_{aData} {}

        size_t size() const override { return data_.size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<Acd1Serum>(data_[aIndex]); }

     private:
        const rjson::value& data_;

    }; // class Acd1Sera

// ----------------------------------------------------------------------

    class Acd1Titers : public RjsonTiters
    {
      public:
        Acd1Titers(const rjson::value& data) : RjsonTiters(data, s_keys_) {}

          // old acd1 files have minimum_column_basis inside titers instead of projection
        std::optional<MinimumColumnBasis> minimum_column_basis() const
            {
                if (const auto& mcb = data()["minimum_column_basis"]; !mcb.is_null())
                    return static_cast<std::string>(mcb);
                return {};
            }

     private:
        static const Keys s_keys_;

    }; // class Acd1Titers

// ----------------------------------------------------------------------

    class Acd1ColumnBases : public ColumnBases
    {
      public:
        Acd1ColumnBases(const rjson::value& data) : data_{data} {}
        Acd1ColumnBases(const rjson::value& data, MinimumColumnBasis minimum_column_basis) : data_{data}, minimum_column_basis_{minimum_column_basis} {}

        double column_basis(size_t aSerumNo) const override { return minimum_column_basis_.apply(data_[aSerumNo]); }
        size_t size() const override { return data_.size(); }

     private:
        const rjson::value& data_;
        MinimumColumnBasis minimum_column_basis_;

    }; // class Acd1ColumnBases

// ----------------------------------------------------------------------

    class Acd1Projection : public RjsonProjection
    {
      public:
        Acd1Projection(const Chart& chart, const rjson::value& aData) : RjsonProjection(chart, aData, s_keys_) {}
        Acd1Projection(const Chart& chart, const rjson::value& aData, size_t projection_no) : RjsonProjection(chart, aData, s_keys_, projection_no) {}

        // std::optional<double> stored_stress() const override { return data_.get<double>("stress"); }
        // std::shared_ptr<Layout> layout() const override;
        // std::string comment() const override;
        // size_t number_of_points() const override { return data_["layout"].size(); }
        // size_t number_of_dimensions() const override;
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        enum dodgy_titer_is_regular dodgy_titer_is_regular() const override { if (const auto& sep = data()["stress_evaluator_parameters"]; !sep.is_null()) return sep["dodgy_titer_is_regular"].get_or_default(false) ? dodgy_titer_is_regular::yes : dodgy_titer_is_regular::no; else return dodgy_titer_is_regular::no; }
        double stress_diff_to_stop() const override { if (const auto& sep = data()["stress_evaluator_parameters"]; !sep.is_null()) return sep["stress_diff_to_stop"].get_or_default(0.0); else return 0.0; }
        PointIndexList unmovable() const override { return make_attributes(1); }
        PointIndexList unmovable_in_the_last_dimension() const override  { return make_attributes(3); }
        AvidityAdjusts avidity_adjusts() const override;

        MinimumColumnBasis minimum_column_basis() const override
            {
                if (const auto& sep = data()["stress_evaluator_parameters"]; !sep.is_null()) {
                    if (const auto& mcb = sep["minimum_column_basis"]; !mcb.is_null())
                        return static_cast<std::string>(mcb);
                }
                if (const auto mcb = dynamic_cast<Acd1Titers&>(*chart().titers()).minimum_column_basis(); mcb)
                    return *mcb;
                else
                    return std::string{"none"};
            }

     protected:
        PointIndexList make_disconnected() const override { return make_attributes(2); }

     private:
        static const Keys s_keys_;

        PointIndexList make_attributes(size_t aAttr) const;

    }; // class Acd1Projections

// ----------------------------------------------------------------------

    class Acd1Projections : public Projections
    {
      public:
        Acd1Projections(const Chart& chart, const rjson::value& aData) : Projections(chart), data_{aData}, projections_(aData.size(), nullptr) {}

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<Acd1Projection>(chart(), data_[aIndex], aIndex);
                return projections_[aIndex];
            }

     private:
        const rjson::value& data_;
        mutable std::vector<ProjectionP> projections_;

    }; // class Acd1Projections

// ----------------------------------------------------------------------

    class Acd1PlotSpec : public PlotSpec
    {
      public:
        Acd1PlotSpec(const rjson::value& aData, const Acd1Chart& aChart) : data_{aData}, mChart{aChart} {}

        bool empty() const override { return data_.empty(); }
        DrawingOrder drawing_order() const override;
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;
        size_t number_of_points() const override;

     private:
        const rjson::value& data_;
        const Acd1Chart& mChart;

        PointStyle extract(const rjson::value& aSrc, size_t aPointNo, size_t aStyleNo) const;

    }; // class Acd1PlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
