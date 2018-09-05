#include <set>
#include <vector>
#include <limits>

#include "acmacs-base/debug.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/virus-name.hh"
#include "acmacs-chart-2/ace-import.hh"
#include "acmacs-chart-2/ace.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

const acmacs::chart::RjsonTiters::Keys acmacs::chart::AceTiters::s_keys_{"l", "d", "L"};
const acmacs::chart::RjsonProjection::Keys acmacs::chart::AceProjection::s_keys_{"s", "l", "c"};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

bool acmacs::chart::is_ace(const std::string_view& aData)
{
    return aData.size() > 35 && aData.front() == '{' && aData.find("\"acmacs-ace-v1\"") != std::string_view::npos;

} // acmacs::chart::is_ace

// ----------------------------------------------------------------------

ChartP acmacs::chart::ace_import(const std::string_view& aData, Verify aVerify)
{
    auto chart = std::make_shared<AceChart>(rjson::parse_string(aData));
    chart->verify_data(aVerify);
    return chart;

} // acmacs::chart::ace_import

// ----------------------------------------------------------------------

void AceChart::verify_data(Verify aVerify) const
{
    try {
        const auto& antigens = data_.get("c", "a");
        if (antigens.empty())
            throw import_error("no antigens");
        const auto& sera = data_.get("c", "s");
        if (sera.empty())
            throw import_error("no sera");
        const auto& titers = data_.get("c", "t");
        if (titers.empty())
            throw import_error("no titers");
        if (const auto& ll = titers["l"]; !ll.is_null()) {
            if (ll.size() != antigens.size())
                throw import_error("number of the titer rows (" + acmacs::to_string(ll.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else if (const auto& dd = titers["d"]; !dd.is_null()) {
            if (dd.size() != antigens.size())
                throw import_error("number of the titer rows (" + acmacs::to_string(dd.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else
            throw import_error("no titers (neither \"l\" nor \"d\" present)");
        if (aVerify != Verify::None) {
            std::cerr << "WARNING: AceChart::verify_data not implemented\n";
        }
    }
    catch (std::exception& err) {
        throw import_error("[ace]: structure verification failed: "s + err.what());
    }

} // AceChart::verify_data

// ----------------------------------------------------------------------

InfoP AceChart::info() const
{
    return std::make_shared<AceInfo>(data_.get("c", "i"));

} // AceChart::info

// ----------------------------------------------------------------------

AntigensP AceChart::antigens() const
{
    return std::make_shared<AceAntigens>(data_.get("c", "a"), mAntigenNameIndex);

} // AceChart::antigens

// ----------------------------------------------------------------------

SeraP AceChart::sera() const
{
    auto sera = std::make_shared<AceSera>(data_.get("c", "s"));
    set_homologous(find_homologous_for_big_chart::no, sera);
    return sera;

} // AceChart::sera

// ----------------------------------------------------------------------

const rjson::value& AceChart::extension_field(std::string field_name) const
{
    return data_.get("c", field_name);

} // AceChart::extension_field

// ----------------------------------------------------------------------

TitersP AceChart::titers() const
{
    return std::make_shared<AceTiters>(data_.get("c", "t"));

} // AceChart::titers

// ----------------------------------------------------------------------

ColumnBasesP AceChart::forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const
{
    if (const auto& cb = data_.get("c", "C"); !cb.empty())
        return std::make_shared<AceColumnBases>(cb, aMinimumColumnBasis);
    return nullptr;

} // AceChart::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP AceChart::projections() const
{
    if (!projections_)
        projections_ = std::make_shared<AceProjections>(*this, data_.get("c", "P"));
    return projections_;

} // AceChart::projections

// ----------------------------------------------------------------------

PlotSpecP AceChart::plot_spec() const
{
    return std::make_shared<AcePlotSpec>(data_.get("c", "p"), *this);

} // AceChart::plot_spec

// ----------------------------------------------------------------------

bool AceChart::is_merge() const
{
    return !data_.get("c", "t", "L").empty();

} // AceChart::is_merge

// ----------------------------------------------------------------------

std::string AceInfo::name(Compute aCompute) const
{
    std::string result{data_["N"].get_or_default("")};
    if (result.empty()) {
        if (const auto& sources = data_["S"]; !sources.empty()) {
            std::vector<std::string> composition(sources.size());
            rjson::transform(sources, composition.begin(), [](const rjson::value& sinfo) { return sinfo["N"].get_or_default(""); });
            composition.erase(std::remove_if(composition.begin(), composition.end(), [](const auto& s) { return s.empty(); }), composition.end());
            if (composition.size() > (sources.size() / 2))
                result = string::join(" + ", composition);
        }
    }
    if (result.empty() && aCompute == Compute::Yes) {
        result = string::join(" ", {virus_not_influenza(aCompute), virus_type(aCompute), subset(aCompute), assay(aCompute), lab(aCompute), rbc_species(aCompute), date(aCompute)});
    }
    return result;

} // AceInfo::name

// ----------------------------------------------------------------------

std::string AceInfo::make_field(const char* aField, const char* aSeparator, Compute aCompute) const
{
    std::string result{data_[aField].get_or_default("")};
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources = data_["S"]; !sources.empty()) {
            std::set<std::string> composition;
            rjson::transform(sources, std::inserter(composition, composition.begin()), [aField](const rjson::value& sinfo) { return sinfo[aField].get_or_default(""); });
            result = string::join(aSeparator, composition);
        }
    }
    return result;

} // AceInfo::make_field

// ----------------------------------------------------------------------

std::string AceInfo::date(Compute aCompute) const
{
    std::string result{data_["D"].get_or_default("")};
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources = data_["S"]; !sources.empty()) {
            std::vector<std::string> composition{sources.size()};
            rjson::transform(sources, composition.begin(), [](const rjson::value& sinfo) { return sinfo["D"].get_or_default(""); });
            std::sort(std::begin(composition), std::end(composition));
            result = string::join("-", {composition.front(), composition.back()});
        }
    }
    return result;

} // AceInfo::date

// ----------------------------------------------------------------------

BLineage AceAntigen::lineage() const
{
    return data_["L"].get_or_default("");

} // AceAntigen::lineage

BLineage AceSerum::lineage() const
{
    return data_["L"].get_or_default("");

} // AceSerum::lineage

// ----------------------------------------------------------------------

std::optional<size_t> AceAntigens::find_by_full_name(std::string aFullName) const
{
    if (mAntigenNameIndex.empty())
        make_name_index();
    const std::string_view name{virus_name::name(aFullName)};
    if (const auto found = mAntigenNameIndex.find(name); found != mAntigenNameIndex.end()) {
        for (auto index: found->second) {
            if (AceAntigen(data_[index]).full_name() == aFullName)
                return index;
        }
    }
    return {};

} // AceAntigens::find_by_full_name

// std::optional<size_t> AceAntigens::find_by_full_name(std::string aFullName) const
// {
//     const std::string_view name{virus_name::name(aFullName)};
//     for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
//         if ((*iter)["N"] == name && AceAntigen(*iter).full_name() == aFullName) {
//             return static_cast<size_t>(iter - data_.begin());
//         }
//     }
//     return {};

// } // AceAntigens::find_by_full_name

// ----------------------------------------------------------------------

void AceAntigens::make_name_index() const
{
    rjson::for_each(data_, [this](const rjson::value& val, size_t index) {
        mAntigenNameIndex[static_cast<std::string>(val["N"])].push_back(index);
    });

} // AceAntigens::make_name_index

// ----------------------------------------------------------------------

// std::shared_ptr<Layout> AceProjection::layout() const
// {
//     if (!layout_)
//         layout_ = std::make_shared<rjson_import::Layout>(data_.get_or_empty_array("l"));
//     return layout_;

// } // AceProjection::layout

// ----------------------------------------------------------------------

// size_t AceProjection::number_of_dimensions() const
// {
//     return rjson_import::number_of_dimensions(data_["l"]);

// } // AceProjection::number_of_dimensions

// ----------------------------------------------------------------------

ColumnBasesP AceProjection::forced_column_bases() const
{
    if (const auto& cb = data()["C"]; !cb.empty())
        return std::make_shared<AceColumnBases>(cb);
    return nullptr;

} // AceProjection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::Transformation AceProjection::transformation() const
{
    acmacs::Transformation result;
    if (const auto& array = data()["t"]; !array.empty()) {
        result.set(array[0], array[1], array[2], array[3]);
    }
    return result;

} // AceProjection::transformation

// ----------------------------------------------------------------------

Color AcePlotSpec::error_line_positive_color() const
{
    if (const auto& color = data_.get("E", "c"); !color.is_null())
        return Color(color);
    else
        return "red";

} // AcePlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color AcePlotSpec::error_line_negative_color() const
{
    if (const auto& color = data_.get("e", "c"); !color.is_null())
        return Color(color);
    else
        return "blue";

} // AcePlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle AcePlotSpec::style(size_t aPointNo) const
{
    const auto& indices = data_["p"];
    try {
        const size_t style_no = indices[aPointNo];
        // std::cerr << "style " << aPointNo << ' ' << style_no << ' ' << data_["P"][style_no].to_json() << '\n';
        return extract(data_["P"][style_no], aPointNo, style_no);
    }
    catch (std::exception& /*err*/) {
          // std::cerr << "WARNING: [ace]: cannot get style for point " << aPointNo << ": " << err.what() << '\n';
    }
    return mChart.default_style(aPointNo);

} // AcePlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> AcePlotSpec::all_styles() const
{
    if (const auto& indices = data_["p"]; !indices.empty()) {
        std::vector<acmacs::PointStyle> result(indices.size());
        for (auto [point_no, target]: acmacs::enumerate(result)) {
            try {
                const size_t style_no = indices[point_no];
                target = extract(data_["P"][style_no], point_no, style_no);
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [ace]: cannot get point " << point_no << " style: " << err.what() << '\n';
                target = mChart.default_style(point_no);
            }
        }
        return result;
    }
    else {
          // std::cerr << "WARNING: [ace]: no point styles stored, default is used\n";
        return mChart.default_all_styles();
    }

} // AcePlotSpec::all_styles

// ----------------------------------------------------------------------

size_t AcePlotSpec::number_of_points() const
{
    if (const auto& indices = data_["p"]; !indices.empty())
        return indices.size();
    else
        return mChart.number_of_points();

} // AcePlotSpec::number_of_points

// ----------------------------------------------------------------------

acmacs::PointStyle AcePlotSpec::extract(const rjson::value& aSrc, size_t aPointNo, size_t aStyleNo) const
{
    acmacs::PointStyle result;
    rjson::for_each(aSrc, [&result,aPointNo,aStyleNo,this](const rjson::object::value_type& kv) {
        if (const auto [field_name, field_value] = kv; !field_name.empty()) {
            try {
                switch (field_name[0]) {
                  case '+':
                      result.shown = field_value;
                      break;
                  case 'F':
                      result.fill = Color(field_value);
                      break;
                  case 'O':
                      result.outline = Color(field_value);
                      break;
                  case 'o':
                      result.outline_width = Pixels{field_value};
                      break;
                  case 's':
                      result.size = Pixels{static_cast<double>(field_value) * acmacs::chart::ace::PointScale};
                      break;
                  case 'r':
                      result.rotation = Rotation{field_value};
                      break;
                  case 'a':
                      result.aspect = Aspect{field_value};
                      break;
                  case 'S':
                      result.shape = static_cast<std::string>(field_value);
                      break;
                  case 'l':
                      this->label_style(result, field_value);
                      break;
                }
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [ace]: point " << aPointNo << " style " << aStyleNo << " field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << rjson::to_string(field_value) << '\n';
            }
        }
    });
    return result;

} // AcePlotSpec::extract

// ----------------------------------------------------------------------

void AcePlotSpec::label_style(acmacs::PointStyle& aStyle, const rjson::value& aData) const
{
    rjson::for_each(aData, [&aStyle](const rjson::object::value_type& kv) {
        if (const auto [field_name, field_value] = kv; !field_name.empty()) {
            try {
                auto& label_style = aStyle.label;
                switch (field_name[0]) {
                  case '+':
                      label_style.shown = field_value;
                      break;
                  case 'p':
                      label_style.offset = acmacs::Offset{field_value[0], field_value[1]};
                      break;
                  case 's':
                      label_style.size = Pixels{static_cast<double>(field_value) * acmacs::chart::ace::LabelScale};
                      break;
                  case 'c':
                      label_style.color = Color(field_value);
                      break;
                  case 'r':
                      label_style.rotation = Rotation{field_value};
                      break;
                  case 'i':
                      label_style.interline = field_value;
                      break;
                  case 'f':
                      label_style.style.font_family = field_value;
                      break;
                  case 'S':
                      label_style.style.slant = static_cast<std::string>(field_value);
                      break;
                  case 'W':
                      label_style.style.weight = static_cast<std::string>(field_value);
                      break;
                  case 't':
                      aStyle.label_text = field_value;
                      break;
                }
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [ace]: label style field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << rjson::to_string(field_value) << '\n';
            }
        }
    });

} // AcePlotSpec::label_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
