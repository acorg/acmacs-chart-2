#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class InfoModify;
    class AntigensModify;
    class SeraModify;
    class AntigenModify;
    class SerumModify;
    class TitersModify;
    class ColumnBasesModify;
    class ProjectionModify;
    class ProjectionsModify;
    class PlotSpecModify;

    using InfoModifyP = std::shared_ptr<InfoModify>;
    using AntigensModifyP = std::shared_ptr<AntigensModify>;
    using SeraModifyP = std::shared_ptr<SeraModify>;
    using AntigenModifyP = std::shared_ptr<AntigenModify>;
    using SerumModifyP = std::shared_ptr<SerumModify>;
    using TitersModifyP = std::shared_ptr<TitersModify>;
    using ColumnBasesModifyP = std::shared_ptr<ColumnBasesModify>;
    using ProjectionsModifyP = std::shared_ptr<ProjectionsModify>;
    using ProjectionModifyP = std::shared_ptr<ProjectionModify>;
    using PlotSpecModifyP = std::shared_ptr<PlotSpecModify>;

    namespace internal
    {
        class ProjectionModifyData;

    } // namespace internal

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

        InfoModifyP info_modify();
        AntigensModifyP antigens_modify();
        SeraModifyP sera_modify();
        TitersModifyP titers_modify();
        ColumnBasesModifyP forced_column_bases_modify();
        ProjectionsModifyP projections_modify();
        ProjectionModifyP projection_modify(size_t aProjectionNo);
        PlotSpecModifyP plot_spec_modify();

     private:
        ChartP mMain;
        std::unique_ptr<internal::ProjectionModifyData> mProjectionModifyData;

        friend class ProjectionModify;

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

        inline Name name() const override  { return mMain->name(); }
        inline Date date() const override  { return mMain->date(); }
        inline Passage passage() const override { return mMain->passage(); }
        inline BLineage lineage() const override  { return mMain->lineage(); }
        inline Reassortant reassortant() const override { return mMain->reassortant(); }
        inline LabIds lab_ids() const override { return mMain->lab_ids(); }
        inline Clades clades() const override { return mMain->clades(); }
        inline Annotations annotations() const override { return mMain->annotations(); }
        inline bool reference() const override { return mMain->reference(); }

     private:
        AntigenP mMain;

    }; // class AntigenModify

// ----------------------------------------------------------------------

    class SerumModify : public Serum
    {
     public:
        inline SerumModify(SerumP aMain) : mMain{aMain} {}

        inline Name name() const override { return mMain->name(); }
        inline Passage passage() const override { return mMain->passage(); }
        inline BLineage lineage() const override { return mMain->lineage(); }
        inline Reassortant reassortant() const override { return mMain->reassortant(); }
        inline Annotations annotations() const override { return mMain->annotations(); }
        inline SerumId serum_id() const override { return mMain->serum_id(); }
        inline SerumSpecies serum_species() const override { return mMain->serum_species(); }
        inline PointIndexList homologous_antigens() const override { return mMain->homologous_antigens(); }
        inline void set_homologous(const std::vector<size_t>& ags) const override { return mMain->set_homologous(ags); }

     private:
        SerumP mMain;

    }; // class SerumModify

// ----------------------------------------------------------------------

    class AntigensModify : public Antigens
    {
     public:
        inline AntigensModify(AntigensP aMain) : mMain{aMain} {}

        inline size_t size() const override { return mMain->size(); }
        inline AntigenP operator[](size_t aIndex) const override { return std::make_shared<AntigenModify>(mMain->operator[](aIndex)); }
        inline std::optional<size_t> find_by_full_name(std::string aFullName) const override { return mMain->find_by_full_name(aFullName); }

     private:
        AntigensP mMain;

    }; // class AntigensModify

// ----------------------------------------------------------------------

    class SeraModify : public Sera
    {
     public:
        inline SeraModify(SeraP aMain) : mMain{aMain} {}

        inline size_t size() const override { return mMain->size(); }
        inline SerumP operator[](size_t aIndex) const override { return std::make_shared<SerumModify>(mMain->operator[](aIndex)); }

     private:
        SeraP mMain;

    }; // class SeraModify

