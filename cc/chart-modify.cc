#include "acmacs-base/range.hh"
#include "acmacs-chart-2/chart-modify.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

ProjectionsP ChartModifyBase::projections() const
{
    return get_projections();

} // ChartModifyBase::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModifyBase::plot_spec() const
{
    return get_plot_spec();

} // ChartModifyBase::plot_spec

// ----------------------------------------------------------------------

ProjectionModifyP ChartModifyBase::projection_modify(size_t aProjectionNo)
{
    return get_projections()->at(aProjectionNo);

} // ChartModifyBase::projection_modify

// ----------------------------------------------------------------------

ProjectionsModifyP ChartModifyBase::get_projections() const
{
    if (!projections_)
        new_projections();
    return projections_;

} // ChartModifyBase::get_projections

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModifyBase::get_plot_spec() const
{
    if (!plot_spec_)
        new_plot_spec();
    return plot_spec_;

} // ChartModifyBase::get_plot_spec

// ----------------------------------------------------------------------
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
        info_ = std::make_shared<InfoModifyMain>(main_->info());
    return info_;

} // ChartModify::info_modify

// ----------------------------------------------------------------------

AntigensP ChartModify::antigens() const
{
    return std::make_shared<AntigensModify>(main_->antigens());

} // ChartModify::antigens

// ----------------------------------------------------------------------

SeraP ChartModify::sera() const
{
    return std::make_shared<SeraModify>(main_->sera());

} // ChartModify::sera

// ----------------------------------------------------------------------

TitersP ChartModify::titers() const
{
    return std::make_shared<TitersModify>(main_->titers());

} // ChartModify::titers

// ----------------------------------------------------------------------

ColumnBasesP ChartModify::forced_column_bases() const
{
    if (auto cb = main_->forced_column_bases(); cb)
        return std::make_shared<ColumnBasesModify>(cb);
    else
        return nullptr;

} // ChartModify::forced_column_bases

// ----------------------------------------------------------------------

void ChartModify::new_projections() const
{
    projections_ = std::make_shared<ProjectionsModify>(main_->projections());

} // ChartModify::new_projections

// ----------------------------------------------------------------------

void ChartModify::new_plot_spec() const
{
    plot_spec_ = std::make_shared<PlotSpecModify>(main_->plot_spec(), number_of_antigens());

} // ChartModify::new_plot_spec

// ----------------------------------------------------------------------

AntigensModifyP ChartModify::antigens_modify()
{
    return std::make_shared<AntigensModify>(main_->antigens());

} // ChartModify::antigens_modify

// ----------------------------------------------------------------------

SeraModifyP ChartModify::sera_modify()
{
    return std::make_shared<SeraModify>(main_->sera());

} // ChartModify::sera_modify

// ----------------------------------------------------------------------

TitersModifyP ChartModify::titers_modify()
{
    return std::make_shared<TitersModify>(main_->titers());

} // ChartModify::titers_modify

// ----------------------------------------------------------------------

ColumnBasesModifyP ChartModify::forced_column_bases_modify()
{
    if (auto cb = main_->forced_column_bases(); cb)
        return std::make_shared<ColumnBasesModify>(cb);
    else
        return nullptr;

} // ChartModify::forced_column_bases_modify

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
