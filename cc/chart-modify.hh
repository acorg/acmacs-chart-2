#pragma once

#include <variant>

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

    class ChartModify : public Chart
    {
     public:
        explicit ChartModify();
        explicit ChartModify(ChartP main) : main_{main} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases() const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;

        bool is_merge() const override { return main_ ? main_->is_merge() : false; }

        InfoModifyP info_modify();
        AntigensModifyP antigens_modify();
        SeraModifyP sera_modify();
        TitersModifyP titers_modify();
        ColumnBasesModifyP forced_column_bases_modify();
        ProjectionsModifyP projections_modify();
        ProjectionModifyP projection_modify(size_t aProjectionNo);
        PlotSpecModifyP plot_spec_modify();

        std::pair<optimization_status, ProjectionModifyP> relax(MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, acmacs::chart::optimization_options options, const PointIndexList& disconnect_points = {});

     private:
        ChartP main_;
        InfoModifyP info_;
        AntigensModifyP antigens_;
        SeraModifyP sera_;
        TitersModifyP titers_;
        ColumnBasesModifyP forced_column_bases_;
        ProjectionsModifyP projections_;
        PlotSpecModifyP plot_spec_;

    }; // class ChartModify

// ----------------------------------------------------------------------

    class InfoModify : public Info
    {
     public:
        explicit InfoModify() = default;
        explicit InfoModify(InfoP main);

        std::string name(Compute aCompute = Compute::No) const override { return aCompute == Compute::No ? name_ : computed_name_; }
        std::string virus(Compute /*aCompute*/ = Compute::No) const override { return virus_; }
        std::string virus_type(Compute /*aCompute*/ = Compute::Yes) const override { return virus_type_; }
        std::string subset(Compute /*aCompute*/ = Compute::No) const override { return subset_; }
        std::string assay(Compute /*aCompute*/ = Compute::No) const override { return assay_; }
        std::string lab(Compute /*aCompute*/ = Compute::No) const override { return lab_; }
        std::string rbc_species(Compute /*aCompute*/ = Compute::No) const override { return rbc_species_; }
        std::string date(Compute /*aCompute*/ = Compute::No) const override { return date_; }
        size_t number_of_sources() const override { return 0; }
        InfoP source(size_t /*aSourceNo*/) const override { return nullptr; }

        void name(std::string value) { name_ = value; computed_name_ = value; }
        void virus(std::string value) { virus_ = value; }
        void virus_type(std::string value) { virus_type_ = value; }
        void subset(std::string value) { subset_ = value; }
        void assay(std::string value) { assay_ = value; }
        void lab(std::string value) { lab_ = value; }
        void rbc_species(std::string value) { rbc_species_ = value; }
        void date(std::string value) { date_ = value; }

     protected:
        std::string name_;
        std::string computed_name_;
        std::string virus_;
        std::string virus_type_;
        std::string subset_;
        std::string assay_;
        std::string lab_;
        std::string rbc_species_;
        std::string date_;

    }; // class InfoModify

// ----------------------------------------------------------------------

    class AntigenModify : public Antigen
    {
     public:
        explicit AntigenModify() = default;
        explicit AntigenModify(AntigenP main);

        Name name() const override { return name_; }
        Date date() const override { return date_; }
        Passage passage() const override { return passage_; }
        BLineage lineage() const override { return lineage_; }
        Reassortant reassortant() const override { return reassortant_; }
        LabIds lab_ids() const override { return lab_ids_; }
        Clades clades() const override { return clades_; }
        Annotations annotations() const override { return annotations_; }
        bool reference() const override { return reference_; }

        void name(const std::string& value) { name_ = value; }
        void date(const std::string& value) { date_ = value; }
        void passage(const std::string& value) { passage_ = value; }
        void lineage(const std::string& value) { lineage_ = value; }
        void reassortant(const std::string& value) { reassortant_ = value; }
        void reference(bool value) { reference_ = value; }

     private:
        Name name_;
        Date date_;
        Passage passage_;
        BLineage lineage_;
        Reassortant reassortant_;
        Annotations annotations_;
        LabIds lab_ids_;
        Clades clades_;
        bool reference_ = false;

    }; // class AntigenModify

