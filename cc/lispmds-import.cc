#include <set>
#include <vector>
#include <limits>
#include <regex>
#include <cmath>

#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/lispmds-import.hh"
#include "acmacs-chart-2/lispmds-encode.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------


static std::vector<double> native_column_bases(const acmacs::lispmds::value& aData);
static std::vector<double> column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo);
static std::pair<std::shared_ptr<acmacs::chart::ColumnBases>, acmacs::chart::MinimumColumnBasis> forced_column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo);

// ----------------------------------------------------------------------

static inline size_t number_of_antigens(const acmacs::lispmds::value& aData)
{
    return aData[0][1].size();

} // number_of_antigens

static inline size_t number_of_sera(const acmacs::lispmds::value& aData)
{
    return aData[0][2].size();

} // number_of_sera

static inline const acmacs::lispmds::value& projection_data(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    if (aData.empty(":STARTING-COORDSS"))
        return aData[":BATCH-RUNS"][aProjectionNo];
    else if (aProjectionNo == 0)
        return aData[":STARTING-COORDSS"];
    else
        return aData[":BATCH-RUNS"][aProjectionNo - 1];

} // LispmdsProjection::data

static inline const acmacs::lispmds::value& projection_layout(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    if (aData.empty(":STARTING-COORDSS"))
        return aData[":BATCH-RUNS"][aProjectionNo][0];
    else if (aProjectionNo == 0)
        return aData[":STARTING-COORDSS"];
    else
        return aData[":BATCH-RUNS"][aProjectionNo - 1][0];

} // LispmdsProjection::data

// ----------------------------------------------------------------------

std::vector<double> native_column_bases(const acmacs::lispmds::value& aData)
{
    std::vector<double> cb(number_of_sera(aData), 0);
    for (const auto& row: std::get<acmacs::lispmds::list>(aData[0][3])) {
        for (auto [sr_no, titer_v]: acmacs::enumerate(std::get<acmacs::lispmds::list>(row))) {
            try {
                const double titer = std::get<acmacs::lispmds::number>(titer_v);
                if (titer > cb[sr_no])
                    cb[sr_no] = titer;
            }
            catch (std::bad_variant_access&) {
                if (const std::string titer_s = titer_v; !titer_s.empty()) {
                    double titer;
                    switch (titer_s[0]) {
                      case '<':
                          titer = std::stod(titer_s.substr(1));
                          if (titer > cb[sr_no])
                              cb[sr_no] = titer;
                          break;
                      case '>':
                          titer = std::stod(titer_s.substr(1)) + 1;
                          if (titer > cb[sr_no])
                              cb[sr_no] = titer;
                          break;
                      default:
                          break;
                    }
                }
            }
        }
    }
    return cb;

} // native_column_bases

// ----------------------------------------------------------------------

std::vector<double> column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    const auto num_antigens = number_of_antigens(aData);
    const auto num_sera = number_of_sera(aData);
    const auto number_of_points = num_antigens + num_sera;
    const acmacs::lispmds::list& cb = projection_layout(aData, aProjectionNo)[number_of_points][0][1];
    std::vector<double> result(num_sera);
    using diff_t = decltype(cb.end() - cb.begin());
    std::transform(cb.begin() + static_cast<diff_t>(num_antigens), cb.begin() + static_cast<diff_t>(number_of_points), result.begin(), [](const auto& val) -> double { return std::get<acmacs::lispmds::number>(val); });
    return result;

} // column_bases

// ----------------------------------------------------------------------

std::pair<std::shared_ptr<acmacs::chart::ColumnBases>, acmacs::chart::MinimumColumnBasis> forced_column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    try {
        const auto native_cb = native_column_bases(aData);
        const auto cb = column_bases(aData, aProjectionNo);
        if (native_cb == cb) {
            return {nullptr, acmacs::chart::MinimumColumnBasis()};
        }
        else {
            const double min_forced = *std::min_element(cb.begin(), cb.end());
            std::decay_t<decltype(native_cb)> native_upgraded(native_cb.size());
            std::transform(native_cb.begin(), native_cb.end(), native_upgraded.begin(), [min_forced](double b) -> double { return std::max(b, min_forced); });
            // std::cerr << "INFO: native: " << native_cb << '\n';
            // std::cerr << "INFO: forced: " << cb << '\n';
            // std::cerr << "INFO: upgrad: " << native_upgraded << '\n';
            if (native_upgraded == cb)
                return {nullptr, acmacs::chart::MinimumColumnBasis(min_forced)};
            else
                return {std::make_shared<LispmdsColumnBases>(cb), acmacs::chart::MinimumColumnBasis()};
        }
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return {nullptr, acmacs::chart::MinimumColumnBasis()};
    }
    catch (acmacs::lispmds::error& err) {
        std::cerr << "WARNING: broken save: " << err.what() << '\n';
        return {nullptr, acmacs::chart::MinimumColumnBasis()};
    }

} // forced_column_bases

