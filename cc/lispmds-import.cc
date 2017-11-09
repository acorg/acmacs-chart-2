#include <set>
#include <vector>
#include <limits>
#include <regex>

#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart/lispmds-import.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

std::shared_ptr<Chart> acmacs::chart::lispmds_import(const std::string_view& aData, Verify aVerify)
{
    try {
        auto chart = std::make_shared<LispmdsChart>(acmacs::lispmds::parse_string(aData));
        chart->verify_data(aVerify);
        return chart;
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        throw;
    }

} // acmacs::chart::lispmds_import

// ----------------------------------------------------------------------

void LispmdsChart::verify_data(Verify) const
{
    try {
    }
    catch (std::exception& err) {
        throw import_error("[lispmds]: structure verification failed: "s + err.what());
    }

} // LispmdsChart::verify_data

// ----------------------------------------------------------------------

std::shared_ptr<Info> LispmdsChart::info() const
{
    return std::make_shared<LispmdsInfo>(mData);

} // LispmdsChart::info

// ----------------------------------------------------------------------

std::shared_ptr<Antigens> LispmdsChart::antigens() const
{
    return std::make_shared<LispmdsAntigens>(mData);

} // LispmdsChart::antigens

// ----------------------------------------------------------------------

std::shared_ptr<Sera> LispmdsChart::sera() const
{
    return std::make_shared<LispmdsSera>(mData);

} // LispmdsChart::sera

// ----------------------------------------------------------------------

std::shared_ptr<Titers> LispmdsChart::titers() const
{
    return std::make_shared<LispmdsTiters>(mData);

} // LispmdsChart::titers

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> LispmdsChart::forced_column_bases() const
{
    try {
        const auto number_of_antigens = mData[0][1].size();
        const auto number_of_sera = mData[0][2].size();
        return std::make_shared<LispmdsForcedColumnBases>(mData[":STARTING-COORDSS"][number_of_antigens][0][1], number_of_antigens, number_of_sera);
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return std::make_shared<LispmdsNoColumnBases>();
    }

} // LispmdsChart::forced_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<Projections> LispmdsChart::projections() const
{
    return std::make_shared<LispmdsProjections>(mData);

} // LispmdsChart::projections

// ----------------------------------------------------------------------

std::shared_ptr<PlotSpec> LispmdsChart::plot_spec() const
{
    return std::make_shared<LispmdsPlotSpec>(mData);

} // LispmdsChart::plot_spec

// ----------------------------------------------------------------------

std::string LispmdsInfo::name(Compute) const
{
    if (mData[0].size() >= 5)
        return std::get<acmacs::lispmds::symbol>(mData[0][4]);
    else
        return {};

} // LispmdsInfo::name

// ----------------------------------------------------------------------

Name LispmdsAntigen::name() const
{
    return static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][1][mIndex]));

} // LispmdsAntigen::name

Name LispmdsSerum::name() const
{
    return static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][2][mIndex]));

} // LispmdsSerum::name

// ----------------------------------------------------------------------

bool LispmdsAntigen::reference() const
{
    try {
        const auto& val = mData[":REFERENCE-ANTIGENS"];
        if (val.empty())
            return false;
        const auto& name = std::get<acmacs::lispmds::symbol>(mData[0][1][mIndex]);
        const auto& val_l = std::get<acmacs::lispmds::list>(val);
        return std::find_if(val_l.begin(), val_l.end(), [&name](const auto& ev) -> bool { return std::get<acmacs::lispmds::symbol>(ev) == name; }) != val_l.end();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return false;
    }

} // LispmdsAntigen::reference

// ----------------------------------------------------------------------

size_t LispmdsAntigens::size() const
{
    return mData[0][1].size();

} // LispmdsAntigens::size

// ----------------------------------------------------------------------

std::shared_ptr<Antigen> LispmdsAntigens::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsAntigen>(mData, aIndex);

} // LispmdsAntigens::operator[]

// ----------------------------------------------------------------------

size_t LispmdsSera::size() const
{
    return mData[0][2].size();

} // LispmdsSera::size

// ----------------------------------------------------------------------

std::shared_ptr<Serum> LispmdsSera::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsSerum>(mData, aIndex);

} // LispmdsSera::operator[]

// ----------------------------------------------------------------------