// ----------------------------------------------------------------------

    class TitersModify : public Titers
    {
     public:
        inline TitersModify(TitersP aMain) : mMain{aMain} {}

        inline Titer titer(size_t aAntigenNo, size_t aSerumNo) const override { return mMain->titer(aAntigenNo, aSerumNo); }
        inline Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override { return mMain->titer_of_layer(aLayerNo, aAntigenNo, aSerumNo); }
        inline size_t number_of_layers() const override { return mMain->number_of_layers(); }
        inline size_t number_of_antigens() const override { return mMain->number_of_antigens(); }
        inline size_t number_of_sera() const override { return mMain->number_of_sera(); }
        inline size_t number_of_non_dont_cares() const override { return mMain->number_of_non_dont_cares(); }

          // support for fast exporting into ace, if source was ace or acd1
        inline const rjson::array& rjson_list_list() const override { return mMain->rjson_list_list(); }
        inline const rjson::array& rjson_list_dict() const override { return mMain->rjson_list_dict(); }
        inline const rjson::array& rjson_layers() const override { return mMain->rjson_layers(); }

     private:
        TitersP mMain;

    }; // class TitersModify

// ----------------------------------------------------------------------

    class ColumnBasesModify : public ColumnBases
    {
     public:
        inline ColumnBasesModify(ColumnBasesP aMain) : mMain{aMain} {}

        inline bool exists() const override { return mMain->exists(); }
        inline double column_basis(size_t aSerumNo) const override { return mMain->column_basis(aSerumNo); }
        inline size_t size() const override { return mMain->size(); }

     private:
        ColumnBasesP mMain;

    }; // class ColumnBasesModify

// ----------------------------------------------------------------------

    class ProjectionModify : public Projection
    {
     public:
        inline ProjectionModify(ProjectionP aMain, ChartModify& aChart) : mMain{aMain}, mChart(aChart) {}

        inline double stress() const override { return mMain->stress(); }
        inline std::shared_ptr<Layout> layout() const override { return mMain->layout(); }
        inline std::string comment() const override { return mMain->comment(); }
        inline MinimumColumnBasis minimum_column_basis() const override { return mMain->minimum_column_basis(); }
        inline ColumnBasesP forced_column_bases() const override { return mMain->forced_column_bases(); }
        inline acmacs::Transformation transformation() const override { return mMain->transformation(); }
        inline bool dodgy_titer_is_regular() const override { return mMain->dodgy_titer_is_regular(); }
        inline double stress_diff_to_stop() const override { return mMain->stress_diff_to_stop(); }
        inline PointIndexList unmovable() const override { return mMain->unmovable(); }
        inline PointIndexList disconnected() const override { return mMain->disconnected(); }
        inline PointIndexList unmovable_in_the_last_dimension() const override { return mMain->unmovable_in_the_last_dimension(); }
        inline AvidityAdjusts avidity_adjusts() const override { return mMain->avidity_adjusts(); }

     private:
        ProjectionP mMain;
        ChartModify& mChart;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class ProjectionsModify : public Projections
    {
     public:
        inline ProjectionsModify(ProjectionsP aMain, ChartModify& aChart) : mMain{aMain}, mChart(aChart) {}

        inline bool empty() const override { return mMain->empty(); }
        inline size_t size() const override { return mMain->size(); }
        inline ProjectionP operator[](size_t aIndex) const override { return std::make_shared<ProjectionModify>(mMain->operator[](aIndex), mChart); }

     private:
        ProjectionsP mMain;
        ChartModify& mChart;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class PlotSpecModify : public PlotSpec
    {
     public:
        inline PlotSpecModify(PlotSpecP aMain) : mMain{aMain} {}

        inline bool empty() const override { return mMain->empty(); }
        inline DrawingOrder drawing_order() const override { return mMain->drawing_order(); }
        inline Color error_line_positive_color() const override { return mMain->error_line_positive_color(); }
        inline Color error_line_negative_color() const override { return mMain->error_line_negative_color(); }
        inline PointStyle style(size_t aPointNo) const override { return mMain->style(aPointNo); }
        inline std::vector<PointStyle> all_styles() const override { return mMain->all_styles(); }

     private:
        PlotSpecP mMain;

    }; // class PlotSpecModify

// ----------------------------------------------------------------------

    namespace internal
    {
        class ProjectionModifyData
        {
        }; // class ProjectionModifyData

    } // namespace internal


} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
