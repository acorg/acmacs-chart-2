#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class ChartModify;
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

    using ChartModifyP = std::shared_ptr<ChartModify>;
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

// ----------------------------------------------------------------------

    namespace internal
    {
        class Layout : public acmacs::chart::Layout
        {
         public:
            Layout(const acmacs::chart::Layout& aSource);

            inline size_t number_of_points() const noexcept override { return mData.size(); }
            inline size_t number_of_dimensions() const noexcept override { for (const auto& point: mData) { if (point.not_nan()) return point.size(); } return 0; }
            inline acmacs::chart::Coordinates operator[](size_t aPointNo) const override { return mData[aPointNo]; }
            inline double coordinate(size_t aPointNo, size_t aDimensionNo) const override { return mData[aPointNo][aDimensionNo]; }
            void set(size_t aPointNo, const Coordinates& aCoordinates) override;

         private:
            std::vector<acmacs::chart::Coordinates> mData;

        }; // class Layout

        class ProjectionModifyData
        {
         public:
            ProjectionModifyData(ProjectionP aMain);
            inline std::shared_ptr<Layout> layout() { return mLayout; }
            inline std::shared_ptr<acmacs::chart::Layout> transformed_layout() const { if (!mTransformedLayout) mTransformedLayout.reset(mLayout->transform(mTransformation)); return mTransformedLayout; }
            inline const Transformation& transformation() const { return mTransformation; }

            inline void move_point(size_t aPointNo, const std::vector<double>& aCoordinates) { mLayout->set(aPointNo, aCoordinates); mTransformedLayout.reset(); }
            inline void rotate_radians(double aAngle) { mTransformation.rotate(aAngle); mTransformedLayout.reset(); }
            inline void rotate_degrees(double aAngle) { rotate_radians(aAngle * M_PI / 180.0); mTransformedLayout.reset(); }
            inline void flip(double aX, double aY) { mTransformation.flip(aX, aY); mTransformedLayout.reset(); }
            inline void flip_east_west() { flip(0, 1); }
            inline void flip_north_south() { flip(1, 0); }

         private:
            std::shared_ptr<Layout> mLayout;
            Transformation mTransformation;
            mutable std::shared_ptr<acmacs::chart::Layout> mTransformedLayout;

        }; // class ProjectionModifyData

        class PlotSpecModifyData
        {
         public:
            PlotSpecModifyData(PlotSpecP aMain);

            inline const PointStyle& style(size_t aPointNo) const { return mStyles.at(aPointNo); }
            inline const std::vector<PointStyle>& all_styles() const { return mStyles; }

            inline size_t number_of_points() const { return mStyles.size(); }
            inline void validate_point_no(size_t aPointNo) const { if (aPointNo >= number_of_points()) throw std::runtime_error("Invalid point number: " + acmacs::to_string(aPointNo) + ", expected integer in range 0.." + acmacs::to_string(number_of_points() - 1) + ", inclusive"); }
            inline void size(size_t aPointNo, Pixels aSize) { validate_point_no(aPointNo); mStyles[aPointNo].size = aSize; }
            inline void fill(size_t aPointNo, Color aFill) { validate_point_no(aPointNo); mStyles[aPointNo].fill = aFill; }
            inline void outline(size_t aPointNo, Color aOutline) { validate_point_no(aPointNo); mStyles[aPointNo].outline = aOutline; }
            inline void scale_all(double aPointScale, double aOulineScale) { std::for_each(mStyles.begin(), mStyles.end(), [=](auto& style) { style.scale(aPointScale).scale_outline(aOulineScale); }); }

            inline void modify(size_t aPointNo, const PointStyle& aStyle) { mStyles.at(aPointNo) = aStyle; }

            inline const DrawingOrder& drawing_order() const { return mDrawingOrder; }
            inline DrawingOrder& drawing_order() { return mDrawingOrder; }
            inline void raise(size_t aPointNo) { validate_point_no(aPointNo); mDrawingOrder.raise(aPointNo); }
            inline void lower(size_t aPointNo) { validate_point_no(aPointNo); mDrawingOrder.lower(aPointNo); }

         private:
            std::vector<PointStyle> mStyles;
            DrawingOrder mDrawingOrder;

        }; // class PlotSpecModifyData

    } // namespace internal

