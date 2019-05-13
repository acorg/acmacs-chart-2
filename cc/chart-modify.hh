#pragma once

#include <variant>
#include <memory>

#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/procrustes.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class ChartModify;
    class InfoModify;
    class AntigenModify;
    class SerumModify;
    template <typename Base, typename Modify, typename ModifyBase> class AntigensSeraModify;
    using AntigensModify = AntigensSeraModify<Antigens, AntigenModify, Antigen>;
    using SeraModify = AntigensSeraModify<Sera, SerumModify, Serum>;
    class TitersModify;
    class ColumnBasesModify;
    class ProjectionModify;
    class ProjectionsModify;
    class PlotSpecModify;
    class LayoutRandomizer;

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

    enum class use_dimension_annealing { no, yes };
    constexpr inline use_dimension_annealing use_dimension_annealing_from_bool(bool use) { return use ? use_dimension_annealing::yes : use_dimension_annealing::no; }

    enum class report_stresses { no, yes };

// ----------------------------------------------------------------------

    class ChartModify : public Chart
    {
     public:
        explicit ChartModify(ChartP main) : main_{main} {}

        InfoP info() const override;
        AntigensP antigens() const override;
        SeraP sera() const override;
        TitersP titers() const override;
        ColumnBasesP forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const override;
        ProjectionsP projections() const override;
        PlotSpecP plot_spec() const override;
        const rjson::value& extension_field(std::string field_name) const override;

        bool is_merge() const override { return main_ ? main_->is_merge() : false; }

        InfoModifyP info_modify();
        AntigensModifyP antigens_modify();
        SeraModifyP sera_modify();
        TitersModifyP titers_modify();
        ColumnBasesModifyP forced_column_bases_modify(MinimumColumnBasis aMinimumColumnBasis);
        ColumnBasesModifyP forced_column_bases_modify(const ColumnBases& source);
        ProjectionsModifyP projections_modify();
        ProjectionModifyP projection_modify(size_t aProjectionNo);
        PlotSpecModifyP plot_spec_modify();

        std::pair<optimization_status, ProjectionModifyP> relax(MinimumColumnBasis minimum_column_basis, number_of_dimensions_t number_of_dimensions, use_dimension_annealing dimension_annealing, acmacs::chart::optimization_options options, const PointIndexList& disconnect_points = {});
        void relax(number_of_optimizations_t number_of_optimizations, MinimumColumnBasis minimum_column_basis, number_of_dimensions_t number_of_dimensions, use_dimension_annealing dimension_annealing, acmacs::chart::optimization_options options, enum report_stresses report_stresses, const PointIndexList& disconnect_points = {});
        void relax_incremetal(size_t source_projection_no, number_of_optimizations_t number_of_optimizations, acmacs::chart::optimization_options options, const PointIndexList& disconnect_points = {}, bool remove_source_projection = true);

        void remove_layers();
        void remove_antigens(const ReverseSortedIndexes& indexes);
        void remove_sera(const ReverseSortedIndexes& indexes);
        AntigenModifyP insert_antigen(size_t before);
        SerumModifyP insert_serum(size_t before);

        void merge(const Chart& merge_in);

     protected:
        explicit ChartModify(size_t number_of_antigens, size_t number_of_sera);
        explicit ChartModify(const Chart& source, bool copy_projections, bool copy_plot_spec);

     private:
        ChartP main_;
        InfoModifyP info_;
        AntigensModifyP antigens_;
        SeraModifyP sera_;
        TitersModifyP titers_;
        ColumnBasesModifyP forced_column_bases_;
        ProjectionsModifyP projections_;
        PlotSpecModifyP plot_spec_;

        // std::shared_ptr<LayoutRandomizer> make_randomizer(const Stress& stress, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, double diameter_multiplier) const;

    }; // class ChartModify

// ----------------------------------------------------------------------

    class ChartNew : public ChartModify
    {
     public:
        explicit ChartNew(size_t number_of_antigens, size_t number_of_sera);

    }; // class ChartNew

// ----------------------------------------------------------------------

    class ChartClone : public ChartModify
    {
     public:
        enum class clone_data {
            titers,             // info, antigens, sera, titers, forced_column_bases
            projections,         // titers+projections
            plot_spec,            // titers+plot_spec
            projections_plot_spec,            // titers+projections+plot_spec
        };

        explicit ChartClone(const Chart& source, clone_data cd = clone_data::titers);
        explicit ChartClone(ChartP source, clone_data cd = clone_data::titers) : ChartClone(*source, cd) {}

    }; // class ChartNew

