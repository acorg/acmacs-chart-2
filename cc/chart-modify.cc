#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/chart-modify.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

ChartModify::ChartModify()
    : info_{std::make_shared<InfoModify>()}
{

} // ChartModify::ChartModify

// ----------------------------------------------------------------------

InfoP ChartModify::info() const
{
    if (info_)
        return info_;
    else
        return main_->info();

} // ChartModify::info

// ----------------------------------------------------------------------

InfoModifyP ChartModify::info_modify()
{
    if (!info_)
        info_ = std::make_shared<InfoModify>(main_->info());
    return info_;

} // ChartModify::info_modify

// ----------------------------------------------------------------------

AntigensP ChartModify::antigens() const
{
    if (antigens_)
        return antigens_;
    else
        return main_->antigens();

} // ChartModify::antigens

// ----------------------------------------------------------------------

AntigensModifyP ChartModify::antigens_modify()
{
    if (!antigens_)
        antigens_ = std::make_shared<AntigensModify>(main_->antigens());
    return antigens_;

} // ChartModify::antigens_modify

// ----------------------------------------------------------------------

SeraP ChartModify::sera() const
{
    if (sera_)
        return sera_;
    else
        return main_->sera();

} // ChartModify::sera

// ----------------------------------------------------------------------

SeraModifyP ChartModify::sera_modify()
{
    if (!sera_)
        sera_ = std::make_shared<SeraModify>(main_->sera());
    return sera_;

} // ChartModify::sera_modify

// ----------------------------------------------------------------------

TitersP ChartModify::titers() const
{
    if (titers_)
        return titers_;
    else
        return main_->titers();

} // ChartModify::titers

// ----------------------------------------------------------------------

TitersModifyP ChartModify::titers_modify()
{
    if (!titers_)
        titers_ = std::make_shared<TitersModify>(main_->titers());
    return titers_;

} // ChartModify::titers_modify

// ----------------------------------------------------------------------

ProjectionsP ChartModify::projections() const
{
    if (projections_)
        return projections_;
    else
        return main_->projections();

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    if (plot_spec_)
        return plot_spec_;
    else
        return main_->plot_spec();

} // ChartModify::plot_spec

// ----------------------------------------------------------------------

ColumnBasesP ChartModify::forced_column_bases() const
{
    if (forced_column_bases_)
        return forced_column_bases_;
    else
        return main_->forced_column_bases();

} // ChartModify::forced_column_bases

// ----------------------------------------------------------------------

ColumnBasesModifyP ChartModify::forced_column_bases_modify()
{
    if (!forced_column_bases_) {
        if (auto fcb = main_->forced_column_bases(); fcb)
            forced_column_bases_ = std::make_shared<ColumnBasesModify>(fcb);
    }
    return forced_column_bases_;

} // ChartModify::forced_column_bases_modify

// ----------------------------------------------------------------------

ProjectionsModifyP ChartModify::projections_modify()
{
    if (!projections_)
        projections_ = std::make_shared<ProjectionsModify>(main_->projections());
    return projections_;

} // ChartModify::projections_modify

// ----------------------------------------------------------------------

