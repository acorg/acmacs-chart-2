#include "acmacs-chart-2/chart-modify.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

InfoP ChartModify::info() const
{
    return std::make_shared<InfoModify>(main_->info());

} // ChartModify::info

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

inline ProjectionsModifyP ChartModify::get_projections() const
{
    if (!projections_)
        projections_ = std::make_shared<ProjectionsModify>(main_->projections());
    return projections_;

} // ChartModify::get_projections

// ----------------------------------------------------------------------

ProjectionsP ChartModify::projections() const
{
    return get_projections();

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModify::get_plot_spec() const
{
    if (!plot_spec_)
        plot_spec_ = std::make_shared<PlotSpecModify>(main_->plot_spec(), number_of_antigens());
    return plot_spec_;

} // ChartModify::get_plot_spec

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    return get_plot_spec();

} // ChartModify::plot_spec

// ----------------------------------------------------------------------

InfoModifyP ChartModify::info_modify()
{
    return std::make_shared<InfoModify>(main_->info());

} // ChartModify::info_modify

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

ProjectionsModifyP ChartModify::projections_modify()
{
    return get_projections();

} // ChartModify::projections_modify

// ----------------------------------------------------------------------

ProjectionModifyP ChartModify::projection_modify(size_t aProjectionNo)
{
    return get_projections()->at(aProjectionNo);

} // ChartModify::projection_modify

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModify::plot_spec_modify()
{
    return get_plot_spec();

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

void ProjectionModify::randomize_layout(double max_distance_multiplier)
{
    auto cb = forced_column_bases();
    if (!cb)
        cb = chart().column_bases(minimum_column_basis());
    LayoutRandomizerPlain randomizer(chart().titers()->max_distance(*cb) * max_distance_multiplier);
    randomize_layout(randomizer);

} // ProjectionModify::randomize_layout

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
