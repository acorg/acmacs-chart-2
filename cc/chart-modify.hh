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

    // using ProjectionId = size_t;

    class ChartModify : public Chart
    {
     public:

        inline ChartModify(ChartP main) : main_{main} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases() const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        inline bool is_merge() const override { return main_->is_merge(); }

        InfoModifyP info_modify();
        AntigensModifyP antigens_modify();
        SeraModifyP sera_modify();
        TitersModifyP titers_modify();
        ColumnBasesModifyP forced_column_bases_modify();
        ProjectionsModifyP projections_modify();
        ProjectionModifyP projection_modify(size_t aProjectionNo);
        PlotSpecModifyP plot_spec_modify();

     private:
        ChartP main_;
        mutable ProjectionsModifyP projections_;

        ProjectionsModifyP get_projections() const;

        std::optional<internal::PlotSpecModifyData> mPlotSpecModifyData;

        internal::PlotSpecModifyData& modify_plot_spec();
        inline bool modified_plot_spec() const { return mPlotSpecModifyData.has_value(); }

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
        inline std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const override { return mMain->titers_for_layers(aAntigenNo, aSerumNo); }
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

        inline double column_basis(size_t aSerumNo) const override { return mMain->column_basis(aSerumNo); }
        inline size_t size() const override { return mMain->size(); }

     private:
        ColumnBasesP mMain;

    }; // class ColumnBasesModify

// ----------------------------------------------------------------------

    class ProjectionModify : public Projection
    {
     public:
        ProjectionModify() = default;
        ProjectionModify(const ProjectionModify& aSource)
            {
                if (aSource.modified()) {
                    layout_ = std::make_shared<acmacs::Layout>(*aSource.layout_modified());
                    transformation_ = aSource.transformation_modified();
                }
            }

        void move_point(size_t aPointNo, const std::vector<double>& aCoordinates) { modify(); layout_->set(aPointNo, aCoordinates); transformed_layout_.reset(); }
        void rotate_radians(double aAngle) { modify(); transformation_.rotate(aAngle); transformed_layout_.reset(); }
        void rotate_degrees(double aAngle) { rotate_radians(aAngle * M_PI / 180.0); }
        void flip(double aX, double aY) { modify(); transformation_.flip(aX, aY); transformed_layout_.reset(); }
        void flip_east_west() { flip(0, 1); }
        void flip_north_south() { flip(1, 0); }

     protected:
        virtual void modify() {}
        virtual bool modified() const { return true; }
        bool layout_present() const { return static_cast<bool>(layout_); }
        void clone_from(const Projection& aSource) { layout_ = std::make_shared<acmacs::Layout>(*aSource.layout()); transformation_ = aSource.transformation(); transformed_layout_.reset(); }
        std::shared_ptr<Layout> layout_modified() const { return layout_; }
        std::shared_ptr<Layout> transformed_layout_modified() const { if (!transformed_layout_) transformed_layout_.reset(layout_->transform(transformation_)); return transformed_layout_; }
        size_t number_of_points_modified() const { return layout_->number_of_points(); }
        size_t number_of_dimensions_modified() const { return layout_->number_of_dimensions(); }
        const Transformation& transformation_modified() const { return transformation_; }

     private:
        std::shared_ptr<Layout> layout_;
        Transformation transformation_;
        mutable std::shared_ptr<acmacs::chart::Layout> transformed_layout_;

        friend class ProjectionsModify;
        virtual ProjectionModifyP clone() const = 0;

    }; // class ProjectionModify

// ----------------------------------------------------------------------

    class ProjectionModifyMain : public ProjectionModify
    {
     public:
        ProjectionModifyMain(ProjectionP main) : main_{main} {}
        ProjectionModifyMain(const ProjectionModifyMain& aSource) : ProjectionModify(aSource), main_(aSource.main_) {}

        double stress() const override { return modified() ? 0.0 : main_->stress(); } // no stress if projection was modified
        std::shared_ptr<Layout> layout() const override { return modified() ? layout_modified() : main_->layout(); }
        std::shared_ptr<Layout> transformed_layout() const override { return modified() ? transformed_layout_modified() : main_->transformed_layout(); }
        std::string comment() const override { return main_->comment(); }
        size_t number_of_points() const override { return modified() ? number_of_points_modified() : main_->number_of_points(); }
        size_t number_of_dimensions() const override { return modified() ? number_of_dimensions_modified() : main_->number_of_dimensions(); }
        MinimumColumnBasis minimum_column_basis() const override { return main_->minimum_column_basis(); }
        ColumnBasesP forced_column_bases() const override { return main_->forced_column_bases(); }
        acmacs::Transformation transformation() const override { return modified() ? transformation_modified() : main_->transformation(); }
        bool dodgy_titer_is_regular() const override { return main_->dodgy_titer_is_regular(); }
        double stress_diff_to_stop() const override { return main_->stress_diff_to_stop(); }
        PointIndexList unmovable() const override { return main_->unmovable(); }
        PointIndexList disconnected() const override { return main_->disconnected(); }
        PointIndexList unmovable_in_the_last_dimension() const override { return main_->unmovable_in_the_last_dimension(); }
        AvidityAdjusts avidity_adjusts() const override { return main_->avidity_adjusts(); }

     protected:
        bool modified() const override { return layout_present(); }
        void modify() override { if (!modified()) clone_from(*main_); }

     private:
        ProjectionP main_;

        ProjectionModifyP clone() const override { return std::make_shared<ProjectionModifyMain>(*this); }

    }; // class ProjectionModifyMain

// ----------------------------------------------------------------------

    class ProjectionsModify : public Projections
    {
     public:
        inline ProjectionsModify(ProjectionsP aMain)
            : projections_(aMain->size(), nullptr)
            {
                std::transform(aMain->begin(), aMain->end(), projections_.begin(), [](ProjectionP aSource) mutable { return std::make_shared<ProjectionModifyMain>(aSource); });
            }

        inline bool empty() const override { return projections_.empty(); }
        inline size_t size() const override { return projections_.size(); }
        inline ProjectionP operator[](size_t aIndex) const override { return projections_.at(aIndex); }
        inline ProjectionModifyP at(size_t aIndex) const { return projections_.at(aIndex); }
        inline ProjectionModifyP clone(size_t aIndex) { auto cloned = projections_.at(aIndex)->clone(); projections_.push_back(cloned); return cloned; }
        inline void sort() { std::sort(projections_.begin(), projections_.end(), [](const auto& p1, const auto& p2) { return p1->stress() < p2->stress(); }); }

     private:
        std::vector<ProjectionModifyP> projections_;

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
