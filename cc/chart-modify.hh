#pragma once

#include <optional>

#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/randomizer.hh"
#include "acmacs-chart-2/optimize.hh"

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

    class ChartModifyBase : public Chart
    {
     public:
        ChartModifyBase() {}

        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;

        InfoModifyP info_modify() { return info_; }
        AntigensModifyP antigens_modify() { return antigens_; }
        SeraModifyP sera_modify() { return sera_; }
        TitersModifyP titers_modify() { return titers_; }
        ColumnBasesModifyP forced_column_bases_modify() { return forced_column_bases_; }
        ProjectionsModifyP projections_modify() { return get_projections(); }
        ProjectionModifyP projection_modify(size_t aProjectionNo);
        PlotSpecModifyP plot_spec_modify() { return get_plot_spec(); }

     protected:
        // virtual void modify() {  }
        // virtual bool modified() const { return true; }
        virtual ProjectionsModifyP get_projections() const;
        virtual PlotSpecModifyP get_plot_spec() const;
        virtual void new_projections() const = 0;
        virtual void new_plot_spec() const = 0;

        InfoModifyP info_;
        AntigensModifyP antigens_;
        SeraModifyP sera_;
        TitersModifyP titers_;
        ColumnBasesModifyP forced_column_bases_;
        mutable ProjectionsModifyP projections_;
        mutable PlotSpecModifyP plot_spec_;

    }; // class ChartModifyBase

// ----------------------------------------------------------------------

    class ChartModify : public ChartModifyBase
    {
     public:
        ChartModify(ChartP main) : main_{main} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases() const override;
        bool is_merge() const override { return main_->is_merge(); }

        InfoModifyP info_modify();
        AntigensModifyP antigens_modify();
        SeraModifyP sera_modify();
        TitersModifyP titers_modify();
        ColumnBasesModifyP forced_column_bases_modify();

        std::pair<optimization_status, ProjectionModifyP> relax(MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, acmacs::chart::optimization_options options, const PointIndexList& disconnect_points = {});

     protected:
        // bool modified() const override {}
        // ProjectionsModifyP get_projections() const override;
        // PlotSpecModifyP get_plot_spec() const override;
        void new_projections() const override;
        void new_plot_spec() const override;

     private:
        ChartP main_;
        // mutable ProjectionsModifyP projections_;
        // mutable PlotSpecModifyP plot_spec_;

    }; // class ChartModify

// ----------------------------------------------------------------------

    // class ChartModifyNew : public ChartModifyBase
    // {
    //  public:
    //     ChartModifyNew() {}

    //     InfoP info() const override { return info_modify(); }
    //     AntigensP antigens() const override;
    //     SeraP sera() const override;
    //     TitersP titers() const override;
    //     ColumnBasesP forced_column_bases() const override;
    //     ProjectionsP projections() const override;
    //     PlotSpecP plot_spec() const override;
    //     bool is_merge() const override { return false; }

    //     InfoModifyP info_modify();
    //     AntigensModifyP antigens_modify();
    //     SeraModifyP sera_modify();
    //     TitersModifyP titers_modify();
    //     ColumnBasesModifyP forced_column_bases_modify();
    //     ProjectionsModifyP projections_modify();
    //     ProjectionModifyP projection_modify(size_t aProjectionNo);
    //     PlotSpecModifyP plot_spec_modify();

    //     std::pair<optimization_status, ProjectionModifyP> relax(MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, acmacs::chart::optimization_options options, const PointIndexList& disconnect_points = {});

    //  private:
        // void new_projections() const override;
        // void new_plot_spec() const override;

    //     ProjectionsModifyP get_projections() const;
    //     PlotSpecModifyP get_plot_spec() const;

    // }; // class ChartModifyNew