ProjectionModifyP ChartModify::projection_modify(size_t aProjectionNo)
{
    return projections_modify()->at(aProjectionNo);

} // ChartModify::projection_modify

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModify::plot_spec_modify()
{
    if (!plot_spec_)
        plot_spec_ = std::make_shared<PlotSpecModify>(main_->plot_spec(), number_of_antigens());
    return plot_spec_;

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

std::pair<optimization_status, ProjectionModifyP> ChartModify::relax(MinimumColumnBasis minimum_column_basis, size_t number_of_dimensions, bool dimension_annealing, optimization_options options, const PointIndexList& disconnect_points)
{
    const auto start = std::chrono::high_resolution_clock::now();
    const size_t start_num_dim = dimension_annealing && number_of_dimensions < 5 ? 5 : number_of_dimensions;
    auto projection = projections_modify()->new_from_scratch(start_num_dim, minimum_column_basis);
    projection->set_disconnected(disconnect_points);
    auto layout = projection->layout_modified();
    auto stress = acmacs::chart::stress_factory<double>(*projection, options.mult);
    projection->randomize_layout(options.max_distance_multiplier);
    auto status = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), optimization_precision::rough);
    if (start_num_dim > number_of_dimensions) {
        acmacs::chart::dimension_annealing(options.method, projection->number_of_dimensions(), number_of_dimensions, layout->data(), layout->data() + layout->size());
        layout->change_number_of_dimensions(number_of_dimensions);
        stress.change_number_of_dimensions(number_of_dimensions);
        const auto status2 = acmacs::chart::optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
        status.number_of_iterations += status2.number_of_iterations;
        status.number_of_stress_calculations += status2.number_of_stress_calculations;
        status.termination_report = status2.termination_report;
        status.initial_stress = status2.initial_stress;
        status.final_stress = status2.final_stress;
    }
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return {status, projection};

} // ChartModify::relax

// ----------------------------------------------------------------------

InfoModify::InfoModify(InfoP main)
    : name_{main->name(Compute::No)},
      computed_name_{main->name(Compute::Yes)},
      virus_{main->virus(Compute::Yes)},
      virus_type_{main->virus_type(Compute::Yes)},
      subset_{main->subset(Compute::Yes)},
      assay_{main->assay(Compute::Yes)},
      lab_{main->lab(Compute::Yes)},
      rbc_species_{main->rbc_species(Compute::Yes)},
      date_{main->date(Compute::Yes)}
{
} // InfoModify::InfoModify

// ----------------------------------------------------------------------

AntigenModify::AntigenModify(AntigenP main)
    :
    name_{main->name()},
    date_{main->date()},
    passage_{main->passage()},
    lineage_{main->lineage()},
    reassortant_{main->reassortant()},
    annotations_{main->annotations()},
    lab_ids_{main->lab_ids()},
    clades_{main->clades()},
    reference_{main->reference()}
{
} // AntigenModify::AntigenModify

// ----------------------------------------------------------------------

SerumModify::SerumModify(SerumP main)
    :
    name_{main->name()},
    passage_{main->passage()},
    lineage_{main->lineage()},
    reassortant_{main->reassortant()},
    annotations_{main->annotations()},
    serum_id_{main->serum_id()},
    serum_species_{main->serum_species()},
    homologous_antigens_{main->homologous_antigens()}
{
} // SerumModify::SerumModify

// ----------------------------------------------------------------------

AntigensModify::AntigensModify(AntigensP main)
    : antigens_(main->size(), nullptr)
{
    std::transform(main->begin(), main->end(), antigens_.begin(), [](auto ag) { return std::make_shared<AntigenModify>(ag); });

} // AntigensModify::AntigensModify

// ----------------------------------------------------------------------

SeraModify::SeraModify(SeraP main)
    : sera_(main->size(), nullptr)
{
    std::transform(main->begin(), main->end(), sera_.begin(), [](auto sr) { return std::make_shared<SerumModify>(sr); });

} // SeraModify::SeraModify

// ----------------------------------------------------------------------

TitersModify::TitersModify()
    : titers_{dense_t{}}
{
} // TitersModify::TitersModify

// ----------------------------------------------------------------------