// ----------------------------------------------------------------------

ChartP acmacs::chart::lispmds_import(const std::string_view& aData, Verify aVerify)
{
    // try {
        auto chart = std::make_shared<LispmdsChart>(acmacs::lispmds::parse_string(aData));
        chart->verify_data(aVerify);
        return chart;
    // }
    // catch (std::exception& err) {
    //     std::cerr << "ERROR: " << err.what() << '\n';
    //     throw;
    // }

} // acmacs::chart::lispmds_import

// ----------------------------------------------------------------------

void LispmdsChart::verify_data(Verify) const
{
    try {
        if (number_of_antigens() == 0)
            throw import_error("no antigens");
        if (number_of_sera() == 0)
            throw import_error("no sera (genetic tables are not supported)");
    }
    catch (std::exception& err) {
        throw import_error("[lispmds]: structure verification failed: "s + err.what());
    }

} // LispmdsChart::verify_data

// ----------------------------------------------------------------------

InfoP LispmdsChart::info() const
{
    return std::make_shared<LispmdsInfo>(mData);

} // LispmdsChart::info

// ----------------------------------------------------------------------

AntigensP LispmdsChart::antigens() const
{
    return std::make_shared<LispmdsAntigens>(mData);

} // LispmdsChart::antigens

// ----------------------------------------------------------------------

SeraP LispmdsChart::sera() const
{
    return std::make_shared<LispmdsSera>(mData);

} // LispmdsChart::sera

// ----------------------------------------------------------------------

TitersP LispmdsChart::titers() const
{
    return std::make_shared<LispmdsTiters>(mData);

} // LispmdsChart::titers

// ----------------------------------------------------------------------

ColumnBasesP LispmdsChart::forced_column_bases(MinimumColumnBasis /*aMinimumColumnBasis*/) const
{
    return ::forced_column_bases(mData, 0).first;

} // LispmdsChart::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP LispmdsChart::projections() const
{
    if (!projections_)
        projections_ = std::make_shared<LispmdsProjections>(*this, mData);
    return projections_;

} // LispmdsChart::projections

// ----------------------------------------------------------------------

PlotSpecP LispmdsChart::plot_spec() const
{
    return std::make_shared<LispmdsPlotSpec>(mData);

} // LispmdsChart::plot_spec

// ----------------------------------------------------------------------

size_t LispmdsChart::number_of_antigens() const
{
    return ::number_of_antigens(mData);

} // LispmdsChart::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsChart::number_of_sera() const
{
    return ::number_of_sera(mData);

} // LispmdsChart::number_of_sera

// ----------------------------------------------------------------------

std::string LispmdsInfo::name(Compute) const
{
    if (mData[0].size() >= 5)
        return lispmds_decode(std::get<acmacs::lispmds::symbol>(mData[0][4]));
    else
        return {};

} // LispmdsInfo::name

// ----------------------------------------------------------------------

static inline std::string antigen_name(const acmacs::lispmds::value& aData, size_t aIndex)
{
    return std::get<acmacs::lispmds::symbol>(aData[0][1][aIndex]);
}

// ----------------------------------------------------------------------

Name LispmdsAntigen::name() const
{
    Name name;
    Reassortant reassortant;
    Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return name;

} // LispmdsAntigen::name

// ----------------------------------------------------------------------

Passage LispmdsAntigen::passage() const
{
    Name name;
    Reassortant reassortant;
    Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return passage;

} // LispmdsAntigen::passage

// ----------------------------------------------------------------------

Reassortant LispmdsAntigen::reassortant() const
{
    Name name;
    Reassortant reassortant;
    Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return reassortant;

} // LispmdsAntigen::reassortant

// ----------------------------------------------------------------------

Annotations LispmdsAntigen::annotations() const
{
    Name name;
    Reassortant reassortant;
    Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return annotations;

} // LispmdsAntigen::annotations

// ----------------------------------------------------------------------

