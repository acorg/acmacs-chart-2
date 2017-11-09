#pragma once

#include "acmacs-chart/chart.hh"
#include "acmacs-chart/verify.hh"
#include "acmacs-chart/lispmds-token.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class LispmdsChart : public Chart
    {
      public:
        inline LispmdsChart(acmacs::lispmds::value&& aSrc) : mData{std::move(aSrc)} {}

        std::shared_ptr<Info> info() const override;
        std::shared_ptr<Antigens> antigens() const override;
        std::shared_ptr<Sera> sera() const override;
        std::shared_ptr<Titers> titers() const override;
        std::shared_ptr<ForcedColumnBases> forced_column_bases() const override;
        std::shared_ptr<Projections> projections() const override;
        std::shared_ptr<PlotSpec> plot_spec() const override;

        void verify_data(Verify aVerify) const;

     private:
        acmacs::lispmds::value mData;

    }; // class Chart

    inline bool is_lispmds(const std::string_view& aData)
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

    std::shared_ptr<Chart> lispmds_import(const std::string_view& aData, Verify aVerify);

// ----------------------------------------------------------------------

    class LispmdsInfo : public Info
    {
      public:
        inline LispmdsInfo(const acmacs::lispmds::value& aData) : mData{aData} {}

        std::string name(Compute aCompute = Compute::No) const override;
        inline std::string virus(Compute = Compute::No) const override { return {}; }
        inline std::string virus_type(Compute = Compute::No) const override { return {}; }
        inline std::string subset(Compute = Compute::No) const override { return {}; }
        inline std::string assay(Compute = Compute::No) const override { return {}; }
        inline std::string lab(Compute = Compute::No) const override { return {}; }
        inline std::string rbc_species(Compute = Compute::No) const override { return{}; }
        inline std::string date(Compute = Compute::No) const override { return {}; }
        inline size_t number_of_sources() const override { return 0; }
        inline std::shared_ptr<Info> source(size_t) const override { return nullptr; }

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsInfo

// ----------------------------------------------------------------------

    class LispmdsAntigen : public Antigen
    {
      public:
        inline LispmdsAntigen(const acmacs::lispmds::value& aData) : mData{aData} {}

        Name name() const override;
        inline Date date() const override { return {}; }
        inline Passage passage() const override { return {}; }
        inline BLineage lineage() const override { return {}; }
        inline Reassortant reassortant() const override { return {}; }
        inline LabIds lab_ids() const override { return {}; }
        inline Clades clades() const override { return {}; }
        inline Annotations annotations() const override { return {}; }
        bool reference() const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsAntigen

// ----------------------------------------------------------------------

    class LispmdsSerum : public Serum
    {
      public:
        inline LispmdsSerum(const acmacs::lispmds::value& aData) : mData{aData} {}

        Name name() const override;
        inline Passage passage() const override { return {}; }
        inline BLineage lineage() const override { return {}; }
        inline Reassortant reassortant() const override { return {}; }
        inline Annotations annotations() const override { return {}; }
        inline SerumId serum_id() const override { return {}; }
        inline SerumSpecies serum_species() const override { return {}; }
        inline PointIndexList homologous_antigens() const override { return {}; }

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsSerum

// ----------------------------------------------------------------------

    class LispmdsAntigens : public Antigens
    {
     public:
        inline LispmdsAntigens(const acmacs::lispmds::value& aData) : mData{aData} {}

        size_t size() const override;
        std::shared_ptr<Antigen> operator[](size_t aIndex) const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsAntigens

// ----------------------------------------------------------------------

    class LispmdsSera : public Sera
    {
      public:
        inline LispmdsSera(const acmacs::lispmds::value& aData) : mData{aData} {}

        inline size_t size() const override;
        inline std::shared_ptr<Serum> operator[](size_t aIndex) const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsSera

// ----------------------------------------------------------------------

    class LispmdsTiters : public Titers
    {
      public:
        inline LispmdsTiters(const acmacs::lispmds::value& aData) : mData{aData} {}

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        inline Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override { throw std::runtime_error("lispmds importer does not support layers"); }
        inline size_t number_of_layers() const override { return 0; }
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override;
        size_t number_of_non_dont_cares() const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsTiters

// ----------------------------------------------------------------------

    class LispmdsForcedColumnBases : public ForcedColumnBases
    {
      public:
        inline LispmdsForcedColumnBases(const acmacs::lispmds::value& aData) : mData{aData} {}

        inline bool exists() const override { return true; }
        double column_basis(size_t aSerumNo) const override;
        size_t size() const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsForcedColumnBases

// ----------------------------------------------------------------------

    class LispmdsProjection : public Projection
    {
      public:
        inline LispmdsProjection(const acmacs::lispmds::value& aData) : mData{aData} {}

        double stress() const override;
        size_t number_of_dimensions() const override;
        size_t number_of_points() const override;
        double coordinate(size_t aPointNo, size_t aDimensionNo) const override;
        inline std::string comment() const override { return {}; }
        inline MinimumColumnBasis minimum_column_basis() const override { return {}; }
        std::shared_ptr<ForcedColumnBases> forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        inline bool dodgy_titer_is_regular() const override { return false; }
        inline double stress_diff_to_stop() const override { return 0.0; }
        PointIndexList unmovable() const override;
        PointIndexList disconnected() const override;
        inline PointIndexList unmovable_in_the_last_dimension() const override { return {}; }

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsProjections

// ----------------------------------------------------------------------

    class LispmdsProjections : public Projections
    {
      public:
        inline LispmdsProjections(const acmacs::lispmds::value& aData) : mData{aData} {}

        bool empty() const override;
        size_t size() const override;
        std::shared_ptr<Projection> operator[](size_t aIndex) const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsProjections

// ----------------------------------------------------------------------

    class LispmdsPlotSpec : public PlotSpec
    {
      public:
        inline LispmdsPlotSpec(const acmacs::lispmds::value& aData) : mData{aData} {}

        bool empty() const override;
        DrawingOrder drawing_order() const override;
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;
        std::vector<PointStyle> all_styles() const override;

     private:
        const acmacs::lispmds::value& mData;

    }; // class LispmdsPlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: