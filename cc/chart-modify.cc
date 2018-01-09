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

ProjectionModifyP ChartModify::new_projection(size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
{
    return get_projections()->new_from_scratch(*this, number_of_points(), number_of_dimensions, minimum_column_basis, forced_column_bases());

} // ChartModify::new_projection

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
