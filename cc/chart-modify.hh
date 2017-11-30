#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class ChartModify : public Chart
    {
      public:
        inline ChartModify(ChartP aMain) : mMain{aMain} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases() const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        inline bool is_merge() const override { return mMain->is_merge(); }

     private:
        ChartP mMain;

    }; // class ChartModify

// ----------------------------------------------------------------------

    class InfoModify : public Info
    {
      public:
        inline InfoModify(InfoP aMain) : mMain{aMain} {}

        inline std::string name(Compute aCompute = Compute::No) const override { return mMain->name(aCompute); }
        inline std::string virus(Compute aCompute = Compute::No) const override { return mMain->virus(aCompute); }
        inline std::string virus_type(Compute aCompute = Compute::Yes) const override { return mMain->virus_type(aCompute); }
        inline std::string subset(Compute aCompute = Compute::No) const override { return mMain->subset(aCompute); }
        inline std::string assay(Compute aCompute = Compute::No) const override { return mMain->assay(aCompute); }
        inline std::string lab(Compute aCompute = Compute::No) const override { return mMain->lab(aCompute); }
        inline std::string rbc_species(Compute aCompute = Compute::No) const override { return mMain->rbc_species(aCompute); }
        inline std::string date(Compute aCompute = Compute::No) const override { return mMain->date(aCompute); }
        inline size_t number_of_sources() const override { return mMain->number_of_sources(); }
        inline InfoP source(size_t aSourceNo) const override { return mMain->source(aSourceNo); }

     private:
        InfoP mMain;

    }; // class InfoModify

// ----------------------------------------------------------------------

    class AntigenModify : public Antigen
    {
      public:
        inline AntigenModify(AntigenP aMain) : mMain{aMain} {}

        // inline Name name() const override { return mData["N"]; }
        // inline Date date() const override { return mData.get_or_default("D", ""); }
        // inline Passage passage() const override { return mData.get_or_default("P", ""); }
        // BLineage lineage() const override;
        // inline Reassortant reassortant() const override { return mData.get_or_default("R", ""); }
        // inline LabIds lab_ids() const override { return mData.get_or_empty_array("l"); }
        // inline Clades clades() const override { return mData.get_or_empty_array("c"); }
        // inline Annotations annotations() const override { return mData.get_or_empty_array("a"); }
        // inline bool reference() const override { return mData.get_or_default("S", "").find("R") != std::string::npos; }

     private:
        AntigenP mMain;

    }; // class AntigenModify

// ----------------------------------------------------------------------

    class SerumModify : public Serum
    {
      public:
        inline SerumModify(SerumP aMain) : mMain{aMain} {}

        // inline Name name() const override { return mData["N"]; }
        // inline Passage passage() const override { return mData.get_or_default("P", ""); }
        // BLineage lineage() const override;
        // inline Reassortant reassortant() const override { return mData.get_or_default("R", ""); }
        // inline Annotations annotations() const override { return mData.get_or_empty_array("a"); }
        // inline SerumId serum_id() const override { return mData.get_or_default("I", ""); }
        // inline SerumSpecies serum_species() const override { return mData.get_or_default("s", ""); }
        // inline PointIndexList homologous_antigens() const override { return mData.get_or_empty_array("h"); }
        // inline void set_homologous(const std::vector<size_t>& ags) const override { const_cast<rjson::object&>(mData).set_field("h", rjson::array(rjson::array::use_iterator, ags.begin(), ags.end())); }

     private:
        SerumP mMain;

    }; // class SerumModify

// ----------------------------------------------------------------------

    class AntigensModify : public Antigens
    {
     public:
        inline AntigensModify(AntigensP aMain) : mMain{aMain} {}

        // inline size_t size() const override { return mData.size(); }
        // inline AntigenP operator[](size_t aIndex) const override { return std::make_shared<AntigenModify>(mData[aIndex]); }
        // std::optional<size_t> find_by_full_name(std::string aFullName) const override;

     private:
        AntigensP mMain;

    }; // class AntigensModify