Titer LispmdsTiters::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    const auto& titer_v = mData[0][3][aAntigenNo][aSerumNo];
    std::string prefix;
    double titer = 0;
    try {
        titer = std::get<acmacs::lispmds::number>(titer_v);
    }
    catch (std::bad_variant_access&) {
        const std::string sym = std::get<acmacs::lispmds::symbol>(titer_v);
        prefix.append(1, sym[0]);
        if (prefix == "*")
            return prefix;
        titer = std::stod(sym.substr(1));
    }
    return prefix + acmacs::to_string(std::lround(std::exp2(titer) * 10));

} // LispmdsTiters::titer

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_antigens() const
{
    return mData[0][3].size();

} // LispmdsTiters::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_sera() const
{
    return mData[0][3][0].size();

} // LispmdsTiters::number_of_sera

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    for (const auto& row: std::get<acmacs::lispmds::list>(mData[0][3])) {
        for (const auto& titer: std::get<acmacs::lispmds::list>(row)) {
            try {
                if (static_cast<std::string>(std::get<acmacs::lispmds::symbol>(titer))[0] != '*')
                    ++result;
            }
            catch (std::bad_variant_access&) {
                ++result;
            }
        }
    }
    return result;

} // LispmdsTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

double LispmdsForcedColumnBases::column_basis(size_t aSerumNo) const
{
    std::cerr << "column_basis: " << mData << '\n';

    return std::get<acmacs::lispmds::number>(mData[mNumberOfAntigens + aSerumNo]);

} // LispmdsForcedColumnBases::column_basis

// ----------------------------------------------------------------------

double LispmdsProjection::stress() const
{
    if (mIndex == 0)
        return 0;
    const auto& batch_runs = mData[":BATCH-RUNS"];
    return std::get<acmacs::lispmds::number>(batch_runs[mIndex - 1][1]);

} // LispmdsProjection::stress

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_points() const
{

} // LispmdsProjection::number_of_points

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_dimensions() const
{
    const auto& point0 = mIndex == 0 ? mData[":STARTING-COORDSS"][0] : mData[":BATCH-RUNS"][mIndex - 1][0];
    return point0.size();

} // LispmdsProjection::number_of_dimensions

// ----------------------------------------------------------------------

double LispmdsProjection::coordinate(size_t aPointNo, size_t aDimensionNo) const
{

} // LispmdsProjection::coordinate

// ----------------------------------------------------------------------

std::shared_ptr<ForcedColumnBases> LispmdsProjection::forced_column_bases() const
{

} // LispmdsProjection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::Transformation LispmdsProjection::transformation() const
{
    acmacs::Transformation result;
    return result;

} // LispmdsProjection::transformation

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::unmovable() const
{
    return {};

} // LispmdsProjection::unmovable

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::disconnected() const
{
    return {};

} // LispmdsProjection::disconnected

// ----------------------------------------------------------------------

bool LispmdsProjections::empty() const
{
    try {
        const auto& val = mData[":STARTING-COORDSS"];
        return val.empty();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return true;
    }

} // LispmdsProjections::empty

// ----------------------------------------------------------------------

size_t LispmdsProjections::size() const
{
    size_t result = 0;
    // try {
    //     const auto& starting_coordss = mData[":STARTING-COORDSS"];
    //     if (!starting_coordss.empty())
    //         ++result;
    //     const auto& batch_runs = mData[":BATCH-RUNS"];
    //     result += batch_runs.size();
    // }
    // catch (acmacs::lispmds::keyword_no_found&) {
    // }
    return result;

} // LispmdsProjections::size

// ----------------------------------------------------------------------

std::shared_ptr<Projection> LispmdsProjections::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsProjection>(mData, aIndex);

} // LispmdsProjections::operator[]

// ----------------------------------------------------------------------

bool LispmdsPlotSpec::empty() const
{
    return true;

} // LispmdsPlotSpec::empty

// ----------------------------------------------------------------------

DrawingOrder LispmdsPlotSpec::drawing_order() const
{
    DrawingOrder result;
    return result;

} // LispmdsPlotSpec::drawing_order

// ----------------------------------------------------------------------

Color LispmdsPlotSpec::error_line_positive_color() const
{
    return "red";

} // LispmdsPlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color LispmdsPlotSpec::error_line_negative_color() const
{
    return "blue";

} // LispmdsPlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle LispmdsPlotSpec::style(size_t aPointNo) const
{
    acmacs::PointStyle result;
    return result;

} // LispmdsPlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> LispmdsPlotSpec::all_styles() const
{
    return {};

} // LispmdsPlotSpec::all_styles

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