bool LispmdsAntigen::reference() const
{
    try {
        const auto& val = mData[":REFERENCE-ANTIGENS"];
        if (val.empty())
            return false;
        const auto name = antigen_name(mData, mIndex);
        const auto& val_l = std::get<acmacs::lispmds::list>(val);
        return std::find_if(val_l.begin(), val_l.end(), [&name](const auto& ev) -> bool { return std::get<acmacs::lispmds::symbol>(ev) == name; }) != val_l.end();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return false;
    }

} // LispmdsAntigen::reference

// ----------------------------------------------------------------------

static inline std::string serum_name(const acmacs::lispmds::value& aData, size_t aIndex)
{
    return std::get<acmacs::lispmds::symbol>(aData[0][2][aIndex]);
}

// ----------------------------------------------------------------------

Name LispmdsSerum::name() const
{
    Name name;
    Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return name;

} // LispmdsSerum::name

// ----------------------------------------------------------------------

Reassortant LispmdsSerum::reassortant() const
{
    Name name;
    Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return reassortant;

} // LispmdsSerum::reassortant

// ----------------------------------------------------------------------

Annotations LispmdsSerum::annotations() const
{
    Name name;
    Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return annotations;

} // LispmdsSerum::annotations

// ----------------------------------------------------------------------

SerumId LispmdsSerum::serum_id() const
{
    Name name;
    Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return serum_id;

} // LispmdsSerum::serum_id

// ----------------------------------------------------------------------

size_t LispmdsAntigens::size() const
{
    return number_of_antigens(mData);

} // LispmdsAntigens::size

// ----------------------------------------------------------------------

AntigenP LispmdsAntigens::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsAntigen>(mData, aIndex);

} // LispmdsAntigens::operator[]

// ----------------------------------------------------------------------

size_t LispmdsSera::size() const
{
    return number_of_sera(mData);

} // LispmdsSera::size

// ----------------------------------------------------------------------

SerumP LispmdsSera::operator[](size_t aIndex) const
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
            std::visit([&result](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, acmacs::lispmds::symbol>) {
                    if (arg[0] != '*')
                        ++result;
                }
                else if constexpr (std::is_same_v<T, acmacs::lispmds::number>)
                    ++result;
                else
                    throw acmacs::lispmds::type_mismatch("Unexpected titer type: "s + typeid(T).name());
            }, titer);
        }
    }
    return result;

} // LispmdsTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

void LispmdsProjection::check() const
{
    try {
        if (auto nd = layout()->number_of_dimensions(); nd > 5)
            throw import_error("[lispmds] projection " + acmacs::to_string(projection_no()) + " has unsupported number of dimensions: " + acmacs::to_string(nd));
    }
    catch (std::exception& err) {
        throw import_error("[lispmds] projection " + acmacs::to_string(projection_no()) + " reading error: " + err.what());
    }

} // LispmdsProjection::check

// ----------------------------------------------------------------------

std::optional<double> LispmdsProjection::stored_stress() const
{
    return std::visit([](auto&& arg) -> std::optional<double> {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, acmacs::lispmds::number>)
            return static_cast<double>(arg);
        else
            return {};
    }, projection_data(mData, projection_no())[1]);

} // LispmdsProjection::stress

// ----------------------------------------------------------------------

class LispmdsLayout : public acmacs::Layout
{
 public:
    LispmdsLayout(const acmacs::lispmds::value& aData, size_t aNumberOfAntigens, size_t aNumberOfSera)
        : acmacs::Layout(aNumberOfAntigens + aNumberOfSera, aData[0].size())
        {
            auto target = begin();
            for (size_t p_no = 0; p_no < number_of_points(); ++p_no) {
                const auto& point = aData[p_no];
                if (point.size() == number_of_dimensions()) {
                    for (size_t dim = 0; dim < point.size(); ++dim)
                        *target++ = std::get<acmacs::lispmds::number>(point[dim]);
                }
                else if (!point.empty())
                    throw invalid_data("LispmdsLayout: point has invalid number of coordinates: " + std::to_string(point.size()) + ", expected 0 or " + std::to_string(number_of_dimensions()));
                else
                    target += static_cast<decltype(target)::difference_type>(number_of_dimensions());
            }
        }

    inline void set(size_t /*aPointNo*/, const acmacs::Coordinates& /*aCoordinates*/) override { throw acmacs::chart::chart_is_read_only{"LispmdsLayout::set: cannot modify"}; }

}; // class LispmdsLayout

// ----------------------------------------------------------------------

