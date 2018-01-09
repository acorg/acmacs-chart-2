#pragma once

#include "acmacs-base/rjson.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/verify.hh"

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
        inline Acd1Chart(rjson::value&& aSrc) : mData{std::move(aSrc)} {}

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
        inline Acd1Info(const rjson::value& aData) : mData{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        inline std::string virus(Compute aCompute = Compute::No) const override { return make_field("virus", "+", aCompute); }
        inline std::string virus_type(Compute aCompute = Compute::Yes) const override { return make_field("virus_type", "+", aCompute); }
        inline std::string subset(Compute aCompute = Compute::No) const override { return make_field("virus_subset", "+", aCompute); }
        inline std::string assay(Compute aCompute = Compute::No) const override { return make_field("assay", "+", aCompute); }
        inline std::string lab(Compute aCompute = Compute::No) const override { return make_field("lab", "+", aCompute); }
        inline std::string rbc_species(Compute aCompute = Compute::No) const override { return make_field("rbc_species", "+", aCompute); }
        std::string date(Compute aCompute = Compute::No) const override;
        inline size_t number_of_sources() const override { return mData.get_or_empty_array("sources").size(); }
        inline InfoP source(size_t aSourceNo) const override { return std::make_shared<Acd1Info>(mData.get_or_empty_array("sources")[aSourceNo]); }

     private:
        const rjson::value& mData;

        std::string make_field(const char* aField, const char* aSeparator, Compute aCompute) const;

    }; // class Acd1Info

// ----------------------------------------------------------------------

    class Acd1Antigen : public Antigen
    {
      public:
        inline Acd1Antigen(const rjson::object& aData) : mData{aData} {}

        Name name() const override;
        inline Date date() const override { return mData.get_or_default("date", ""); }
        Passage passage() const override;
        BLineage lineage() const override;
        Reassortant reassortant() const override;
        LabIds lab_ids() const override;
        inline Clades clades() const override { return {}; /* not implemented */ }
        Annotations annotations() const override;
        inline bool reference() const override { return mData.get_or_default("reference", false); }

     private:
        const rjson::object& mData;

    }; // class Acd1Antigen

// ----------------------------------------------------------------------

    class Acd1Serum : public Serum
    {
      public:
        inline Acd1Serum(const rjson::object& aData) : mData{aData} {}

        Name name() const override;
        Passage passage() const override;
        BLineage lineage() const override;
        Reassortant reassortant() const override;
        Annotations annotations() const override;
        SerumId serum_id() const override;
        inline SerumSpecies serum_species() const override { return mData.get_or_default("serum_species", ""); }
        inline PointIndexList homologous_antigens() const override { return mData.get_or_empty_array("*homologous"); }
        inline void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::object&>(mData).set_field("*homologous", rjson::array(rjson::array::use_iterator, ags.begin(), ags.end())); }

     private:
        const rjson::object& mData;
        // PointIndexList mHomologous;

    }; // class Acd1Serum

// ----------------------------------------------------------------------

    class Acd1Antigens : public Antigens
    {
     public:
        inline Acd1Antigens(const rjson::array& aData, acd1::name_index_t& aAntigenNameIndex) : mData{aData}, mAntigenNameIndex{aAntigenNameIndex} {}

        inline size_t size() const override { return mData.size(); }
        inline AntigenP operator[](size_t aIndex) const override { return std::make_shared<Acd1Antigen>(mData[aIndex]); }
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
        inline Acd1Sera(const rjson::array& aData) : mData{aData} {}

        inline size_t size() const override { return mData.size(); }
        inline SerumP operator[](size_t aIndex) const override { return std::make_shared<Acd1Serum>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class Acd1Sera

// ----------------------------------------------------------------------

    class Acd1Titers : public Titers
    {
      public:
        inline Acd1Titers(const rjson::object& aData) : mData{aData} {}

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override;
        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const override;
        inline size_t number_of_layers() const override { return mData.get_or_empty_array("layers_dict_for_antigen").size(); }
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override;
        size_t number_of_non_dont_cares() const override;

          // support for fast exporting into ace, if source was ace or acd1
        inline const rjson::array& rjson_list_list() const override { const rjson::array& r = mData.get_or_empty_array("titers_list_of_list"); if (r.empty()) throw data_not_available{"no titers_list_of_list"}; return r; }
        inline const rjson::array& rjson_list_dict() const override { const rjson::array& r =  mData.get_or_empty_array("titers_list_of_dict"); if (r.empty()) throw data_not_available{"no titers_list_of_dict"}; return r; }
        inline const rjson::array& rjson_layers() const override {const rjson::array& r = mData.get_or_empty_array("layers_dict_for_antigen"); if (r.empty()) throw data_not_available{"no layers_dict_for_antigen"}; return r; }

        void update(TableDistances<float>& table_distances, const ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const override;
        void update(TableDistances<double>& table_distances, const ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const override;

     private:
        const rjson::object& mData;

        inline const rjson::object& layer(size_t aLayerNo) const { return rjson_layers()[aLayerNo]; }

    }; // class Acd1Titers

// ----------------------------------------------------------------------

    class Acd1ColumnBases : public ColumnBases
    {
      public:
        inline Acd1ColumnBases(const rjson::array& aData) : mData{aData} {}

        inline double column_basis(size_t aSerumNo) const override { return mData[aSerumNo]; }
        inline size_t size() const override { return mData.size(); }

     private:
        const rjson::array& mData;

    }; // class Acd1ColumnBases

// ----------------------------------------------------------------------

    class Acd1Projection : public Projection
    {
      public:
        inline Acd1Projection(const Chart& chart, const rjson::object& aData) : Projection(chart), mData{aData} {}

        inline std::optional<double> stored_stress() const override { return mData.get<double>("stress"); }
        std::shared_ptr<Layout> layout() const override;
        std::string comment() const override;
        inline size_t number_of_points() const override { return mData.get_or_empty_array("layout").size(); }
        size_t number_of_dimensions() const override;
        inline MinimumColumnBasis minimum_column_basis() const override { return mData.get_or_empty_object("stress_evaluator_parameters").get_or_default("minimum_column_basis", "none"); }
        ColumnBasesP forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        inline bool dodgy_titer_is_regular() const override { return mData.get_or_empty_object("stress_evaluator_parameters").get_or_default("dodgy_titer_is_regular", false); }
        inline double stress_diff_to_stop() const override { return mData.get_or_empty_object("stress_evaluator_parameters").get_or_default("stress_diff_to_stop", 0.0); }
        PointIndexList unmovable() const override;
        PointIndexList disconnected() const override;
        PointIndexList unmovable_in_the_last_dimension() const override;
        AvidityAdjusts avidity_adjusts() const override;

     private:
        const rjson::object& mData;
        mutable std::shared_ptr<Layout> layout_;

    }; // class Acd1Projections

// ----------------------------------------------------------------------

    class Acd1Projections : public Projections
    {
      public:
        inline Acd1Projections(const Chart& chart, const rjson::array& aData) : Projections(chart), mData{aData}, projections_(aData.size(), nullptr) {}

        inline bool empty() const override { return projections_.empty(); }
        inline size_t size() const override { return projections_.size(); }
        inline ProjectionP operator[](size_t aIndex) const override
            {
                if (!projections_[aIndex])
                    projections_[aIndex] = std::make_shared<Acd1Projection>(chart(), mData[aIndex]);
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
        inline Acd1PlotSpec(const rjson::object& aData, const Acd1Chart& aChart) : mData{aData}, mChart{aChart} {}

        inline bool empty() const override { return mData.empty(); }
        DrawingOrder drawing_order() const override;
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;

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
