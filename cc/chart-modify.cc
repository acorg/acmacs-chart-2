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
    return std::make_shared<ProjectionsModify>(mMain->projections(), const_cast<ChartModify&>(*this));

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    return std::make_shared<PlotSpecModify>(mMain->plot_spec());

} // ChartModify::plot_spec

// ----------------------------------------------------------------------

InfoModifyP ChartModify::info_modify()
{
    return std::make_shared<InfoModify>(mMain->info());

} // ChartModify::info_modify

// ----------------------------------------------------------------------

AntigensModifyP ChartModify::antigens_modify()
{
    return std::make_shared<AntigensModify>(mMain->antigens());

} // ChartModify::antigens_modify

// ----------------------------------------------------------------------

SeraModifyP ChartModify::sera_modify()
{
    return std::make_shared<SeraModify>(mMain->sera());

} // ChartModify::sera_modify

// ----------------------------------------------------------------------

TitersModifyP ChartModify::titers_modify()
{
    return std::make_shared<TitersModify>(mMain->titers());

} // ChartModify::titers_modify

// ----------------------------------------------------------------------

ColumnBasesModifyP ChartModify::forced_column_bases_modify()
{
    return std::make_shared<ColumnBasesModify>(mMain->forced_column_bases());

} // ChartModify::forced_column_bases_modify

// ----------------------------------------------------------------------

ProjectionsModifyP ChartModify::projections_modify()
{
    return std::make_shared<ProjectionsModify>(mMain->projections(), *this);

} // ChartModify::projections_modify

// ----------------------------------------------------------------------

ProjectionModifyP ChartModify::projection_modify(size_t aProjectionNo)
{
    return std::make_shared<ProjectionModify>(mMain->projections()->operator[](aProjectionNo), aProjectionNo, *this);

} // ChartModify::projection_modify

// ----------------------------------------------------------------------

PlotSpecModifyP ChartModify::plot_spec_modify()
{
    return std::make_shared<PlotSpecModify>(mMain->plot_spec());

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

internal::ProjectionModifyData& ChartModify::projection_modify_data(size_t aProjectionNo)
{
    if (const auto found = mProjectionModifyData.find(aProjectionNo); found == mProjectionModifyData.end()) {
        mProjectionModifyData[aProjectionNo] = std::make_unique<internal::ProjectionModifyData>(mMain->projections()->operator[](aProjectionNo));
        return *mProjectionModifyData[aProjectionNo];
    }
    else
        return *found->second;

} // ChartModify::projection_modify_data

// ----------------------------------------------------------------------

acmacs::chart::internal::Layout::Layout(const acmacs::chart::Layout& aSource)
    : mData(aSource.number_of_points())
{
    for (size_t point_no = 0; point_no < aSource.number_of_points(); ++point_no)
        set(point_no, aSource[point_no]);

} // acmacs::chart::internal::Layout::Layout

// ----------------------------------------------------------------------

acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData(ProjectionP aMain)
    : mLayout(std::make_shared<acmacs::chart::internal::Layout>(*aMain->layout())), mTransformation(aMain->transformation())
{

} // acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
