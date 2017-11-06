#pragma once

#include "acmacs-base/rjson.hh"
#include "acmacs-chart/chart.hh"
#include "acmacs-chart/verify.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class AceChart : public Chart
    {
      public:
        inline AceChart(rjson::value&& aSrc) : mData{std::move(aSrc)} {}

        std::shared_ptr<Info> info() const override;
        std::shared_ptr<Antigens> antigens() const override;
        std::shared_ptr<Sera> sera() const override;
        std::shared_ptr<Titers> titers() const override;
        std::shared_ptr<ForcedColumnBases> forced_column_bases() const override;
        std::shared_ptr<Projections> projections() const override;
        std::shared_ptr<PlotSpec> plot_spec() const override;

        void verify_data(Verify aVerify) const;

     private:
        rjson::value mData;

    }; // class Chart

    inline bool is_ace(const std::string_view& aData)
    {
        return aData.size() > 35 && aData.front() == '{' && aData.find("\"acmacs-ace-v1\"") != std::string_view::npos;
    }

    std::shared_ptr<Chart> ace_import(const std::string_view& aData, Verify aVerify);

// ----------------------------------------------------------------------

    class AceInfo : public Info
    {
      public:
        inline AceInfo(const rjson::value& aData) : mData{aData} {}

        std::string make_info() const override;

        inline std::string name(Compute aCompute = Compute::No) const override { return make_field("N", " + ", aCompute); }
        inline std::string virus(Compute aCompute = Compute::No) const override { return make_field("v", "+", aCompute); }
        inline std::string virus_type(Compute aCompute = Compute::No) const override { return make_field("V", "+", aCompute); }
        inline std::string subset(Compute aCompute = Compute::No) const override { return make_field("s", "+", aCompute); }
        inline std::string assay(Compute aCompute = Compute::No) const override { return make_field("A", "+", aCompute); }
        inline std::string lab(Compute aCompute = Compute::No) const override { return make_field("l", "+", aCompute); }
        inline std::string rbc_species(Compute aCompute = Compute::No) const override { return make_field("r", "+", aCompute); }
        inline std::string date(Compute aCompute = Compute::No) const override;
        inline size_t number_of_sources() const override { return mData.get_or_empty_array("S").size(); }
        inline std::shared_ptr<Info> source(size_t aSourceNo) const override { return std::make_shared<AceInfo>(mData.get_or_empty_array("S")[aSourceNo]); }

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

     private:
        const rjson::object& mData;

    }; // class AceSerum

// ----------------------------------------------------------------------

    class AceAntigens : public Antigens
    {
     public:
        inline AceAntigens(const rjson::array& aData) : mData{aData} {}

        inline size_t size() const override { return mData.size(); }
        inline std::shared_ptr<Antigen> operator[](size_t aIndex) const override { return std::make_shared<AceAntigen>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceAntigens

// ----------------------------------------------------------------------

    class AceSera : public Sera
    {
      public:
        inline AceSera(const rjson::array& aData) : mData{aData} {}

        inline size_t size() const override { return mData.size(); }
        inline std::shared_ptr<Serum> operator[](size_t aIndex) const override { return std::make_shared<AceSerum>(mData[aIndex]); }

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
        inline size_t number_of_layers() const override { return layers().size(); }
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override;
        size_t number_of_non_dont_cares() const override;

     private:
        const rjson::object& mData;

        inline const rjson::array& layers() const { return mData.get_or_empty_array("L"); }
        inline const rjson::object& layer(size_t aLayerNo) const { return layers()[aLayerNo]; }

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

    class AceForcedColumnBases : public ForcedColumnBases
    {
      public:
        inline AceForcedColumnBases(const rjson::array& aData) : mData{aData} {}

        inline bool exists() const override { return !mData.empty(); }
        inline double column_basis(size_t aSerumNo) const override { return mData[aSerumNo]; }
        inline size_t size() const override { return mData.size(); }

     private:
        const rjson::array& mData;

    }; // class AceForcedColumnBases

// ----------------------------------------------------------------------

    class AceProjection : public Projection
    {
      public:
        inline AceProjection(const rjson::object& aData) : mData{aData} {}

        inline double stress() const override { return mData.get_or_default("s", 0.0); }
        size_t number_of_dimensions() const override;
        inline size_t number_of_points() const override { return mData.get_or_empty_array("l").size(); }
        double coordinate(size_t aPointNo, size_t aDimensionNo) const override;
        inline std::string comment() const override { return mData.get_or_default("c", ""); }
        inline MinimumColumnBasis minimum_column_basis() const override { return mData.get_or_default("m", "none"); }
        std::shared_ptr<ForcedColumnBases> forced_column_bases() const override;
        acmacs::Transformation transformation() const override;
        inline bool dodgy_titer_is_regular() const override { return mData.get_or_default("d", false); }
        inline double stress_diff_to_stop() const override { return mData.get_or_default("d", 0.0); }
        inline PointIndexList unmovable() const override { return mData.get_or_empty_array("U"); }
        inline PointIndexList disconnected() const override { return mData.get_or_empty_array("D"); }
        inline PointIndexList unmovable_in_the_last_dimension() const override { return mData.get_or_empty_array("u"); }

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
        inline std::shared_ptr<Projection> operator[](size_t aIndex) const override { return std::make_shared<AceProjection>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AcePlotSpec : public PlotSpec
    {
      public:
        inline AcePlotSpec(const rjson::object& aData) : mData{aData} {}

        inline DrawingOrder drawing_order() const override { return mData["d"]; }
        Color error_line_positive_color() const override;
        Color error_line_negative_color() const override;
        PointStyle style(size_t aPointNo) const override;

     private:
        const rjson::object& mData;

        void label_style(PointStyle& aStyle, const rjson::object& aData) const;

    }; // class AcePlotSpec

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