// ----------------------------------------------------------------------

    class InfoModify : public Info
    {
     public:
        explicit InfoModify() = default;
        explicit InfoModify(InfoP main);

        std::string name(Compute aCompute = Compute::No) const override { return aCompute == Compute::No ? name_ : computed_name_; }
        Virus       virus(Compute aCompute = Compute::No) const override;
        VirusType   virus_type(Compute aCompute = Compute::Yes) const override;
        std::string subset(Compute aCompute = Compute::No) const override;
        Assay       assay(Compute aCompute = Compute::No) const override;
        Lab         lab(Compute aCompute = Compute::No, FixLab fix = FixLab::yes) const override;
        RbcSpecies  rbc_species(Compute aCompute = Compute::No) const override;
        TableDate   date(Compute aCompute = Compute::No) const override;
        size_t number_of_sources() const override { return sources_.size(); }
        InfoP source(size_t aSourceNo) const override { return sources_.at(aSourceNo); }

        void name(std::string value) { name_ = value; computed_name_ = value; }
        void name_append(std::string value) { name_ = ::string::join(" ", {name_, value}); computed_name_ = name_; }
        void virus(std::string value) { virus_ = value; }
        void virus_type(std::string value) { virus_type_ = value; }
        void subset(std::string value) { subset_ = value; }
        void assay(std::string value) { assay_ = value; }
        void lab(std::string value) { lab_ = value; }
        void rbc_species(std::string value) { rbc_species_ = value; }
        void date(std::string value) { date_ = value; }
        void remove_sources() { sources_.clear(); }
        void add_source(InfoP source) { sources_.push_back(std::make_shared<InfoModify>(source)); }

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
        std::vector<InfoP> sources_;

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
        Continent continent() const override { return continent_; }

        void name(const std::string& value) { name_ = value; }
        void date(const std::string& value) { date_ = value; }
        void passage(const std::string& value) { passage_ = value; }
        void lineage(const std::string& value) { lineage_ = value; }
        void reassortant(const std::string& value) { reassortant_ = value; }
        void reference(bool value) { reference_ = value; }
        void add_annotation(const std::string& annotation) { annotations_.add(annotation); }
        void add_clade(const std::string& clade) { clades_.add(clade); }
        void remove_annotation(const std::string& annotation) { annotations_.remove(annotation); }
        void continent(const std::string& value) { continent_ = value; }
        void set_continent();

        void replace_with(AntigenP main);
        void update_with(AntigenP main);

     private:
        Name name_;
        Date date_;
        Passage passage_;
        BLineage lineage_;
        Reassortant reassortant_;
        Annotations annotations_;
        LabIds lab_ids_;
        Clades clades_;
        Continent continent_;
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
        void add_annotation(const std::string& annotation) { annotations_.add(annotation); }
        void remove_annotation(const std::string& annotation) { annotations_.remove(annotation); }
        void set_continent() {}

        void replace_with(SerumP main);
        void update_with(SerumP main);

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

    template <typename Base, typename Modify, typename ModifyBase> class AntigensSeraModify : public Base
    {
      public:
        explicit AntigensSeraModify(size_t number_of) : data_(number_of, nullptr)
        {
            std::transform(data_.begin(), data_.end(), data_.begin(), [](const auto&) { return std::make_shared<Modify>(); });
        }
        explicit AntigensSeraModify(std::shared_ptr<Base> main) : data_(main->size(), nullptr)
        {
            std::transform(main->begin(), main->end(), data_.begin(), [](auto ag_sr) { return std::make_shared<Modify>(ag_sr); });
        }

        size_t size() const override { return data_.size(); }
        std::shared_ptr<ModifyBase> operator[](size_t aIndex) const override { return data_.at(aIndex); }
        Modify& at(size_t aIndex) { return *data_.at(aIndex); }
        std::shared_ptr<Modify> ptr_at(size_t aIndex) { return data_.at(aIndex); }

        void remove(const ReverseSortedIndexes& indexes)
        {
            for (auto index : indexes) {
                if (index >= data_.size())
                    throw invalid_data{"invalid index to remove: " + to_string(index) + ", valid values in [0.." + to_string(data_.size()) + ')'};
                data_.erase(data_.begin() + static_cast<Indexes::difference_type>(index));
            }
        }

        std::shared_ptr<Modify> insert(size_t before)
        {
            if (before > data_.size())
                throw invalid_data{"invalid index to insert before: " + to_string(before) + ", valid values in [0.." + to_string(data_.size()) + ']'};
            return *data_.emplace(data_.begin() + static_cast<Indexes::difference_type>(before), new Modify);
        }

        void set_continent()
        {
            for (auto& entry : data_)
                entry->set_continent();
        }

        void duplicates_distinct(const duplicates_t& dups)
        {
            for (const auto& entry : dups) {
                for (auto ip = entry.begin() + 1; ip != entry.end(); ++ip)
                    at(*ip).add_annotation("DISTINCT");
            }
        }

      private:
        std::vector<std::shared_ptr<Modify>> data_;
    };

// ----------------------------------------------------------------------

    class titers_cannot_be_modified : public std::runtime_error
    {
      public:
        titers_cannot_be_modified() : std::runtime_error("titers cannot be modified (table has layers?)") {}
    };

    class TitersModify : public Titers
    {
     public:
        using dense_t = std::vector<Titer>;
        using sparse_entry_t = std::pair<size_t, Titer>; // serum no, titer
        using sparse_row_t = std::vector<sparse_entry_t>;
        using sparse_t = std::vector<sparse_row_t>;    // size = number_of_antigens
        using titers_t = std::variant<dense_t, sparse_t>;
        using layers_t = std::vector<sparse_t>;

        enum class titer_merge {
            all_dontcare,
            less_and_more_than,
            less_than_only,
            more_than_only_adjust_to_next,
            more_than_only_to_dontcare,
            sd_too_big,
            regular_only,
            max_less_than_bigger_than_max_regular,
            less_than_and_regular,
            min_more_than_less_than_min_regular,
            more_than_and_regular
        };

        struct titer_merge_data
        {
            titer_merge_data(Titer&& a_titer, size_t ag_no, size_t sr_no, titer_merge a_report) : titer{std::move(a_titer)}, antigen{ag_no}, serum{sr_no}, report{a_report} {}
            Titer titer;
            size_t antigen, serum;
            titer_merge report;
        };

        using titer_merge_report = std::vector<titer_merge_data>;

        explicit TitersModify(size_t number_of_antigens, size_t number_of_sera);
        explicit TitersModify(TitersP main);

        Titer titer(size_t aAntigenNo, size_t aSerumNo) const override;
        size_t number_of_antigens() const override;
        size_t number_of_sera() const override { return number_of_sera_; }
        size_t number_of_non_dont_cares() const override;

        bool modifiable() const noexcept { return layers_.empty(); }
        void modifiable_check() const { if (!modifiable()) throw titers_cannot_be_modified{}; }

        void titer(size_t aAntigenNo, size_t aSerumNo, const std::string& aTiter);
        void dontcare_for_antigen(size_t aAntigenNo);
        void dontcare_for_serum(size_t aSerumNo);
        void multiply_by_for_antigen(size_t aAntigenNo, double multiply_by);
        void multiply_by_for_serum(size_t aSerumNo, double multiply_by);
        void set_proportion_of_titers_to_dont_care(double proportion);

        void remove_antigens(const ReverseSortedIndexes& indexes);
        void remove_sera(const ReverseSortedIndexes& indexes);
        void insert_antigen(size_t before);
        void insert_serum(size_t before);

        size_t number_of_layers() const override { return layers_.size(); }
        Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const override;
        const layers_t& layers() const { return layers_; }
        std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo, include_dotcare inc = include_dotcare::no) const override;
        void remove_layers();
        void create_layers(size_t number_of_layers, size_t number_of_antigens);
        void titer(size_t aAntigenNo, size_t aSerumNo, size_t aLayerNo, const std::string& aTiter);
        std::unique_ptr<titer_merge_report> set_from_layers(ChartModify& chart);

        static std::string titer_merge_report_brief(titer_merge data);
        static std::string titer_merge_report_long(titer_merge data);
        static std::string titer_merge_report_description();

     private:
          // size_t number_of_antigens_;
        size_t number_of_sera_;
        titers_t titers_;
        layers_t layers_;
        bool layer_titer_modified_ = false; // force titer recalculation

        static Titer find_titer_for_serum(const sparse_row_t& aRow, size_t aSerumNo);
        static Titer titer_in_sparse_t(const sparse_t& aSparse, size_t aAntigenNo, size_t aSerumNo);

        void set_titer(dense_t& titers, size_t aAntigenNo, size_t aSerumNo, const std::string& aTiter) { titers[aAntigenNo * number_of_sera_ + aSerumNo] = aTiter; }
        void set_titer(sparse_t& titers, size_t aAntigenNo, size_t aSerumNo, const std::string& aTiter);

        enum class more_than_thresholded {adjust_to_next, to_dont_care};

        std::unique_ptr<titer_merge_report> set_titers_from_layers(more_than_thresholded mtt);
        std::pair<Titer, titer_merge> titer_from_layers(size_t ag_no, size_t sr_no, more_than_thresholded mtt, double standard_deviation_threshold) const;

    }; // class TitersModify

// ----------------------------------------------------------------------

    class ColumnBasesModify : public ColumnBasesData
    {
     public:
        explicit ColumnBasesModify(ColumnBasesP aMain) : ColumnBasesData{*aMain} {}
        explicit ColumnBasesModify(const ColumnBases& aSource) : ColumnBasesData{aSource} {}

    }; // class ColumnBasesModify

// ----------------------------------------------------------------------

    class ProjectionModifyNew;

    class ProjectionModify : public Projection
    {
     public:
        enum class randomizer { plain_with_table_max_distance, plain_with_current_layout_area, plain_from_sample_optimization };

        explicit ProjectionModify(const Chart& chart) : Projection(chart) {}
        explicit ProjectionModify(const ProjectionModify& aSource) : Projection(aSource.chart())
            {
                if (aSource.modified()) {
                    layout_ = std::make_shared<acmacs::Layout>(*aSource.layout_modified());
                    transformation_ = aSource.transformation_modified();
                }
            }

        std::string comment() const override { return comment_; }

        std::optional<double> stored_stress() const override { return stress_; }
        void move_point(size_t aPointNo, const PointCoordinates& aCoordinates) { modify(); (*layout_)[aPointNo] = aCoordinates; transformed_layout_.reset(); }
        void rotate_radians(double aAngle) { modify(); transformation_.rotate(aAngle); transformed_layout_.reset(); }
        void rotate_degrees(double aAngle) { rotate_radians(aAngle * M_PI / 180.0); }
        void flip(double aX, double aY) { modify(); transformation_.flip(aX, aY); transformed_layout_.reset(); }
        void flip_east_west() { flip(0, 1); }
        void flip_north_south() { flip(1, 0); }
        void transformation(const Transformation& transformation) { modify(); transformation_ = transformation; transformed_layout_.reset(); }
        using Projection::transformation;
        void set_forced_column_bases(ColumnBasesP aSource) { if (aSource) forced_column_bases_ = std::make_shared<ColumnBasesModify>(aSource); else forced_column_bases_.reset(); }
        void set_forced_column_basis(size_t serum_no, double column_basis);

        std::shared_ptr<acmacs::Layout> layout_modified() { modify(); return layout_; }
        std::shared_ptr<acmacs::Layout> layout_modified() const { return layout_; }
        std::shared_ptr<acmacs::Layout> randomize_layout(std::shared_ptr<LayoutRandomizer> randomizer);
        std::shared_ptr<acmacs::Layout> randomize_layout(randomizer rnd, double diameter_multiplier);
        std::shared_ptr<acmacs::Layout> randomize_layout(const PointIndexList& to_randomize, std::shared_ptr<LayoutRandomizer> randomizer); // randomize just some point coordinates
        virtual void set_layout(const acmacs::Layout& layout, bool allow_size_change = false);
        virtual void set_stress(double stress) { stress_ = stress; }
        virtual void comment(std::string comment) { modify(); comment_ = comment; }
        virtual optimization_status relax(optimization_options options);
        virtual optimization_status relax(optimization_options options, IntermediateLayouts& intermediate_layouts);
        virtual std::shared_ptr<ProjectionModifyNew> clone(ChartModify& chart) const;
        ProcrustesData orient_to(const Projection& master);

        void remove_antigens(const ReverseSortedIndexes& indexes) { modify(); layout_modified()->remove_points(indexes, 0); }
        void remove_sera(const ReverseSortedIndexes& indexes, size_t number_of_antigens);
        void insert_antigen(size_t before) { modify(); layout_modified()->insert_point(before, 0); }
        void insert_serum(size_t before, size_t number_of_antigens);

     protected:
        virtual void modify() { stress_.reset(); transformed_layout_.reset(); }
        virtual bool modified() const { return true; }
        double recalculate_stress() const override { stress_ = Projection::recalculate_stress(); return *stress_; }
        bool layout_present() const { return static_cast<bool>(layout_); }
        void clone_from(const Projection& aSource);
        std::shared_ptr<Layout> transformed_layout_modified() const { if (!transformed_layout_) transformed_layout_ = layout_->transform(transformation_); return transformed_layout_; }
        size_t number_of_points_modified() const { return layout_->number_of_points(); }
        number_of_dimensions_t number_of_dimensions_modified() const { return layout_->number_of_dimensions(); }
        const Transformation& transformation_modified() const { return transformation_; }
        ColumnBasesModifyP forced_column_bases_modified() const { return forced_column_bases_; }
        void new_layout(size_t number_of_points, number_of_dimensions_t number_of_dimensions) { layout_ = std::make_shared<acmacs::Layout>(number_of_points, number_of_dimensions); transformation_.reset(number_of_dimensions); transformed_layout_.reset(); }

     private:
        std::shared_ptr<acmacs::Layout> layout_;
        Transformation transformation_;
        mutable std::shared_ptr<Layout> transformed_layout_;
        mutable std::optional<double> stress_;
        ColumnBasesModifyP forced_column_bases_;
        std::string comment_;

        friend class ProjectionsModify;
        friend class ChartModify; // to set stress_ in ChartModify::relax()

    }; // class ProjectionModify

