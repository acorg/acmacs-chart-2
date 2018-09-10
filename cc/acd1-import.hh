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
        Acd1Chart(rjson::v1::value&& aSrc) : mData{std::move(aSrc)} {}

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
        rjson::v1::value mData;
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
        Acd1Info(const rjson::v1::value& aData) : mData{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        std::string virus(Compute aCompute = Compute::No) const override { return make_field("virus", "+", aCompute); }
        std::string virus_type(Compute aCompute = Compute::Yes) const override { return make_field("virus_type", "+", aCompute); }
        std::string subset(Compute aCompute = Compute::No) const override { return make_field("virus_subset", "+", aCompute); }
        std::string assay(Compute aCompute = Compute::No) const override { return make_field("assay", "+", aCompute); }
        std::string lab(Compute aCompute = Compute::No) const override { return make_field("lab", "+", aCompute); }
        std::string rbc_species(Compute aCompute = Compute::No) const override { return make_field("rbc_species", "+", aCompute); }
        std::string date(Compute aCompute = Compute::No) const override;
        size_t number_of_sources() const override { return mData.get_or_empty_array("sources").size(); }
        InfoP source(size_t aSourceNo) const override { return std::make_shared<Acd1Info>(mData.get_or_empty_array("sources")[aSourceNo]); }

     private:
        const rjson::v1::value& mData;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class Acd1Info

// ----------------------------------------------------------------------

    class Acd1Antigen : public Antigen
    {
      public:
        Acd1Antigen(const rjson::v1::object& aData) : mData{aData} {}

        Name name() const override;
        Date date() const override { return mData.get_or_default("date", ""); }
        Passage passage() const override;
        BLineage lineage() const override;
        Reassortant reassortant() const override;
        LabIds lab_ids() const override;
        Clades clades() const override { return {}; /* not implemented */ }
        Annotations annotations() const override;
        bool reference() const override { return mData.get_or_default("reference", false); }

     private:
        const rjson::v1::object& mData;

    }; // class Acd1Antigen

// ----------------------------------------------------------------------

    class Acd1Serum : public Serum
    {
      public:
        Acd1Serum(const rjson::v1::object& aData) : mData{aData} {}

        Name name() const override;
        Passage passage() const override;
        BLineage lineage() const override;
        Reassortant reassortant() const override;
        Annotations annotations() const override;
        SerumId serum_id() const override;
        SerumSpecies serum_species() const override { return mData.get_or_default("serum_species", ""); }
        PointIndexList homologous_antigens() const override { return mData.get_or_empty_array("*homologous"); }
        void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::v1::object&>(mData).set_field("*homologous", rjson::v1::array(rjson::v1::array::use_iterator, ags.begin(), ags.end())); }

     private:
        const rjson::v1::object& mData;
        // PointIndexList mHomologous;

    }; // class Acd1Serum

// ----------------------------------------------------------------------

    class Acd1Antigens : public Antigens
    {
     public:
        Acd1Antigens(const rjson::v1::array& aData, acd1::name_index_t& aAntigenNameIndex) : mData{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        size_t size() const override { return mData.size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<Acd1Antigen>(mData[aIndex]); }
        std::optional<size_t> find_by_full_name(std::string aFullName) const override;

     private:
        const rjson::v1::array& mData;
        acd1::name_index_t& mAntigenNameIndex;

        void make_name_index() const;

    }; // class Acd1Antigens

// ----------------------------------------------------------------------

    class Acd1Sera : public Sera
    {
      public:
        Acd1Sera(const rjson::v1::array& aData) : mData{aData} {}

        size_t size() const override { return mData.size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<Acd1Serum>(mData[aIndex]); }

     private:
        const rjson::v1::array& mData;

    }; // class Acd1Sera

// ----------------------------------------------------------------------

    class Acd1Titers : public RjsonTiters
    {
      public:
        Acd1Titers(const rjson::v1::object& data) : RjsonTiters(data, s_keys_) {}

          // old acd1 files have minimum_column_basis inside titers instead of projection
        std::optional<MinimumColumnBasis> minimum_column_basis() const
            {
                try {
                    return data()["minimum_column_basis"].strv();
                }
                catch (std::exception&) {
                    return {};
                }
            }

     private:
        static const Keys s_keys_;

    }; // class Acd1Titers

// ----------------------------------------------------------------------

    class Acd1ColumnBases : public ColumnBases
    {
      public:
        Acd1ColumnBases(const rjson::v1::array& data) : data_{data} {}
        Acd1ColumnBases(const rjson::v1::array& data, MinimumColumnBasis minimum_column_basis) : data_{data}, minimum_column_basis_{minimum_column_basis} {}

        double column_basis(size_t aSerumNo) const override { return minimum_column_basis_.apply(data_[aSerumNo]); }
        size_t size() const override { return data_.size(); }

     private:
        const rjson::v1::array& data_;
        MinimumColumnBasis minimum_column_basis_;

    }; // class Acd1ColumnBases

// ----------------------------------------------------------------------

    class Acd1Projection : public RjsonProjection
    {
      public:
        Acd1Projection(const Chart& chart, const rjson::v1::object& aData) : RjsonProjection(chart, aData, s_keys_) {}
        Acd1Projection(const Chart& chart, const rjson::v1::object& aData, size_t projection_no) : RjsonProjection(chart, aData, s_keys_, projection_no) {}

        // std::optional<double> stored_stress() const override { return mData.get<double>("stress"); }
        // std::shared_ptr<Layout> layout() const override;
        // std::string comment() const override;
        // size_t number_of_points() const override { return mData.get_or_empty_array("layout").size(); }
        // size_t number_of_dimensions() const override;
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        bool dodgy_titer_is_regular() const override { return data().get_or_empty_object("stress_evaluator_parameters").get_or_default("dodgy_titer_is_regular", false); }
        double stress_diff_to_stop() const override { return data().get_or_empty_object("stress_evaluator_parameters").get_or_default("stress_diff_to_stop", 0.0); }
        PointIndexList unmovable() const override { return make_attributes(1); }
        PointIndexList unmovable_in_the_last_dimension() const override  { return make_attributes(3); }
        AvidityAdjusts avidity_adjusts() const override;

        MinimumColumnBasis minimum_column_basis() const override
            {
                try {
                    return data().get_or_empty_object("stress_evaluator_parameters")["minimum_column_basis"].strv();
                }
                catch (rjson::v1::field_not_found&) {
                    if (const auto mcb = dynamic_cast<Acd1Titers&>(*chart().titers()).minimum_column_basis(); mcb)
                        return *mcb;
                    else
                        return std::string{"none"};
                }
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
        Acd1Projections(const Chart& chart, const rjson::v1::array& aData) : Projections(chart), mData{aData}, projections_(aData.size(), nullptr) {}

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<Acd1Projection>(chart(), mData[aIndex], aIndex);
                return projections_[aIndex];
            }

     private:
        const rjson::v1::array& mData;
        mutable std::vector<ProjectionP> projections_;

    }; // class Acd1Projections

// ----------------------------------------------------------------------

    class Acd1PlotSpec : public PlotSpec
    {
      public:
        Acd1PlotSpec(const rjson::v1::object& aData, const Acd1Chart& aChart) : mData{aData}, mChart{aChart} {}

        bool empty() const override { return mData.empty(); }
        DrawingOrder drawing_order() const override;
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;
        size_t number_of_points() const override;

     private:
        const rjson::v1::object& mData;
        const Acd1Chart& mChart;

        PointStyle extract(const rjson::v1::object& aSrc, size_t aPointNo, size_t aStyleNo) const;

    }; // class Acd1PlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
