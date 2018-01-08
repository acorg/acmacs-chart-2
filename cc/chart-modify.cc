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

PlotSpecP ChartModify::plot_spec() const
{
    return std::make_shared<PlotSpecModify>(main_->plot_spec(), const_cast<ChartModify&>(*this));

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
    return std::make_shared<PlotSpecModify>(main_->plot_spec(), *this);

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

internal::PlotSpecModifyData& ChartModify::modify_plot_spec()
{
    if (!mPlotSpecModifyData) {
        mPlotSpecModifyData = internal::PlotSpecModifyData(main_->plot_spec());
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

acmacs::chart::internal::PlotSpecModifyData::PlotSpecModifyData(PlotSpecP aMain)
    : mStyles(aMain->all_styles()), mDrawingOrder(aMain->drawing_order())
{
    mDrawingOrder.fill_if_empty(number_of_points());

} // acmacs::chart::internal::PlotSpecModifyData::PlotSpecModifyData

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