// ----------------------------------------------------------------------

    class ProjectionModifyMain : public ProjectionModify
    {
     public:
        explicit ProjectionModifyMain(const ChartModify& chart, ProjectionP main) : ProjectionModify(chart), main_{main} {}
        explicit ProjectionModifyMain(const ProjectionModifyMain& aSource) : ProjectionModify(aSource), main_(aSource.main_) {}

        std::optional<double> stored_stress() const override { if (modified()) return ProjectionModify::stored_stress(); else return main_->stored_stress(); } // no stress if projection was modified
        std::shared_ptr<Layout> layout() const override { return modified() ? layout_modified() : main_->layout(); }
        std::shared_ptr<Layout> transformed_layout() const override { return modified() ? transformed_layout_modified() : main_->transformed_layout(); }
        std::string comment() const override { return modified() ? ProjectionModify::comment() : main_->comment(); }
        size_t number_of_points() const override { return modified() ? number_of_points_modified() : main_->number_of_points(); }
        number_of_dimensions_t number_of_dimensions() const override { return modified() ? number_of_dimensions_modified() : main_->number_of_dimensions(); }
        MinimumColumnBasis minimum_column_basis() const override { return main_->minimum_column_basis(); }
        ColumnBasesP forced_column_bases() const override { return modified() ? forced_column_bases_modified() : main_->forced_column_bases(); }
        acmacs::Transformation transformation() const override { return modified() ? transformation_modified() : main_->transformation(); }
        enum dodgy_titer_is_regular dodgy_titer_is_regular() const override { return main_->dodgy_titer_is_regular(); }
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
        explicit ProjectionModifyNew(const Chart& chart, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
            : ProjectionModify(chart), minimum_column_basis_(minimum_column_basis)
            {
                new_layout(chart.number_of_points(), number_of_dimensions);
                set_forced_column_bases(chart.forced_column_bases(minimum_column_basis));
            }

        explicit ProjectionModifyNew(const ProjectionModify& aSource)
            : ProjectionModify(aSource), minimum_column_basis_(aSource.minimum_column_basis()),
              dodgy_titer_is_regular_(aSource.dodgy_titer_is_regular()), stress_diff_to_stop_(aSource.stress_diff_to_stop()),
              disconnected_(aSource.disconnected())
            {
                const auto& source_layout = *aSource.layout();
                new_layout(source_layout.number_of_points(), source_layout.number_of_dimensions());
                set_layout(source_layout);
                set_forced_column_bases(aSource.forced_column_bases());
                comment(aSource.comment());
            }

        std::shared_ptr<Layout> layout() const override { return layout_modified(); }
        std::shared_ptr<Layout> transformed_layout() const override { return transformed_layout_modified(); }
        size_t number_of_points() const override { return number_of_points_modified(); }
        number_of_dimensions_t number_of_dimensions() const override { return number_of_dimensions_modified(); }
        MinimumColumnBasis minimum_column_basis() const override { return minimum_column_basis_; }
        ColumnBasesP forced_column_bases() const override { return forced_column_bases_modified(); }
        using ProjectionModify::transformation;
        acmacs::Transformation transformation() const override { return transformation_modified(); }
        enum dodgy_titer_is_regular dodgy_titer_is_regular() const override { return dodgy_titer_is_regular_; }
        double stress_diff_to_stop() const override { return stress_diff_to_stop_; }
        PointIndexList unmovable() const override { return {}; }
        PointIndexList disconnected() const override { return disconnected_; }
        void set_disconnected(const PointIndexList& disconnect) { disconnected_ = disconnect; }
        void connect(const PointIndexList& to_connect);
        PointIndexList unmovable_in_the_last_dimension() const override { return {}; }
        AvidityAdjusts avidity_adjusts() const override { return {}; }

     private:
        MinimumColumnBasis minimum_column_basis_;
        enum dodgy_titer_is_regular dodgy_titer_is_regular_ = dodgy_titer_is_regular::no;
        double stress_diff_to_stop_{0};
        PointIndexList disconnected_;

    }; // class ProjectionModifyNew

// ----------------------------------------------------------------------

    class ProjectionsModify : public Projections
    {
     public:
        explicit ProjectionsModify(const ChartModify& chart) : Projections(chart) {}
        explicit ProjectionsModify(const ChartModify& chart, ProjectionsP main)
            : Projections(chart), projections_(main->size(), nullptr)
            {
                std::transform(main->begin(), main->end(), projections_.begin(), [&chart](ProjectionP aSource) { return std::make_shared<ProjectionModifyMain>(chart, aSource); });
                set_projection_no();
            }

        bool empty() const override { return projections_.empty(); }
        size_t size() const override { return projections_.size(); }
        ProjectionP operator[](size_t aIndex) const override { return projections_.at(aIndex); }
        ProjectionModifyP at(size_t aIndex) const { return projections_.at(aIndex); }
          // ProjectionModifyP clone(size_t aIndex) { auto cloned = projections_.at(aIndex)->clone(); projections_.push_back(cloned); return cloned; }
        void sort() { std::sort(projections_.begin(), projections_.end(), [](const auto& p1, const auto& p2) { return p1->stress() < p2->stress(); }); set_projection_no(); }
        void add(std::shared_ptr<ProjectionModify> projection);

        std::shared_ptr<ProjectionModifyNew> new_from_scratch(number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis);
        std::shared_ptr<ProjectionModifyNew> new_by_cloning(const ProjectionModify& source, bool add_to_chart = true);

        void keep_just(size_t number_of_projections_to_keep)
            {
                if (projections_.size() > number_of_projections_to_keep)
                    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(number_of_projections_to_keep), projections_.end());
            }

        void remove_all();
        void remove(size_t projection_no);
        void remove_all_except(size_t projection_no);
        void remove_except(size_t number_of_initial_projections_to_keep, ProjectionP projection_to_keep);

        void remove_antigens(const ReverseSortedIndexes& indexes) { for_each(projections_.begin(), projections_.end(), [&](auto& projection) { projection->remove_antigens(indexes); }); }
        void remove_sera(const ReverseSortedIndexes& indexes, size_t number_of_antigens) { for_each(projections_.begin(), projections_.end(), [&indexes,number_of_antigens](auto& projection) { projection->remove_sera(indexes, number_of_antigens); }); }
        void insert_antigen(size_t before) { for_each(projections_.begin(), projections_.end(), [=](auto& projection) { projection->insert_antigen(before); }); }
        void insert_serum(size_t before, size_t number_of_antigens) { for_each(projections_.begin(), projections_.end(), [=](auto& projection) { projection->insert_serum(before, number_of_antigens); }); }

     private:
        std::vector<ProjectionModifyP> projections_;

        void set_projection_no() { std::for_each(acmacs::index_iterator(0UL), acmacs::index_iterator(projections_.size()), [this](auto index) { this->projections_[index]->set_projection_no(index); }); }

        friend class ChartModify;

    }; // class ProjectionsModify