TitersModify::TitersModify(TitersP main)
    : number_of_sera_{main->number_of_sera()},
      titers_{main->is_dense() ? titers_t{dense_t{}} : titers_t{sparse_t{}}}
{
      // Titers

    auto fill_titers = [this, &main](auto &titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
              // Dense ==================================================
            titers.resize(main->number_of_antigens() * this->number_of_sera_);
            for (const auto &entry : *main) { // order of iterations is not specified!
                // std::cerr << entry.antigen << ' ' << entry.serum << ' ' << entry.titer << '\n';
                titers[entry.antigen * this->number_of_sera_ + entry.serum] = entry.titer;
            }
        }
        else { // Sparse ==================================================
            titers.resize(main->number_of_antigens());
            for (const auto &entry : *main) { // order of iterations is not specified!
                titers[entry.antigen].emplace_back(entry.serum, entry.titer);
            }
              // sort entries by serum no
            for (auto& row : titers)
                std::sort(row.begin(), row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
        }
    };

    std::visit(fill_titers, titers_);

      // Layers
    if (main->number_of_layers() > 0) {
        layers_.resize(main->number_of_layers());
        try {
            auto source = main->rjson_layers();
            for (const auto [layer_no, source_layer_v] : acmacs::enumerate(source)) {
                const rjson::array& source_layer = source_layer_v;
                auto& target = layers_[layer_no];
                target.resize(source_layer.size());
                for (const auto [ag_no, source_row] : acmacs::enumerate(source_layer)) {
                    for (const auto& entry : static_cast<const rjson::object&>(source_row)) {
                        target[ag_no].emplace_back(std::stoul(entry.first.str()), entry.second);
                    }
                }
                  // sort entries by serum no
                for (auto& row : target)
                    std::sort(row.begin(), row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
            }
        }
        catch (data_not_available&) {
            for (size_t layer_no = 0; layer_no < main->number_of_layers(); ++layer_no) {
                auto& target = layers_[layer_no];
                target.resize(main->number_of_antigens());
                for (auto ag_no : acmacs::range(target.size())) {
                    for (auto sr_no : acmacs::range(number_of_sera_))
                        if (const auto titer = main->titer_of_layer(layer_no, ag_no, sr_no); !titer.is_dont_care() && !titer.is_invalid())
                            target[ag_no].emplace_back(sr_no, titer);
                }
            }
        }
    }

} // TitersModify::TitersModify

// ----------------------------------------------------------------------

inline acmacs::chart::Titer TitersModify::find_titer_for_serum(const sparse_row_t& aRow, size_t aSerumNo)
{
    if (aRow.empty())
        return {};
    if (const auto found = std::lower_bound(aRow.begin(), aRow.end(), aSerumNo,
                                            [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != aRow.end() && found->first == aSerumNo)
        return found->second;
    return {};

} // TitersModify::find_titer_for_serum

// ----------------------------------------------------------------------

inline acmacs::chart::Titer TitersModify::titer_in_sparse_t(const sparse_t& aSparse, size_t aAntigenNo, size_t aSerumNo)
{
    return find_titer_for_serum(aSparse[aAntigenNo], aSerumNo);

} // TitersModify::titer_in_sparse_t

// ----------------------------------------------------------------------

Titer TitersModify::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    auto get = [this,aAntigenNo,aSerumNo](const auto& titers) -> Titer {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers[aAntigenNo * this->number_of_sera_ + aSerumNo];
        else
            return titer_in_sparse_t(titers, aAntigenNo, aSerumNo);
    };
    return std::visit(get, titers_);

} // TitersModify::titer

// ----------------------------------------------------------------------

Titer TitersModify::titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const
{
    return titer_in_sparse_t(layers_[aLayerNo], aAntigenNo, aSerumNo);

} // TitersModify::titer_of_layer

// ----------------------------------------------------------------------

std::vector<Titer> TitersModify::titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const
{
    if (layers_.empty())
        throw acmacs::chart::data_not_available("no layers");
    std::vector<Titer> result;
    for (const auto& layer: layers_) {
        if (const auto titer = find_titer_for_serum(layer[aAntigenNo], aSerumNo); !titer.is_dont_care())
            result.push_back(titer);
    }
    return result;

} // TitersModify::titers_for_layers

// ----------------------------------------------------------------------

size_t TitersModify::number_of_antigens() const
{
    auto num_ags = [this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers.size() / this->number_of_sera_;
        else
            return titers.size();
    };
    return std::visit(num_ags, titers_);

} // TitersModify::number_of_antigens

// ----------------------------------------------------------------------

size_t TitersModify::number_of_non_dont_cares() const
{
    auto num_non_dont_cares = [](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& titer) -> size_t { return a + (titer.is_dont_care() ? size_t{0} : size_t{1}); });
        else
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& row) -> size_t { return a + row.size(); });
    };
    return std::visit(num_non_dont_cares, titers_);

} // TitersModify::number_of_non_dont_cares

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionModify::clone(ChartModify& chart) const
{
    auto projection = std::make_shared<ProjectionModifyNew>(*this);
    chart.projections_modify()->add(projection);
    return projection;

} // ProjectionModify::clone