// ----------------------------------------------------------------------

    class InfoModify : public Info
    {
     public:
        InfoModify() = default;

        size_t number_of_sources() const override { return 0; }
        InfoP source(size_t /*aSourceNo*/) const override { return nullptr; }

     protected:
        std::optional<std::string> name_;
        std::optional<std::string> virus_;
        std::optional<std::string> virus_type_;
        std::optional<std::string> subset_;
        std::optional<std::string> assay_;
        std::optional<std::string> lab_;
        std::optional<std::string> rbc_species_;
        std::optional<std::string> date_;

    }; // class InfoModify

    class InfoModifyMain : public InfoModify
    {
     public:
        InfoModifyMain(InfoP aMain) : mMain{aMain} {}

        std::string name(Compute aCompute = Compute::No) const override { return name_ ? *name_ : mMain->name(aCompute); }
        std::string virus(Compute aCompute = Compute::No) const override { return virus_ ? *virus_ : mMain->virus(aCompute); }
        std::string virus_type(Compute aCompute = Compute::Yes) const override { return virus_type_ ? *virus_type_ : mMain->virus_type(aCompute); }
        std::string subset(Compute aCompute = Compute::No) const override { return subset_ ? *subset_ : mMain->subset(aCompute); }
        std::string assay(Compute aCompute = Compute::No) const override { return assay_ ? *assay_ : mMain->assay(aCompute); }
        std::string lab(Compute aCompute = Compute::No) const override { return lab_ ? *lab_ : mMain->lab(aCompute); }
        std::string rbc_species(Compute aCompute = Compute::No) const override { return rbc_species_ ? *rbc_species_ : mMain->rbc_species(aCompute); }
        std::string date(Compute aCompute = Compute::No) const override { return date_ ? *date_ : mMain->date(aCompute); }
        size_t number_of_sources() const override { return mMain->number_of_sources(); }
        InfoP source(size_t aSourceNo) const override { return mMain->source(aSourceNo); }

     private:
        InfoP mMain;

    }; // class InfoModifyMain

// ----------------------------------------------------------------------

    class AntigenModify : public Antigen
    {
     public:
        AntigenModify(AntigenP aMain) : mMain{aMain} {}

        Name name() const override  { return mMain->name(); }
        Date date() const override  { return mMain->date(); }
        Passage passage() const override { return mMain->passage(); }
        BLineage lineage() const override  { return mMain->lineage(); }
        Reassortant reassortant() const override { return mMain->reassortant(); }
        LabIds lab_ids() const override { return mMain->lab_ids(); }
        Clades clades() const override { return mMain->clades(); }
        Annotations annotations() const override { return mMain->annotations(); }
        bool reference() const override { return mMain->reference(); }

     private:
        AntigenP mMain;

    }; // class AntigenModify

// ----------------------------------------------------------------------

    class SerumModify : public Serum
    {
     public:
        SerumModify(SerumP aMain) : mMain{aMain} {}

        Name name() const override { return mMain->name(); }
        Passage passage() const override { return mMain->passage(); }
        BLineage lineage() const override { return mMain->lineage(); }
        Reassortant reassortant() const override { return mMain->reassortant(); }
        Annotations annotations() const override { return mMain->annotations(); }
        SerumId serum_id() const override { return mMain->serum_id(); }
        SerumSpecies serum_species() const override { return mMain->serum_species(); }
        PointIndexList homologous_antigens() const override { return mMain->homologous_antigens(); }
        void set_homologous(const std::vector<size_t>& ags) const override { return mMain->set_homologous(ags); }

     private:
        SerumP mMain;

    }; // class SerumModify

// ----------------------------------------------------------------------

    class AntigensModify : public Antigens
    {
     public:
        AntigensModify(AntigensP aMain) : mMain{aMain} {}

        size_t size() const override { return mMain->size(); }
        AntigenP operator[](size_t aIndex) const override { return std::make_shared<AntigenModify>(mMain->operator[](aIndex)); }
        std::optional<size_t> find_by_full_name(std::string aFullName) const override { return mMain->find_by_full_name(aFullName); }

     private:
        AntigensP mMain;

    }; // class AntigensModify

// ----------------------------------------------------------------------

    class SeraModify : public Sera
    {
     public:
        SeraModify(SeraP aMain) : mMain{aMain} {}

        size_t size() const override { return mMain->size(); }
        SerumP operator[](size_t aIndex) const override { return std::make_shared<SerumModify>(mMain->operator[](aIndex)); }

     private:
        SeraP mMain;

    }; // class SeraModify

// ----------------------------------------------------------------------

    class TitersModify : public Titers
    {
     public:
        TitersModify(TitersP aMain) : mMain{aMain} {}

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override { return mMain->titer(aAntigenNo, aSerumNo); }
        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override { return mMain->titer_of_layer(aLayerNo, aAntigenNo, aSerumNo); }
        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const override { return mMain->titers_for_layers(aAntigenNo, aSerumNo); }
        size_t number_of_layers() const override { return mMain->number_of_layers(); }
        size_t number_of_antigens() const override { return mMain->number_of_antigens(); }
        size_t number_of_sera() const override { return mMain->number_of_sera(); }
        size_t number_of_non_dont_cares() const override { return mMain->number_of_non_dont_cares(); }

          // support for fast exporting into ace, if source was ace or acd1
        const rjson::array& rjson_list_list() const override { return mMain->rjson_list_list(); }
        const rjson::array& rjson_list_dict() const override { return mMain->rjson_list_dict(); }
        const rjson::array& rjson_layers() const override { return mMain->rjson_layers(); }

        TiterIterator begin() const override { return mMain->begin(); }
        TiterIterator end() const override { return mMain->end(); }

     private:
        TitersP mMain;

    }; // class TitersModify

// ----------------------------------------------------------------------

    class ColumnBasesModify : public ColumnBases
    {
     public:
        ColumnBasesModify(ColumnBasesP aMain) : mMain{aMain} {}

        double column_basis(size_t aSerumNo) const override { return mMain->column_basis(aSerumNo); }
        size_t size() const override { return mMain->size(); }

     private:
        ColumnBasesP mMain;

    }; // class ColumnBasesModify

// ----------------------------------------------------------------------

    class ProjectionModifyNew;

    class ProjectionModify : public Projection
    {
     public:
        ProjectionModify(const Chart& chart) : Projection(chart) {}
        ProjectionModify(const ProjectionModify& aSource) : Projection(aSource.chart())
            {
                if (aSource.modified()) {
                    layout_ = std::make_shared<acmacs::Layout>(*aSource.layout_modified());
                    transformation_ = aSource.transformation_modified();
                }
            }

        std::optional<double> stored_stress() const override { return stress_; }
        void move_point(size_t aPointNo, const std::vector<double>& aCoordinates) { modify(); layout_->set(aPointNo, aCoordinates); transformed_layout_.reset(); }
        void rotate_radians(double aAngle) { modify(); transformation_.rotate(aAngle); transformed_layout_.reset(); }
        void rotate_degrees(double aAngle) { rotate_radians(aAngle * M_PI / 180.0); }
        void flip(double aX, double aY) { modify(); transformation_.flip(aX, aY); transformed_layout_.reset(); }
        void flip_east_west() { flip(0, 1); }
        void flip_north_south() { flip(1, 0); }

        std::shared_ptr<acmacs::Layout> layout_modified() { modify(); return layout_; }
        std::shared_ptr<acmacs::Layout> layout_modified() const { return layout_; }
        virtual void randomize_layout(double max_distance_multiplier = 1.0) { auto rnd = make_randomizer_plain(max_distance_multiplier); randomize_layout(rnd); }
        virtual void randomize_layout(const PointIndexList& to_randomize, double max_distance_multiplier = 1.0) { auto rnd = make_randomizer_plain(max_distance_multiplier); randomize_layout(to_randomize, rnd); } // randomize just some point coordinates
        virtual void randomize_layout(LayoutRandomizer& randomizer);
        virtual void randomize_layout(const PointIndexList& to_randomize, LayoutRandomizer& randomizer); // randomize just some point coordinates
        virtual void set_layout(const acmacs::Layout& layout, bool allow_size_change = false);
        virtual void set_layout(const acmacs::LayoutInterface& layout);
        virtual optimization_status relax(acmacs::chart::optimization_options options)
            {
                const auto status = acmacs::chart::optimize(*this, options);
                stress_ = status.final_stress;
                return status;
            }
        virtual std::shared_ptr<ProjectionModifyNew> clone(ChartModify& chart) const;

     protected:
        virtual void modify() { stress_.reset(); }
        virtual bool modified() const { return true; }
        double recalculate_stress() const override { stress_ = Projection::recalculate_stress(); return *stress_; }
        bool layout_present() const { return static_cast<bool>(layout_); }
        void clone_from(const Projection& aSource) { layout_ = std::make_shared<acmacs::Layout>(*aSource.layout()); transformation_ = aSource.transformation(); transformed_layout_.reset(); }
        std::shared_ptr<Layout> transformed_layout_modified() const { if (!transformed_layout_) transformed_layout_.reset(layout_->transform(transformation_)); return transformed_layout_; }
        size_t number_of_points_modified() const { return layout_->number_of_points(); }
        size_t number_of_dimensions_modified() const { return layout_->number_of_dimensions(); }
        const Transformation& transformation_modified() const { return transformation_; }
        void new_layout(size_t number_of_points, size_t number_of_dimensions) { layout_ = std::make_shared<acmacs::Layout>(number_of_points, number_of_dimensions); transformation_.reset(); transformed_layout_.reset(); }

     private:
        std::shared_ptr<acmacs::Layout> layout_;
        Transformation transformation_;
        mutable std::shared_ptr<acmacs::chart::Layout> transformed_layout_;
        mutable std::optional<double> stress_;

        friend class ProjectionsModify;

        LayoutRandomizerPlain make_randomizer_plain(double max_distance_multiplier) const;

    }; // class ProjectionModify