// ----------------------------------------------------------------------

    class PlotSpecModify : public PlotSpec
    {
     public:
        explicit PlotSpecModify(size_t number_of_antigens, size_t number_of_sera);
        explicit PlotSpecModify(PlotSpecP main, size_t number_of_antigens) : main_{main}, number_of_antigens_(number_of_antigens) {}

        bool empty() const override { return modified() ? false : main_->empty(); }
        Color error_line_positive_color() const override { return main_ ? main_->error_line_positive_color() : BLUE; }
        Color error_line_negative_color() const override { return main_ ? main_->error_line_negative_color() : RED; }
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

        void shown(size_t point_no, bool shown) { modify(); validate_point_no(point_no); styles_[point_no].shown = shown; }
        void size(size_t point_no, Pixels size) { modify(); validate_point_no(point_no); styles_[point_no].size = size; }
        void fill(size_t point_no, Color fill) { modify(); validate_point_no(point_no); styles_[point_no].fill = fill; }
        void outline(size_t point_no, Color outline) { modify(); validate_point_no(point_no); styles_[point_no].outline = outline; }
        void outline_width(size_t point_no, Pixels outline_width) { modify(); validate_point_no(point_no); styles_[point_no].outline_width = outline_width; }
        void rotation(size_t point_no, Rotation rotation) { modify(); validate_point_no(point_no); styles_[point_no].rotation = rotation; }
        void aspect(size_t point_no, Aspect aspect) { modify(); validate_point_no(point_no); styles_[point_no].aspect = aspect; }
        void shape(size_t point_no, PointShape::Shape shape) { modify(); validate_point_no(point_no); styles_[point_no].shape = shape; }
        void label_shown(size_t point_no, bool shown) { modify(); validate_point_no(point_no); styles_[point_no].label.shown = shown; }
        void label_offset_x(size_t point_no, double offset) { modify(); validate_point_no(point_no); styles_[point_no].label.offset.set().x(offset); }
        void label_offset_y(size_t point_no, double offset) { modify(); validate_point_no(point_no); styles_[point_no].label.offset.set().y(offset); }
        void label_size(size_t point_no, Pixels size) { modify(); validate_point_no(point_no); styles_[point_no].label.size = size; }
        void label_color(size_t point_no, Color color) { modify(); validate_point_no(point_no); styles_[point_no].label.color = color; }
        void label_rotation(size_t point_no, Rotation rotation) { modify(); validate_point_no(point_no); styles_[point_no].label.rotation = rotation; }
        void label_slant(size_t point_no, FontSlant slant) { modify(); validate_point_no(point_no); styles_[point_no].label.style.slant = slant; }
        void label_weight(size_t point_no, FontWeight weight) { modify(); validate_point_no(point_no); styles_[point_no].label.style.weight = weight; }
        void label_font_family(size_t point_no, std::string font_family) { modify(); validate_point_no(point_no); styles_[point_no].label.style.font_family = font_family; }
        void label_text(size_t point_no, std::string text) { modify(); validate_point_no(point_no); styles_[point_no].label_text = text; }

        void scale_all(double point_scale, double outline_scale) { modify(); std::for_each(styles_.begin(), styles_.end(), [=](auto& style) { style.scale(point_scale).scale_outline(outline_scale); }); }

        void modify(size_t point_no, const PointStyle& style) { modify(); validate_point_no(point_no); styles_[point_no] = style; }
        void modify(const Indexes& points, const PointStyle& style) { modify(); std::for_each(points.begin(), points.end(), [this,&style](size_t index) { this->modify(index, style); }); }
        void modify_serum(size_t serum_no, const PointStyle& style) { modify(serum_no + number_of_antigens_, style); }
        void modify_sera(const Indexes& sera, const PointStyle& style) { std::for_each(sera.begin(), sera.end(), [this,&style](size_t index) { this->modify(index + this->number_of_antigens_, style); }); }

        void remove_antigens(const ReverseSortedIndexes& indexes);
        void remove_sera(const ReverseSortedIndexes& indexes);
        void insert_antigen(size_t before);
        void insert_serum(size_t before);

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
