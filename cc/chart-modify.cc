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
    return std::make_shared<PlotSpecModify>(mMain->plot_spec(), *this);

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

internal::ProjectionModifyData& ChartModify::modify_projection(size_t aProjectionNo)
{
    if (const auto found = mProjectionModifyData.find(aProjectionNo); found == mProjectionModifyData.end()) {
        mProjectionModifyData[aProjectionNo] = std::make_unique<internal::ProjectionModifyData>(mMain->projections()->operator[](aProjectionNo));
        return *mProjectionModifyData[aProjectionNo];
    }
    else
        return *found->second;

} // ChartModify::modify_projection

// ----------------------------------------------------------------------

internal::PlotSpecModifyData& ChartModify::modify_plot_spec()
{
    if (!mPlotSpecModifyData) {
        mPlotSpecModifyData = internal::PlotSpecModifyData(mMain->plot_spec());
    }
    return *mPlotSpecModifyData;

} // ChartModify::modify_plot_spec

// ----------------------------------------------------------------------

acmacs::chart::internal::Layout::Layout(const acmacs::chart::Layout& aSource)
    : mData(aSource.number_of_points())
{
    for (size_t point_no = 0; point_no < aSource.number_of_points(); ++point_no)
        mData[point_no] = aSource[point_no];

} // acmacs::chart::internal::Layout::Layout

// ----------------------------------------------------------------------

void acmacs::chart::internal::Layout::set(size_t aPointNo, const Coordinates& aCoordinates)
{
    // std::cerr << *this << '\n';
    if (number_of_dimensions() != aCoordinates.size())
        throw std::runtime_error{"Wrong number of dimensions (" + acmacs::to_string(aCoordinates.size()) + ") in acmacs::chart::internal::Layout::set(), expected: " + acmacs::to_string(number_of_dimensions())};
    mData[aPointNo] = aCoordinates;

} // acmacs::chart::internal::Layout::set

// ----------------------------------------------------------------------

acmacs::chart::DrawingOrder acmacs::chart::PlotSpecModify::drawing_order() const
{
    if (mChart.modified_plot_spec())
        return mChart.modify_plot_spec().drawing_order();
    auto drawing_order = mMain->drawing_order();
    drawing_order.fill_if_empty(mChart.number_of_points());
    return drawing_order;

} // acmacs::chart::PlotSpecModify::drawing_order

// ----------------------------------------------------------------------

acmacs::chart::internal::ProjectionModifyData::ProjectionModifyData(ProjectionP aMain)
    : mLayout(std::make_shared<acmacs::chart::internal::Layout>(*aMain->layout())), mTransformation(aMain->transformation())
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