// ----------------------------------------------------------------------

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
        std::map<size_t, std::unique_ptr<internal::ProjectionModifyData>> mProjectionModifyData; // projection_no to data
        std::optional<internal::PlotSpecModifyData> mPlotSpecModifyData;

        internal::ProjectionModifyData& modify_projection(size_t aProjectionNo);
        inline bool modified_projection(size_t aProjectionNo) const { return mProjectionModifyData.find(aProjectionNo) != mProjectionModifyData.end(); }
        internal::PlotSpecModifyData& modify_plot_spec();
        inline bool modified_plot_spec() const { return mPlotSpecModifyData.has_value(); }

        friend class ProjectionModify;
        friend class PlotSpecModify;

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
        inline ProjectionModify(ProjectionP aMain, size_t aProjectionNo, ChartModify& aChart) : mMain{aMain}, mProjectionNo(aProjectionNo), mChart(aChart) {}

        inline double stress() const override { return mChart.modified_projection(mProjectionNo) ? 0.0 : mMain->stress(); } // no stress if projection was modified
        inline std::shared_ptr<Layout> layout() const override { return mChart.modified_projection(mProjectionNo) ? mChart.modify_projection(mProjectionNo).layout() : mMain->layout(); }
        inline std::shared_ptr<Layout> transformed_layout() const override { return mChart.modified_projection(mProjectionNo) ? mChart.modify_projection(mProjectionNo).transformed_layout() : mMain->transformed_layout(); }
        inline std::string comment() const override { return mMain->comment(); }
        inline MinimumColumnBasis minimum_column_basis() const override { return mMain->minimum_column_basis(); }
        inline ColumnBasesP forced_column_bases() const override { return mMain->forced_column_bases(); }
        inline acmacs::Transformation transformation() const override { return mChart.modified_projection(mProjectionNo) ? mChart.modify_projection(mProjectionNo).transformation() : mMain->transformation(); }
        inline bool dodgy_titer_is_regular() const override { return mMain->dodgy_titer_is_regular(); }
        inline double stress_diff_to_stop() const override { return mMain->stress_diff_to_stop(); }
        inline PointIndexList unmovable() const override { return mMain->unmovable(); }
        inline PointIndexList disconnected() const override { return mMain->disconnected(); }
        inline PointIndexList unmovable_in_the_last_dimension() const override { return mMain->unmovable_in_the_last_dimension(); }
        inline AvidityAdjusts avidity_adjusts() const override { return mMain->avidity_adjusts(); }

        inline size_t projection_no() const { return mProjectionNo; }
        inline void move_point(size_t aPointNo, const std::vector<double>& aCoordinates) { mChart.modify_projection(mProjectionNo).move_point(aPointNo, aCoordinates); }
        inline void rotate_radians(double aAngle) { mChart.modify_projection(mProjectionNo).rotate_radians(aAngle); }
        inline void rotate_degrees(double aAngle) { mChart.modify_projection(mProjectionNo).rotate_degrees(aAngle); }
        inline void flip(double aX, double aY) { mChart.modify_projection(mProjectionNo).flip(aX, aY); }
        inline void flip_east_west() { mChart.modify_projection(mProjectionNo).flip_east_west(); }
        inline void flip_north_south() { mChart.modify_projection(mProjectionNo).flip_north_south(); }

     private:
        ProjectionP mMain;
        size_t mProjectionNo;
        ChartModify& mChart;

    }; // class ProjectionModify