// ----------------------------------------------------------------------

void ProjectionsModify::add(std::shared_ptr<ProjectionModify> projection)
{
    projections_.push_back(projection);
    projections_.back()->set_projection_no(projections_.size() - 1);

} // ProjectionsModify::add

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionsModify::new_from_scratch(size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
{
    auto projection = std::make_shared<ProjectionModifyNew>(chart(), number_of_dimensions, minimum_column_basis);
    add(projection);
    return projection;

} // ProjectionsModify::new_from_scratch

// ----------------------------------------------------------------------

void ProjectionsModify::remove(size_t projection_no)
{
    if (projection_no >= projections_.size())
        throw invalid_data{"invalid projection number: " + std::to_string(projection_no)};
    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no));
    set_projection_no();

} // ProjectionsModify::remove

// ----------------------------------------------------------------------

void ProjectionsModify::remove_all_except(size_t projection_no)
{
    if (projection_no >= projections_.size())
        throw invalid_data{"invalid projection number: " + std::to_string(projection_no)};
    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no + 1), projections_.end());
    projections_.erase(projections_.begin(), projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no));
    set_projection_no();

} // ProjectionsModify::remove_all_except

// ----------------------------------------------------------------------

void ProjectionModify::randomize_layout(LayoutRandomizer& randomizer)
{
    modify();
    auto layout = layout_modified();
    for (size_t point_no = 0; point_no < layout->number_of_points(); ++point_no) {
        for (size_t dim_no = 0; dim_no < layout->number_of_dimensions(); ++dim_no)
            layout->set(point_no, dim_no, randomizer());
    }

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

void ProjectionModify::randomize_layout(const PointIndexList& to_randomize, LayoutRandomizer& randomizer)
{
    modify();
    auto layout = layout_modified();
    for (auto point_no : to_randomize) {
        for (size_t dim_no = 0; dim_no < layout->number_of_dimensions(); ++dim_no)
            layout->set(point_no, dim_no, randomizer());
    }

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

LayoutRandomizerPlain ProjectionModify::make_randomizer_plain(double max_distance_multiplier) const
{
    auto cb = forced_column_bases();
    if (!cb)
        cb = chart().column_bases(minimum_column_basis());
    return LayoutRandomizerPlain(chart().titers()->max_distance(*cb) * max_distance_multiplier);

} // ProjectionModify::make_randomizer_plain

// ----------------------------------------------------------------------

void ProjectionModify::set_layout(const acmacs::Layout& layout, bool allow_size_change)
{
    modify();
    auto target_layout = layout_modified();
    if (!allow_size_change && layout.size() != target_layout->size())
        throw invalid_data("ProjectionModify::set_layout(const acmacs::Layout&): wrong layout size");
    *target_layout = layout;

} // ProjectionModify::set_layout

// ----------------------------------------------------------------------

void ProjectionModify::set_layout(const acmacs::LayoutInterface& layout)
{
    modify();
    new_layout(layout.number_of_points(), layout.number_of_dimensions());
    for (auto point_no : acmacs::range(layout.number_of_points()))
        layout_->set(point_no, layout.get(point_no));

} // ProjectionModify::set_layout

// ----------------------------------------------------------------------

void ProjectionModifyNew::connect(const PointIndexList& to_connect)
{
    for (auto point_no: to_connect) {
        const auto found = std::find(disconnected_.begin(), disconnected_.end(), point_no);
        if (found == disconnected_.end())
            throw invalid_data{"Point was not disconnected: " + std::to_string(point_no) + ": cannot connect it"};
        disconnected_.erase(found);
    }

} // ProjectionModifyNew::connect

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