// ----------------------------------------------------------------------

    class SerumModify : public Serum
    {
     public:
        explicit SerumModify() = default;
        explicit SerumModify(SerumP main);

        Name name() const override { return name_; }
        Passage passage() const override { return passage_; }
        BLineage lineage() const override { return lineage_; }
        Reassortant reassortant() const override { return reassortant_; }
        Annotations annotations() const override { return annotations_; }
        SerumId serum_id() const override { return serum_id_; }
        SerumSpecies serum_species() const override { return serum_species_; }
        PointIndexList homologous_antigens() const override { return homologous_antigens_; }
        void set_homologous(const std::vector<size_t>& ags) const override { homologous_antigens_ = ags; }

        void name(const std::string& value) { name_ = value; }
        void passage(const std::string& value) { passage_ = value; }
        void lineage(const std::string& value) { lineage_ = value; }
        void reassortant(const std::string& value) { reassortant_ = value; }
        void serum_id(const std::string& value) { serum_id_ = value; }
        void serum_species(const std::string& value) { serum_species_ = value; }

     private:
        Name name_;
        Passage passage_;
        BLineage lineage_;
        Reassortant reassortant_;
        Annotations annotations_;
        SerumId serum_id_;
        SerumSpecies serum_species_;
        mutable PointIndexList homologous_antigens_;

    }; // class SerumModify

// ----------------------------------------------------------------------

    class AntigensModify : public Antigens
    {
     public:
        explicit AntigensModify(AntigensP main);
        explicit AntigensModify() = default;

        size_t size() const override { return antigens_.size(); }
        AntigenP operator[](size_t aIndex) const override { return antigens_.at(aIndex); }

     private:
        std::vector<AntigenModifyP> antigens_;

    }; // class AntigensModify

// ----------------------------------------------------------------------

    class SeraModify : public Sera
    {
     public:
        explicit SeraModify() = default;
        explicit SeraModify(SeraP main);

        size_t size() const override { return sera_.size(); }
        SerumP operator[](size_t aIndex) const override { return sera_.at(aIndex); }

     private:
        std::vector<SerumModifyP> sera_;

    }; // class SeraModify

// ----------------------------------------------------------------------

    class titers_cannot_be_modified : public std::runtime_error { public: titers_cannot_be_modified() : std::runtime_error("titers cannot be modified") {} };

    class TitersModify : public Titers
    {
     public:
        explicit TitersModify();
        explicit TitersModify(TitersP main);

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override;
        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const override;
        size_t number_of_layers() const override { return layers_.size(); }
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override { return number_of_sera_; }
        size_t number_of_non_dont_cares() const override;

        void titer(size_t aAntigenNo, size_t aSerumNo, const std::string& aTiter);
        void all_dontcare_for_antigen(size_t aAntigenNo);
        void all_dontcare_for_serum(size_t aSerumNo);
        void all_multiply_by_for_antigen(size_t aAntigenNo, double multiply_by);
        void all_multiply_by_for_serum(size_t aSerumNo, double multiply_by);

     private:
        using dense_t = std::vector<Titer>;
        using sparse_entry_t = std::pair<size_t, Titer>; // serum no, titer
        using sparse_row_t = std::vector<sparse_entry_t>;
        using sparse_t = std::vector<sparse_row_t>;    // size = number_of_antigens
        using titers_t = std::variant<dense_t, sparse_t>;
        using layers_t = std::vector<sparse_t>;

          // size_t number_of_antigens_;
        size_t number_of_sera_;
        titers_t titers_;
        layers_t layers_;

        bool titers_modifiable() const noexcept { return layers_.empty(); }
        void titers_modifiable_check() const { if (!titers_modifiable()) throw titers_cannot_be_modified{}; }

        static Titer find_titer_for_serum(const sparse_row_t& aRow, size_t aSerumNo);
        static Titer titer_in_sparse_t(const sparse_t& aSparse, size_t aAntigenNo, size_t aSerumNo);

    }; // class TitersModify

// ----------------------------------------------------------------------

    class ColumnBasesModify : public ColumnBasesData
    {
     public:
        explicit ColumnBasesModify(ColumnBasesP aMain) : ColumnBasesData{*aMain} {}

    }; // class ColumnBasesModify

// ----------------------------------------------------------------------

    class ProjectionModifyNew;

    class ProjectionModify : public Projection
    {
     public:
        explicit ProjectionModify(const Chart& chart) : Projection(chart) {}
        explicit ProjectionModify(const ProjectionModify& aSource) : Projection(aSource.chart())
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
        explicit ProjectionModifyMain(ProjectionP main) : ProjectionModify(main->chart()), main_{main} {}
        explicit ProjectionModifyMain(const ProjectionModifyMain& aSource) : ProjectionModify(aSource), main_(aSource.main_) {}

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
        explicit ProjectionModifyNew(const Chart& chart, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
            : ProjectionModify(chart), minimum_column_basis_(minimum_column_basis), forced_column_bases_(chart.forced_column_bases())
            {
                new_layout(chart.number_of_points(), number_of_dimensions);
            }

        explicit ProjectionModifyNew(const ProjectionModify& aSource)
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
        explicit ProjectionsModify(ProjectionsP main)
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
        explicit PlotSpecModify(PlotSpecP main, size_t number_of_antigens) : main_{main}, number_of_antigens_(number_of_antigens) {}

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
