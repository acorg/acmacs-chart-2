#include "acmacs-chart-2/chart-modify.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

InfoP ChartModify::info() const
{
    return std::make_shared<InfoModify>(mMain->info());

} // ChartModify::info

// ----------------------------------------------------------------------

AntigensP ChartModify::antigens() const
{
    return std::make_shared<AntigensModify>(mMain->antigens());

} // ChartModify::antigens

// ----------------------------------------------------------------------

SeraP ChartModify::sera() const
{
    return std::make_shared<SeraModify>(mMain->sera());

} // ChartModify::sera

// ----------------------------------------------------------------------

TitersP ChartModify::titers() const
{
    return std::make_shared<TitersModify>(mMain->titers());

} // ChartModify::titers

// ----------------------------------------------------------------------

ColumnBasesP ChartModify::forced_column_bases() const
{
    return std::make_shared<ColumnBasesModify>(mMain->forced_column_bases());

} // ChartModify::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP ChartModify::projections() const
{
    return std::make_shared<ProjectionsModify>(mMain->projections());

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    return std::make_shared<PlotSpecModify>(mMain->plot_spec());

} // ChartModify::plot_spec

// ----------------------------------------------------------------------

InfoModifyP ChartModify::info_modify() const
{
    return std::make_shared<InfoModify>(mMain->info());

} // ChartModify::info_modify

// ----------------------------------------------------------------------

AntigensModifyP ChartModify::antigens_modify() const
{
    return std::make_shared<AntigensModify>(mMain->antigens());

} // ChartModify::antigens_modify

// ----------------------------------------------------------------------

SeraModifyP ChartModify::sera_modify() const
{
    return std::make_shared<SeraModify>(mMain->sera());

} // ChartModify::sera_modify

// ----------------------------------------------------------------------

TitersModifyP ChartModify::titers_modify() const
{
    return std::make_shared<TitersModify>(mMain->titers());

} // ChartModify::titers_modify

// ----------------------------------------------------------------------

ColumnBasesModifyP ChartModify::forced_column_bases_modify() const
{
    return std::make_shared<ColumnBasesModify>(mMain->forced_column_bases());

} // ChartModify::forced_column_bases_modify

// ----------------------------------------------------------------------

ProjectionsModifyP ChartModify::projections_modify() const
{
    return std::make_shared<ProjectionsModify>(mMain->projections());

} // ChartModify::projections_modify

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModify::plot_spec_modify() const
{
    return std::make_shared<PlotSpecModify>(mMain->plot_spec());

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
