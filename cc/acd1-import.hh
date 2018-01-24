#pragma once

#include "acmacs-base/rjson.hh"
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
        Acd1Chart(rjson::value&& aSrc) : mData{std::move(aSrc)} {}

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
        Acd1Info(const rjson::value& aData) : mData{aData} {}

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
        const rjson::value& mData;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class Acd1Info

// ----------------------------------------------------------------------

    class Acd1Antigen : public Antigen
    {
      public:
        Acd1Antigen(const rjson::object& aData) : mData{aData} {}

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
        const rjson::object& mData;

    }; // class Acd1Antigen

// ----------------------------------------------------------------------

    class Acd1Serum : public Serum
    {
      public:
        Acd1Serum(const rjson::object& aData) : mData{aData} {}

        Name name() const override;
        Passage passage() const override;
        BLineage lineage() const override;
        Reassortant reassortant() const override;
        Annotations annotations() const override;
        SerumId serum_id() const override;
        SerumSpecies serum_species() const override { return mData.get_or_default("serum_species", ""); }
        PointIndexList homologous_antigens() const override { return mData.get_or_empty_array("*homologous"); }
        void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::object&>(mData).set_field("*homologous", rjson::array(rjson::array::use_iterator, ags.begin(), ags.end())); }

     private:
        const rjson::object& mData;
        // PointIndexList mHomologous;

    }; // class Acd1Serum

// ----------------------------------------------------------------------

    class Acd1Antigens : public Antigens
    {
     public:
        Acd1Antigens(const rjson::array& aData, acd1::name_index_t& aAntigenNameIndex) : mData{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        size_t size() const override { return mData.size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<Acd1Antigen>(mData[aIndex]); }
        std::optional<size_t> find_by_full_name(std::string aFullName) const override;

     private:
        const rjson::array& mData;
        acd1::name_index_t& mAntigenNameIndex;

        void make_name_index() const;

    }; // class Acd1Antigens

// ----------------------------------------------------------------------

    class Acd1Sera : public Sera
    {
      public:
        Acd1Sera(const rjson::array& aData) : mData{aData} {}

        size_t size() const override { return mData.size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<Acd1Serum>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class Acd1Sera

// ----------------------------------------------------------------------

    class Acd1Titers : public RjsonTiters
    {
      public:
        Acd1Titers(const rjson::object& data) : RjsonTiters(data, s_keys_) {}

          // old acd1 files have minimum_column_basis inside titers instead of projection
        std::optional<MinimumColumnBasis> minimum_column_basis() const
            {
                try {
                    return data()["minimum_column_basis"].str();
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
        Acd1ColumnBases(const rjson::array& aData) : mData{aData} {}

        double column_basis(size_t aSerumNo) const override { return mData[aSerumNo]; }
        size_t size() const override { return mData.size(); }

     private:
        const rjson::array& mData;

    }; // class Acd1ColumnBases

// ----------------------------------------------------------------------

    class Acd1Projection : public Projection
    {
      public:
        Acd1Projection(const Chart& chart, const rjson::object& aData) : Projection(chart), mData{aData} {}
        Acd1Projection(const Chart& chart, const rjson::object& aData, size_t projection_no) : Acd1Projection(chart, aData) { set_projection_no(projection_no); }

        std::optional<double> stored_stress() const override { return mData.get<double>("stress"); }
        std::shared_ptr<Layout> layout() const override;
        std::string comment() const override;
        size_t number_of_points() const override { return mData.get_or_empty_array("layout").size(); }
        size_t number_of_dimensions() const override;
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        bool dodgy_titer_is_regular() const override { return mData.get_or_empty_object("stress_evaluator_parameters").get_or_default("dodgy_titer_is_regular", false); }
        double stress_diff_to_stop() const override { return mData.get_or_empty_object("stress_evaluator_parameters").get_or_default("stress_diff_to_stop", 0.0); }
        PointIndexList unmovable() const override;
        PointIndexList disconnected() const override;
        PointIndexList unmovable_in_the_last_dimension() const override;
        AvidityAdjusts avidity_adjusts() const override;

        MinimumColumnBasis minimum_column_basis() const override
            {
                  //return mData.get_or_empty_object("stress_evaluator_parameters").get_or_default("minimum_column_basis", "none");
                try {
                    return mData.get_or_empty_object("stress_evaluator_parameters")["minimum_column_basis"].str();
                }
                catch (rjson::field_not_found&) {
                    if (const auto mcb = dynamic_cast<Acd1Titers&>(*chart().titers()).minimum_column_basis(); mcb)
                        return *mcb;
                    else
                        return std::string{"none"};
                }
            }

     private:
        const rjson::object& mData;
        mutable std::shared_ptr<Layout> layout_;

    }; // class Acd1Projections

// ----------------------------------------------------------------------

    class Acd1Projections : public Projections
    {
      public:
        Acd1Projections(const Chart& chart, const rjson::array& aData) : Projections(chart), mData{aData}, projections_(aData.size(), nullptr) {}

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<Acd1Projection>(chart(), mData[aIndex], aIndex);
                return projections_[aIndex];
            }

     private:
        const rjson::array& mData;
        mutable std::vector<ProjectionP> projections_;

    }; // class Acd1Projections

// ----------------------------------------------------------------------

    class Acd1PlotSpec : public PlotSpec
    {
      public:
        Acd1PlotSpec(const rjson::object& aData, const Acd1Chart& aChart) : mData{aData}, mChart{aChart} {}

        bool empty() const override { return mData.empty(); }
        DrawingOrder drawing_order() const override;
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;
        size_t number_of_points() const override;

     private:
        const rjson::object& mData;
        const Acd1Chart& mChart;

        PointStyle extract(const rjson::object& aSrc, size_t aPointNo, size_t aStyleNo) const;

    }; // class Acd1PlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