// ----------------------------------------------------------------------

    class ProjectionModifyMain : public ProjectionModify
    {
     public:
        ProjectionModifyMain(ProjectionP main) : ProjectionModify(main->chart()), main_{main} {}
        ProjectionModifyMain(const ProjectionModifyMain& aSource) : ProjectionModify(aSource), main_(aSource.main_) {}

        std::optional<double> stored_stress() const override { if (modified()) return ProjectionModify::stored_stress(); else return main_->stored_stress(); } // no stress if projection was modified
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
        void modify() override { ProjectionModify::modify(); if (!modified()) clone_from(*main_);  }

     private:
        ProjectionP main_;

    }; // class ProjectionModifyMain

// ----------------------------------------------------------------------

    class ProjectionModifyNew : public ProjectionModify
    {
     public:
        ProjectionModifyNew(const Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
            : ProjectionModify(chart), minimum_column_basis_(minimum_column_basis), forced_column_bases_(chart.forced_column_bases())
            {
                new_layout(chart.number_of_points(), number_of_dimensions);
            }

        ProjectionModifyNew(const ProjectionModify& aSource)
            : ProjectionModify(aSource), minimum_column_basis_(aSource.minimum_column_basis()),
              forced_column_bases_(std::make_shared<ColumnBasesData>(*aSource.forced_column_bases())),
              dodgy_titer_is_regular_(aSource.dodgy_titer_is_regular()), stress_diff_to_stop_(aSource.stress_diff_to_stop()),
              disconnected_(aSource.disconnected())
            {
                set_layout(*aSource.layout());
            }

        std::shared_ptr<Layout> layout() const override { return layout_modified(); }
        std::shared_ptr<Layout> transformed_layout() const override { return transformed_layout_modified(); }
        std::string comment() const override { return {}; }
        size_t number_of_points() const override { return number_of_points_modified(); }
        size_t number_of_dimensions() const override { return number_of_dimensions_modified(); }
        MinimumColumnBasis minimum_column_basis() const override { return minimum_column_basis_; }
        ColumnBasesP forced_column_bases() const override { return forced_column_bases_; }
        acmacs::Transformation transformation() const override { return transformation_modified(); }
        bool dodgy_titer_is_regular() const override { return dodgy_titer_is_regular_; }
        double stress_diff_to_stop() const override { return stress_diff_to_stop_; }
        PointIndexList unmovable() const override { return {}; }
        PointIndexList disconnected() const override { return disconnected_; }
        void set_disconnected(const PointIndexList& disconnect) { disconnected_ = disconnect; }
        void connect(const PointIndexList& to_connect);
        PointIndexList unmovable_in_the_last_dimension() const override { return {}; }
        AvidityAdjusts avidity_adjusts() const override { return {}; }

     private:
        MinimumColumnBasis minimum_column_basis_;
        ColumnBasesP forced_column_bases_;
        bool dodgy_titer_is_regular_{false};
        double stress_diff_to_stop_{0};
        PointIndexList disconnected_;

    }; // class ProjectionModifyNew

// ----------------------------------------------------------------------

    class ProjectionsModify : public Projections
    {
     public:
        ProjectionsModify(ProjectionsP main)
            : Projections(main->chart()), projections_(main->size(), nullptr)
            {
                std::transform(main->begin(), main->end(), projections_.begin(), [](ProjectionP aSource) { return std::make_shared<ProjectionModifyMain>(aSource); });
                set_projection_no();
            }

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override { return projections_.at(aIndex); }
        ProjectionModifyP at(size_t aIndex) const { return projections_.at(aIndex); }
          // ProjectionModifyP clone(size_t aIndex) { auto cloned = projections_.at(aIndex)->clone(); projections_.push_back(cloned); return cloned; }
        void sort() { std::sort(projections_.begin(), projections_.end(), [](const auto& p1, const auto& p2) { return p1->stress() < p2->stress(); }); set_projection_no(); }
        void add(std::shared_ptr<ProjectionModify> projection);

        std::shared_ptr<ProjectionModifyNew> new_from_scratch(size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis);

        void keep_just(size_t number_of_projections_to_keep)
            {
                if (projections_.size() > number_of_projections_to_keep)
                    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(number_of_projections_to_keep), projections_.end());
            }

        void remove(size_t projection_no);
        void remove_all_except(size_t projection_no);

     private:
        std::vector<ProjectionModifyP> projections_;

        void set_projection_no() { std::for_each(acmacs::index_iterator(0UL), acmacs::index_iterator(projections_.size()), [this](auto index) { this->projections_[index]->set_projection_no(index); }); }

        friend class ChartModify;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class PlotSpecModify : public PlotSpec
    {
     public:
        PlotSpecModify(PlotSpecP main, size_t number_of_antigens) : main_{main}, number_of_antigens_(number_of_antigens) {}

        bool empty() const override { return modified() ? false : main_->empty(); }
        Color error_line_positive_color() const override { return main_->error_line_positive_color(); }
        Color error_line_negative_color() const override { return main_->error_line_negative_color(); }
        PointStyle style(size_t aPointNo) const override { return modified() ? style_modified(aPointNo) : main_->style(aPointNo); }
        std::vector<PointStyle> all_styles() const override { return modified() ? styles_ : main_->all_styles(); }
        size_t number_of_points() const override { return modified() ? styles_.size() : main_->number_of_points(); }

        DrawingOrder drawing_order() const override
            {
                if (modified())
                    return drawing_order_;
                auto drawing_order = main_->drawing_order();
                drawing_order.fill_if_empty(number_of_points());
                return drawing_order;
            }

        DrawingOrder& drawing_order_modify() { modify(); return drawing_order_; }
        void raise(size_t point_no) { modify(); validate_point_no(point_no); drawing_order_.raise(point_no); }
        void raise(const Indexes& points) { modify(); std::for_each(points.begin(), points.end(), [this](size_t index) { this->raise(index); }); }
        void lower(size_t point_no) { modify(); validate_point_no(point_no); drawing_order_.lower(point_no); }
        void lower(const Indexes& points) { modify(); std::for_each(points.begin(), points.end(), [this](size_t index) { this->lower(index); }); }
        void raise_serum(size_t serum_no) { raise(serum_no + number_of_antigens_); }
        void raise_serum(const Indexes& sera) { std::for_each(sera.begin(), sera.end(), [this](size_t index) { this->raise(index + this->number_of_antigens_); }); }
        void lower_serum(size_t serum_no) { lower(serum_no + number_of_antigens_); }
        void lower_serum(const Indexes& sera) { std::for_each(sera.begin(), sera.end(), [this](size_t index) { this->lower(index + this->number_of_antigens_); }); }

        void size(size_t point_no, Pixels size) { modify(); validate_point_no(point_no); styles_[point_no].size = size; }
        void fill(size_t point_no, Color fill) { modify(); validate_point_no(point_no); styles_[point_no].fill = fill; }
        void outline(size_t point_no, Color outline) { modify(); validate_point_no(point_no); styles_[point_no].outline = outline; }
        void scale_all(double point_scale, double outline_scale) { modify(); std::for_each(styles_.begin(), styles_.end(), [=](auto& style) { style.scale(point_scale).scale_outline(outline_scale); }); }

        void modify(size_t point_no, const PointStyle& style) { modify(); validate_point_no(point_no); styles_[point_no] = style; }
        void modify(const Indexes& points, const PointStyle& style) { modify(); std::for_each(points.begin(), points.end(), [this,&style](size_t index) { this->modify(index, style); }); }
        void modify_serum(size_t serum_no, const PointStyle& style) { modify(serum_no + number_of_antigens_, style); }
        void modify_sera(const Indexes& sera, const PointStyle& style) { std::for_each(sera.begin(), sera.end(), [this,&style](size_t index) { this->modify(index + this->number_of_antigens_, style); }); }

     protected:
        virtual bool modified() const { return modified_; }
        virtual void modify() { if (!modified()) clone_from(*main_); }
        void clone_from(const PlotSpec& aSource) { modified_ = true; styles_ = aSource.all_styles(); drawing_order_ = aSource.drawing_order(); drawing_order_.fill_if_empty(number_of_points()); }
        const PointStyle& style_modified(size_t point_no) const { return styles_.at(point_no); }

        void validate_point_no(size_t point_no) const
            {
                // std::cerr << "DEBUG: PlotSpecModify::validate_point_no: number_of_points main: " << main_->number_of_points() << " modified: " << modified() << " number_of_points: " << number_of_points() << DEBUG_LINE_FUNC << '\n';
                if (point_no >= number_of_points())
                    throw std::runtime_error("Invalid point number: " + acmacs::to_string(point_no) + ", expected integer in range 0.." + acmacs::to_string(number_of_points() - 1) + ", inclusive");
            }

     private:
        PlotSpecP main_;
        size_t number_of_antigens_;
        bool modified_ = false;
        std::vector<PointStyle> styles_;
        DrawingOrder drawing_order_;

    }; // class PlotSpecModify

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
