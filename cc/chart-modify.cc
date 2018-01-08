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
    if (auto cb = mMain->forced_column_bases(); cb)
        return std::make_shared<ColumnBasesModify>(cb);
    else
        return nullptr;

} // ChartModify::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP ChartModify::projections() const
{
    return std::make_shared<ProjectionsModify>(mMain->projections(), const_cast<ChartModify&>(*this));

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    return std::make_shared<PlotSpecModify>(mMain->plot_spec(), const_cast<ChartModify&>(*this));

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
    if (auto cb = mMain->forced_column_bases(); cb)
        return std::make_shared<ColumnBasesModify>(cb);
    else
        return nullptr;

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
    return std::make_shared<PlotSpecModify>(mMain->plot_spec(), *this);

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

internal::ProjectionModifyData& ChartModify::modify_projection(ProjectionId aProjectionId)
{
    if (const auto found = mProjectionModifyData.find(aProjectionId); found == mProjectionModifyData.end()) {
        mProjectionModifyData[aProjectionId] = std::make_unique<internal::ProjectionModifyData>(mMain->projections()->operator[](aProjectionId));
        return *mProjectionModifyData[aProjectionId];
    }
    else
        return *found->second;

} // ChartModify::modify_projection

// ----------------------------------------------------------------------

void ChartModify::clone_modified_projection(ProjectionId old_id, ProjectionId new_id)
{
    if (const auto found_new = mProjectionModifyData.find(new_id); found_new != mProjectionModifyData.end())
        throw invalid_data("ChartModify::clone_modified_projection: new_id already exists");
    if (const auto found_old = mProjectionModifyData.find(old_id); found_old != mProjectionModifyData.end()) {
        mProjectionModifyData[new_id] = std::make_unique<internal::ProjectionModifyData>(*found_old->second);
    }
    else {
        throw invalid_data("ChartModify::clone_modified_projection: old_id does not exist");
    }

} // ChartModify::clone_modified_projection

// ----------------------------------------------------------------------

internal::PlotSpecModifyData& ChartModify::modify_plot_spec()
{
    if (!mPlotSpecModifyData) {
        mPlotSpecModifyData = internal::PlotSpecModifyData(mMain->plot_spec());
    }
    return *mPlotSpecModifyData;

} // ChartModify::modify_plot_spec

// ----------------------------------------------------------------------

acmacs::chart::DrawingOrder acmacs::chart::PlotSpecModify::drawing_order() const
{
    if (mChart.modified_plot_spec()) {
        return mChart.modify_plot_spec().drawing_order();
    }
    else {
        auto drawing_order = mMain->drawing_order();
        drawing_order.fill_if_empty(mChart.number_of_points());
        return drawing_order;
    }

} // acmacs::chart::PlotSpecModify::drawing_order

// ----------------------------------------------------------------------

acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData(ProjectionP aMain)
    : mLayout(std::make_shared<acmacs::Layout>(*aMain->layout())), mTransformation(aMain->transformation())
{
} // acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData

// ----------------------------------------------------------------------

acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData(const acmacs::chart::internal::ProjectionModifyData& aSource)
    : mLayout(std::make_shared<acmacs::Layout>(aSource.clayout())), mTransformation(aSource.transformation())
{
} // acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData

// ----------------------------------------------------------------------

acmacs::chart::internal::PlotSpecModifyData::PlotSpecModifyData(PlotSpecP aMain)
    : mStyles(aMain->all_styles()), mDrawingOrder(aMain->drawing_order())
{
    mDrawingOrder.fill_if_empty(number_of_points());

} // acmacs::chart::internal::PlotSpecModifyData::PlotSpecModifyData

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