std::shared_ptr<Layout> LispmdsProjection::layout() const
{
    // std::cerr << "antigens: " << mNumberOfAntigens << " sera: " << mNumberOfSera << " points: " << (mNumberOfAntigens + mNumberOfSera) << '\n';
    if (!layout_)
        layout_ = std::make_shared<LispmdsLayout>(projection_layout(mData, projection_no()), mNumberOfAntigens, mNumberOfSera);
    return layout_;

} // LispmdsProjection::layout

// ----------------------------------------------------------------------

size_t LispmdsProjection::number_of_dimensions() const
{
    return projection_layout(mData, projection_no())[0].size();

} // LispmdsProjection::number_of_dimensions

// ----------------------------------------------------------------------

ColumnBasesP LispmdsProjection::forced_column_bases() const
{
    return ::forced_column_bases(mData, projection_no()).first;

} // LispmdsProjection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::chart::MinimumColumnBasis LispmdsProjection::minimum_column_basis() const
{
    return ::forced_column_bases(mData, projection_no()).second;

} // LispmdsProjection::minimum_column_basis

// ----------------------------------------------------------------------

acmacs::Transformation LispmdsProjection::transformation() const
{
    acmacs::Transformation result;
    try {
        if (const auto& coord_tr = mData[":CANVAS-COORD-TRANSFORMATIONS"]; !coord_tr.empty()) {
            try {
                if (const auto& v0 = coord_tr[":CANVAS-BASIS-VECTOR-0"]; !v0.empty()) {
                    result.a = std::get<acmacs::lispmds::number>(v0[0]);
                    result.c = std::get<acmacs::lispmds::number>(v0[1]);
                }
            }
            catch (std::exception&) {
            }
            try {
                if (const auto& v1 = coord_tr[":CANVAS-BASIS-VECTOR-1"]; !v1.empty()) {
                    result.b = std::get<acmacs::lispmds::number>(v1[0]);
                    result.d = std::get<acmacs::lispmds::number>(v1[1]);
                }
            }
            catch (std::exception&) {
            }
            try {
                if (static_cast<double>(std::get<acmacs::lispmds::number>(coord_tr[":CANVAS-X-COORD-SCALE"])) < 0) {
                    result.a = - result.a;
                    result.c = - result.c;
                }
            }
            catch (std::exception&) {
            }
            try {
                if (static_cast<double>(std::get<acmacs::lispmds::number>(coord_tr[":CANVAS-Y-COORD-SCALE"])) < 0) {
                    result.b = - result.b;
                    result.d = - result.d;
                }
            }
            catch (std::exception&) {
            }
        }
    }
    catch (acmacs::lispmds::keyword_no_found&) {
    }
    return result;

} // LispmdsProjection::transformation

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::unmovable() const
{
      //   :UNMOVEABLE-COORDS '(87 86 85 83 82 80 81)
    try {
        const acmacs::lispmds::list& val = mData[":UNMOVEABLE-COORDS"];
        return {val.begin(), val.end(), [](const auto& v) -> size_t { return std::get<acmacs::lispmds::number>(v); }};
    }
    catch (std::exception&) {
        return {};
    }

} // LispmdsProjection::unmovable

// ----------------------------------------------------------------------

PointIndexList LispmdsProjection::disconnected() const
{
      // std::cerr << "WARNING: LispmdsProjection::disconnected not implemented\n";
    return {};

} // LispmdsProjection::disconnected

// ----------------------------------------------------------------------

AvidityAdjusts LispmdsProjection::avidity_adjusts() const
{
    try {
        const auto num_points = layout()->number_of_points();
        const acmacs::lispmds::list& cb = projection_layout(mData, projection_no())[num_points][0][1];
        AvidityAdjusts result(num_points);
        for (size_t i = 0; i < num_points; ++i)
            result[i] = std::exp2(static_cast<double>(std::get<acmacs::lispmds::number>(cb[num_points + i])));
        return result;
    }
    catch (acmacs::lispmds::error& err) {
        std::cerr << "WARNING: broken save: " << err.what() << '\n';
        return {};
    }

} // LispmdsProjection::avidity_adjusts

// ----------------------------------------------------------------------

bool LispmdsProjections::empty() const
{
    return mData.empty(":STARTING-COORDSS") && mData.empty(":BATCH-RUNS");

} // LispmdsProjections::empty

// ----------------------------------------------------------------------

size_t LispmdsProjections::size() const
{
    size_t result = 0;
    if (!mData.empty(":STARTING-COORDSS"))
        ++result;
    try {
        result += mData[":BATCH-RUNS"].size();
    }
    catch (acmacs::lispmds::error&) {
    }
    return result;

} // LispmdsProjections::size