// ----------------------------------------------------------------------

    class ProjectionsModify : public Projections
    {
     public:
        inline ProjectionsModify(ProjectionsP aMain, ChartModify& aChart) : mMain{aMain}, mChart(aChart) {}

        inline bool empty() const override { return mMain->empty(); }
        inline size_t size() const override { return mMain->size(); }
        inline ProjectionP operator[](size_t aIndex) const override { return std::make_shared<ProjectionModify>(mMain->operator[](aIndex), aIndex, mChart); }

     private:
        ProjectionsP mMain;
        ChartModify& mChart;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class PlotSpecModify : public PlotSpec
    {
     public:
        inline PlotSpecModify(PlotSpecP aMain, ChartModify& aChart) : mMain{aMain}, mChart(aChart), mNumberOfAntigens(aChart.number_of_antigens()) {}

        inline bool empty() const override { return mChart.modified_plot_spec() ? false : mMain->empty(); }
        inline Color error_line_positive_color() const override { return mMain->error_line_positive_color(); }
        inline Color error_line_negative_color() const override { return mMain->error_line_negative_color(); }
        inline PointStyle style(size_t aPointNo) const override { return mChart.modified_plot_spec() ? mChart.modify_plot_spec().style(aPointNo) : mMain->style(aPointNo); }
        inline std::vector<PointStyle> all_styles() const override { return mChart.modified_plot_spec() ? mChart.modify_plot_spec().all_styles() : mMain->all_styles(); }

        DrawingOrder drawing_order() const override;
        inline DrawingOrder& drawing_order_modify() { return mChart.modify_plot_spec().drawing_order(); }
        inline void raise(size_t aPointNo) { mChart.modify_plot_spec().raise(aPointNo); }
        inline void raise(const Indexes& aPoints) { std::for_each(aPoints.begin(), aPoints.end(), [&](size_t aIndex) { this->raise(aIndex); }); }
        inline void lower(size_t aPointNo) { mChart.modify_plot_spec().lower(aPointNo); }
        inline void lower(const Indexes& aPoints) { std::for_each(aPoints.begin(), aPoints.end(), [&](size_t aIndex) { this->lower(aIndex); }); }
        inline void raise_serum(size_t aSerumNo) { mChart.modify_plot_spec().raise(aSerumNo + mNumberOfAntigens); }
        inline void raise_serum(const Indexes& aSera) { std::for_each(aSera.begin(), aSera.end(), [&](size_t aIndex) { this->raise_serum(aIndex); }); }
        inline void lower_serum(size_t aSerumNo) { mChart.modify_plot_spec().lower(aSerumNo + mNumberOfAntigens); }
        inline void lower_serum(const Indexes& aSera) { std::for_each(aSera.begin(), aSera.end(), [&](size_t aIndex) { this->lower_serum(aIndex); }); }

        inline void size(size_t aPointNo, Pixels aSize) { mChart.modify_plot_spec().size(aPointNo, aSize); }
        inline void fill(size_t aPointNo, Color aFill) { mChart.modify_plot_spec().fill(aPointNo, aFill); }
        inline void outline(size_t aPointNo, Color aOutline) { mChart.modify_plot_spec().outline(aPointNo, aOutline); }
        inline void scale_all(double aPointScale, double aOulineScale) { mChart.modify_plot_spec().scale_all(aPointScale, aOulineScale); }

        inline void modify(size_t aPointNo, const PointStyle& aStyle) { mChart.modify_plot_spec().modify(aPointNo, aStyle); }
        inline void modify(const Indexes& aPoints, const PointStyle& aStyle) { std::for_each(aPoints.begin(), aPoints.end(), [&](size_t aIndex) { this->modify(aIndex, aStyle); }); }
        inline void modify_serum(size_t aSerumNo, const PointStyle& aStyle) { mChart.modify_plot_spec().modify(aSerumNo + mNumberOfAntigens, aStyle); }
        inline void modify_sera(const Indexes& aSera, const PointStyle& aStyle) { std::for_each(aSera.begin(), aSera.end(), [&](size_t aIndex) { this->modify_serum(aIndex, aStyle); }); }

     private:
        PlotSpecP mMain;
        ChartModify& mChart;
        size_t mNumberOfAntigens;

    }; // class PlotSpecModify

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