// ----------------------------------------------------------------------

    class SeraModify : public Sera
    {
      public:
        inline SeraModify(SeraP aMain) : mMain{aMain} {}

        // inline size_t size() const override { return mData.size(); }
        // inline SerumP operator[](size_t aIndex) const override { return std::make_shared<SerumModify>(mData[aIndex]); }

     private:
        SeraP mMain;

    }; // class SeraModify

// ----------------------------------------------------------------------

    class TitersModify : public Titers
    {
      public:
        inline TitersModify(TitersP aMain) : mMain{aMain} {}

        // Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        // Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override;
        // inline size_t number_of_layers() const override { return mData.get_or_empty_array("L").size(); }
        // size_t number_of_antigens() const override;
        // size_t number_of_sera() const override;
        // size_t number_of_non_dont_cares() const override;

          // support for fast exporting into ace, if source was ace or acd1
        // inline const rjson::array& rjson_list_list() const override { const rjson::array& r = mData.get_or_empty_array("l"); if (r.empty()) throw data_not_available{"no \"l\""}; return r; }
        // inline const rjson::array& rjson_list_dict() const override { const rjson::array& r = mData.get_or_empty_array("d"); if (r.empty()) throw data_not_available{"no \"d\""}; return r; }
        // inline const rjson::array& rjson_layers() const override { const rjson::array& r = mData.get_or_empty_array("L"); if (r.empty()) throw data_not_available{"no \"L\""}; return r; }

     private:
        TitersP mMain;

    }; // class TitersModify

// ----------------------------------------------------------------------

    class ColumnBasesModify : public ColumnBases
    {
      public:
        inline ColumnBasesModify(ColumnBasesP aMain) : mMain{aMain} {}

        // inline bool exists() const override { return !mData.empty(); }
        // inline double column_basis(size_t aSerumNo) const override { return mData[aSerumNo]; }
        // inline size_t size() const override { return mData.size(); }

     private:
        ColumnBasesP mMain;

    }; // class ColumnBasesModify

// ----------------------------------------------------------------------

    class ProjectionModify : public Projection
    {
      public:
        inline ProjectionModify(ProjectionP aMain) : mMain{aMain} {}

        // inline double stress() const override { return mData.get_or_default("s", 0.0); }
        // std::shared_ptr<Layout> layout() const override;
        // inline std::string comment() const override { return mData.get_or_default("c", ""); }
        // inline MinimumColumnBasis minimum_column_basis() const override { return mData.get_or_default("m", "none"); }
        // ColumnBasesP forced_column_bases() const override;
        // acmacs::Transformation transformation() const override;
        // inline bool dodgy_titer_is_regular() const override { return mData.get_or_default("d", false); }
        // inline double stress_diff_to_stop() const override { return mData.get_or_default("d", 0.0); }
        // inline PointIndexList unmovable() const override { return mData.get_or_empty_array("U"); }
        // inline PointIndexList disconnected() const override { return mData.get_or_empty_array("D"); }
        // inline PointIndexList unmovable_in_the_last_dimension() const override { return mData.get_or_empty_array("u"); }
        // inline AvidityAdjusts avidity_adjusts() const override { return mData.get_or_empty_array("f"); }

     private:
        ProjectionP mMain;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class ProjectionsModify : public Projections
    {
      public:
        inline ProjectionsModify(ProjectionsP aMain) : mMain{aMain} {}

        // inline bool empty() const override { return mData.empty(); }
        // inline size_t size() const override { return mData.size(); }
        // inline ProjectionP operator[](size_t aIndex) const override { return std::make_shared<ProjectionModify>(mData[aIndex]); }

     private:
        ProjectionsP mMain;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class PlotSpecModify : public PlotSpec
    {
      public:
        inline PlotSpecModify(PlotSpecP aMain) : mMain{aMain} {}

        // inline bool empty() const override { return mData.empty(); }
        // inline DrawingOrder drawing_order() const override { return mData.get_or_empty_array("d"); }
        // Color error_line_positive_color() const override;
        // Color error_line_negative_color() const override;
        // PointStyle style(size_t aPointNo) const override;
        // std::vector<PointStyle> all_styles() const override;

     private:
        PlotSpecP mMain;

    }; // class PlotSpecModify

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