// ----------------------------------------------------------------------

ProjectionP LispmdsProjections::operator[](size_t aIndex) const
{
    if (!projections_[aIndex])
        projections_[aIndex] = std::make_shared<LispmdsProjection>(chart(), mData, aIndex, number_of_antigens(mData), number_of_sera(mData));
    return projections_[aIndex];

} // LispmdsProjections::operator[]

// ----------------------------------------------------------------------

bool LispmdsPlotSpec::empty() const
{
    try {
        return mData[":PLOT-SPEC"].empty();
    }
    catch (std::exception&) {
        return true;
    }

} // LispmdsPlotSpec::empty

// ----------------------------------------------------------------------

DrawingOrder LispmdsPlotSpec::drawing_order() const
{
      // :RAISE-POINTS 'NIL
      // :LOWER-POINTS 'NIL
      // don't know how drawing order is stored
    return {};

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
    extract_style(result, aPointNo);
    return result;

} // LispmdsPlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> LispmdsPlotSpec::all_styles() const
{
    try {
        const auto number_of_points = mData[0][1].size() + mData[0][2].size();
        std::vector<acmacs::PointStyle> result(number_of_points);
        for (size_t point_no = 0; point_no < number_of_points; ++point_no) {
            extract_style(result[point_no], point_no);
        }
        return result;
    }
    catch (std::exception& err) {
        std::cerr << "WARNING: [lispmds]: cannot get point styles: " << err.what() << '\n';
    }
    return {};

} // LispmdsPlotSpec::all_styles

// ----------------------------------------------------------------------

size_t LispmdsPlotSpec::number_of_points() const
{
    try {
        return mData[0][1].size() + mData[0][2].size();
    }
    catch (std::exception& err) {
        std::cerr << "WARNING: [lispmds]: cannot get point styles: " << err.what() << '\n';
        return 0;
    }

} // LispmdsPlotSpec::number_of_points

// ----------------------------------------------------------------------

void LispmdsPlotSpec::extract_style(acmacs::PointStyle& aTarget, size_t aPointNo) const
{
    std::string name = aPointNo < number_of_antigens(mData)
                                  ? static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][1][aPointNo])) + "-AG"
                                  : static_cast<std::string>(std::get<acmacs::lispmds::symbol>(mData[0][2][aPointNo - number_of_antigens(mData)])) + "-SR";
    const acmacs::lispmds::list& plot_spec = mData[":PLOT-SPEC"];
    for (const acmacs::lispmds::list& pstyle: plot_spec) {
        if (std::get<acmacs::lispmds::symbol>(pstyle[0]) == name) {
            extract_style(aTarget, pstyle);
            break;
        }
    }

} // LispmdsPlotSpec::extract_style

// ----------------------------------------------------------------------

void LispmdsPlotSpec::extract_style(acmacs::PointStyle& aTarget, const acmacs::lispmds::list& aSource) const
{
    try {
        aTarget.size = Pixels{static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":DS"])) / acmacs::lispmds::DS_SCALE};
          // if antigen also divide size by 2 ?
    }
    catch (std::exception&) {
    }

    try {
        aTarget.label_text = aSource[":WN"];
        aTarget.label.shown = !aTarget.label_text->empty();
    }
    catch (std::exception&) {
    }

    try {
        aTarget.shape = static_cast<std::string>(aSource[":SH"]);
    }
    catch (std::exception&) {
    }

    try {
        aTarget.label.size = Pixels{static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":NS"])) / acmacs::lispmds::NS_SCALE};
    }
    catch (std::exception&) {
    }

    try {
        if (const std::string label_color = aSource[":NC"]; label_color != "{}")
            aTarget.label.color = Color(label_color);
    }
    catch (std::exception&) {
    }

    try {
        if (const std::string fill_color = aSource[":CO"]; fill_color != "{}")
            aTarget.fill = Color(fill_color);
        else
            aTarget.fill = TRANSPARENT;
    }
    catch (std::exception&) {
    }

    try {
        if (const std::string outline_color = aSource[":OC"]; outline_color != "{}")
            aTarget.outline = Color(outline_color);
        else
            aTarget.outline = TRANSPARENT;
    }
    catch (std::exception&) {
    }

    try {
        Color fill = aTarget.fill;
        fill.set_transparency(std::get<acmacs::lispmds::number>(aSource[":TR"]));
        aTarget.fill = fill;
    }
    catch (std::exception&) {
    }

} // LispmdsPlotSpec::extract_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
