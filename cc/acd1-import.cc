#include <set>
#include <map>
#include <vector>
#include <limits>

#include "acmacs-base/timeit.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-chart-2/acd1-import.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

constexpr const double PointScale = 5.0;
constexpr const double LabelScale = 10.0;

static std::string convert_to_json(const std::string_view& aData);
static void convert_set(std::string& aData, const std::vector<size_t>& aPerhapsSet);

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

const acmacs::chart::RjsonTiters::Keys acmacs::chart::Acd1Titers::s_keys_{"titers_list_of_list", "titers_list_of_dict", "layers_dict_for_antigen"};
const acmacs::chart::RjsonProjection::Keys acmacs::chart::Acd1Projection::s_keys_{"stress", "layout", "comment"};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

ChartP acmacs::chart::acd1_import(const std::string_view& aData, Verify aVerify)
{
    const std::string json = convert_to_json(aData);
    try {
        auto chart = std::make_shared<Acd1Chart>(rjson::parse_string(json));
        chart->verify_data(aVerify);
        return std::move(chart);
    }
    catch (rjson::parse_error&) {
        std::cout << json << '\n';
        throw;
    }

} // acmacs::chart::acd1_import

// ----------------------------------------------------------------------

static inline bool input_matches(const std::string_view& aInput, size_t aOffset, const std::string_view& aExpected)
{
      // aOffset can be "negative" (overflow), ignore that case
    return aOffset < aInput.size() && aInput.substr(aOffset, aExpected.size()) == aExpected;
}

// returns [it is a numeric key, end of digits pos]
static inline std::pair<bool, size_t> object_numeric_key(const std::string_view& aInput, size_t aOffset)
{
    size_t prev = aOffset - 1;
      // skip spaces
    for (; prev > 0 && std::isspace(aInput[prev]); --prev);
    const bool numeric_key_start = aInput[prev] == ',' || aInput[prev] == '{';
      // skip number (including fraction and exp)
    for (++aOffset; std::isdigit(aInput[aOffset]) || aInput[aOffset] == '.' || aInput[aOffset] == '-' || aInput[aOffset] == '+' || aInput[aOffset] == 'e' || aInput[aOffset] == 'E'; ++aOffset);
    const bool numeric_key_end = aInput[aOffset] == ':';
    return {numeric_key_start && numeric_key_end, aOffset};
}

std::string convert_to_json(const std::string_view& aData)
{
    // Timeit ti("converting acd1 (" + std::to_string(aData.size()) + " bytes) to json: ");
    std::string result;
    std::vector<size_t> perhaps_set;
    for (auto input = aData.find("data = {") + 7; input < aData.size(); ++input) {
        switch (aData[input]) {
          case '\'':
              if (input > 0 && std::isalnum(aData[input - 1]) && std::isalnum(aData[input + 1])) {
                  result.append(1, aData[input]); // "COTE D'IVOIR" case
              }
              else {
                  result.append(1, '"');
                  if (aData[input - 1] == '{')
                      perhaps_set.push_back(result.size() - 2);
              }
              break;
          case '"': // string containing ' enclosed in double quotes, e.g. "COTE D'IVOIR"
              // result.append(1, '\\');
              result.append(1, aData[input]);
              if (aData[input - 1] == '{')
                  perhaps_set.push_back(result.size() - 2);
              break;
          case '\\':
              result.append(1, aData[input++]);
              result.append(1, aData[input]);
              std::cerr << "WARNING: [acd1]: convert_to_json: \\ in the data\n";
              break;
          case 'T':
              if (input_matches(aData, input - 2, ": True")) {
                  result.append("true");
                  input += 3;
              }
              else
                  result.append(1, aData[input]);
              break;
          case 'F':
              if (input_matches(aData, input - 2, ": False")) {
                  result.append("false");
                  input += 4;
              }
              else
                  result.append(1, aData[input]);
              break;
          case 'N':
              if (input_matches(aData, input - 2, ": None")) {
                  result.append("null");
                  input += 3;
              }
              else
                  result.append(1, aData[input]);
              break;
          case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
              if (auto [numeric_key, end_of_number] = object_numeric_key(aData, input); numeric_key) {
                  result.append(1, '"').append(aData.substr(input, end_of_number - input)).append(1, '"');
                  input = end_of_number - 1;
              }
              else {
                  if (aData[input - 1] == '{')
                      perhaps_set.push_back(result.size() - 1);
                  result.append(aData.substr(input, end_of_number - input));
                  input = end_of_number - 1;
              }
              break;
          case 'n':
              if (input_matches(aData, input - 1, "[nan, nan")) {
                    // replace [nan, nan] with []
                  for (input += 3; input < aData.size() && aData[input] != ']'; ++input)
                      ;
                  result.append(1, aData[input]);
              }
              else
                  result.append(1, aData[input]);
              break;
          case '}':
          case ']':
              result.append(1, aData[input]);
              {
                    // remove comma before bracket
                  size_t output = result.size() - 2;
                  while (output > 0 && std::isspace(result[output]))
                      --output;
                  if (result[output] == ',')
                      result[output] = ' ';
              }
              break;
          case '(': // lab_id is set of tuples
              if (input > 0 && aData[input - 1] == '{') {
                  result.back() = '[';
                  result.append(1, '[');
              }
              else
                  result.append(1, aData[input]);
              break;
          case ')': // lab_id is set of tuples
              if (aData[input + 1] == '}') {
                  result.append(1, ']').append(1, ']');
                  ++input;
              }
              else
                  result.append(1, aData[input]);
              break;
          case '#':             // possible comment
          {
              bool comment = true, search_done = false;
              for (auto ri = result.rbegin(); !search_done && ri != result.rend(); ++ri) {
                  switch (*ri) {
                    case ' ':
                        break;
                    case '\n':
                        search_done = true;
                        break;
                    default:
                        search_done = true;
                        comment = false;
                        break;
                  }
              }
              if (comment) {
                    // skip until eol
                  ++input;
                  for (++input; aData[input] != '\n' && input < aData.size(); ++input);
              }
              else
                  result.append(1, aData[input]);
          }
          break;
          default:
              result.append(1, aData[input]);
              break;
        }
    }
      // std::cerr << "perhaps_set: " << perhaps_set.size() << '\n';
    convert_set(result, perhaps_set);
    // std::cout << result << '\n';
    return result;

} // convert_to_json

// ----------------------------------------------------------------------

void convert_set(std::string& aData, const std::vector<size_t>& aPerhapsSet)
{
      // Timeit ti("convert_set: ");
    for (auto offset: aPerhapsSet) {
        if (aData[offset + 1] == '"') {
              // set of strings
            auto p = aData.begin() + static_cast<ssize_t>(offset) + 2;
            for (; *p != '"' && p < aData.end(); ++p);
            ++p;
            if (*p == '}') {    // set
                  // std::cerr << "SET: " << std::string_view(aData.data() + offset, p - aData.begin() - offset + 1) << '\n';
                aData[offset] = '[';
                *p = ']';
            }
        }
        else if (std::isdigit(aData[offset + 1])) {
              // set of numbers, e.g. ["table"]["antigens"]["sources"]
            auto p = aData.begin() + static_cast<ssize_t>(offset) + 2;
            for (; *p != ':' && *p != '}' && p < aData.end(); ++p);
            if (*p == '}') {    // set
                aData[offset] = '[';
                *p = ']';
            }
        }
    }

} // convert_set

// ----------------------------------------------------------------------

void Acd1Chart::verify_data(Verify aVerify) const
{
    try {
        // if (static_cast<size_t>(data_["version"]) != 4)
        //     throw import_error("invalid version");
        const auto& antigens = data_.get("table", "antigens");
        if (antigens.empty())
            throw import_error("no antigens");
        const auto& sera = data_.get("table", "sera");
        if (sera.empty())
            throw import_error("no sera");
        const auto& titers = data_.get("table", "titers");
        if (titers.empty())
            throw import_error("no titers");
        if (const auto& ll = titers["titers_list_of_list"]; !ll.is_null()) {
            if (ll.size() != antigens.size())
                throw import_error("number of the titer rows (" + acmacs::to_string(ll.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else if (const auto& dd = titers["titers_list_of_dict"]; !dd.is_null()) {
            if (dd.size() != antigens.size())
                throw import_error("number of the titer rows (" + acmacs::to_string(dd.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else
            throw import_error("no titers (neither \"titers_list_of_list\" nor \"titers_list_of_dict\" present)");
        if (aVerify != Verify::None) {
            std::cerr << "WARNING: Acd1Chart::verify_data not implemented\n";
        }
    }
    catch (std::exception& err) {
        throw import_error("[acd1]: structure verification failed: "s + err.what());
    }

} // Acd1Chart::verify_data

// ----------------------------------------------------------------------

InfoP Acd1Chart::info() const
{
    return std::make_shared<Acd1Info>(data_["chart_info"]);

} // Acd1Chart::info

// ----------------------------------------------------------------------

AntigensP Acd1Chart::antigens() const
{
    return std::make_shared<Acd1Antigens>(data_.get("table", "antigens"), mAntigenNameIndex);

} // Acd1Chart::antigens

// ----------------------------------------------------------------------

SeraP Acd1Chart::sera() const
{
    return std::make_shared<Acd1Sera>(data_.get("table", "sera"));

} // Acd1Chart::sera

// ----------------------------------------------------------------------

TitersP Acd1Chart::titers() const
{
    return std::make_shared<Acd1Titers>(data_.get("table", "titers"));

} // Acd1Chart::titers

// ----------------------------------------------------------------------

ColumnBasesP Acd1Chart::forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const
{
    if (const auto& cb = data_.get("table", "column_bases"); !cb.empty())
        return std::make_shared<Acd1ColumnBases>(cb, aMinimumColumnBasis);
    return nullptr;

} // Acd1Chart::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP Acd1Chart::projections() const
{
    if (!projections_)
        projections_ = std::make_shared<Acd1Projections>(*this, data_["projections"]);
    return projections_;

} // Acd1Chart::projections

// ----------------------------------------------------------------------

PlotSpecP Acd1Chart::plot_spec() const
{
    return std::make_shared<Acd1PlotSpec>(data_["plot_spec"], *this);

} // Acd1Chart::plot_spec

// ----------------------------------------------------------------------

bool Acd1Chart::is_merge() const
{
    return !data_.get("table", "titers", "layers_dict_for_antigen").empty();

} // Acd1Chart::is_merge

// ----------------------------------------------------------------------

std::string Acd1Info::name(Compute aCompute) const
{
    std::string result{data_["name"].get_or_default("")};
    if (result.empty()) {
        if (const auto& sources = data_["sources"]; !sources.empty()) {
            std::vector<std::string> composition(sources.size());
            rjson::transform(sources, composition.begin(), [](const rjson::value& sinfo) { return sinfo["name"].get_or_default(""); });
            composition.erase(std::remove_if(composition.begin(), composition.end(), [](const auto& s) { return s.empty(); }), composition.end());
            if (composition.size() > (sources.size() / 2))
                result = string::join(" + ", composition); // use only, if most sources have "name"
        }
    }
    if (result.empty() && aCompute == Compute::Yes) {
        result = string::join(" ", {virus_not_influenza(aCompute), virus_type(aCompute), subset(aCompute), assay(aCompute), lab(aCompute), rbc_species(aCompute), date(aCompute)});
    }
    return result;

} // Acd1Info::name

// ----------------------------------------------------------------------

std::string Acd1Info::make_field(const char* aField, const char* aSeparator, Compute aCompute) const
{
    std::string result{data_[aField].get_or_default("")};
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources = data_["sources"]; !sources.empty()) {
            std::set<std::string> composition;
            rjson::transform(sources, std::inserter(composition, composition.begin()), [aField](const rjson::value& sinfo) { return sinfo[aField].get_or_default(""); });
            result = string::join(aSeparator, composition);
        }
    }
    return result;

} // Acd1Info::make_field

// ----------------------------------------------------------------------

TableDate Acd1Info::date(Compute aCompute) const
{
    std::string result = data_["date"].get_or_default("");
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources = data_["sources"]; !sources.empty()) {
            std::vector<std::string> composition{sources.size()};
            rjson::transform(sources, composition.begin(), [](const rjson::value& sinfo) { return sinfo["date"].get_or_default(""); });
            std::sort(std::begin(composition), std::end(composition));
            result = string::join("-", {composition.front(), composition.back()});
        }
    }
    return std::move(result);

} // Acd1Info::date

// ----------------------------------------------------------------------

static inline Name make_name(const rjson::value& aData)
{
    if (auto name = aData["_name"].get_or_default(""); !name.empty())
        return std::move(name);
    if (auto isolation_number = aData["isolation_number"].get_or_default(""); !isolation_number.empty()) {
        std::string host = aData["host"].get_or_default("");
        if (host == "HUMAN")
            host.clear();
        return string::join("/", {aData["virus_type"].get_or_default(""), host, aData.get("location", "name").get_or_default(""), isolation_number, aData["year"].get_or_default("")});
    }
    else if (auto raw_name = aData["raw_name"].get_or_default(""); !raw_name.empty()) {
        return std::move(raw_name);
    }
    else {
        const std::string cdc_abbreviation = aData.get("location", "cdc_abbreviation").get_or_default("");
        std::string name = aData["name"].get_or_default("");
        if (!cdc_abbreviation.empty() && name.size() > 3 && name[2] == '-' && name[0] == cdc_abbreviation[0] && name[1] == cdc_abbreviation[1])
            name.erase(0, 3);   // old cdc name (acmacs-b?) begins with cdc_abbreviation
        return string::join(" ", {cdc_abbreviation, name});
    }
}

Name Acd1Antigen::name() const
{
    return make_name(data_);

} // Acd1Antigen::name

Name Acd1Serum::name() const
{
    return make_name(data_);

} // Acd1Serum::name

// ----------------------------------------------------------------------

static inline Passage make_passage(const rjson::value& aData)
{
    if (const auto& p_dict = aData["passage"]; !p_dict.is_null()) {
        std::string p = p_dict["passage"].get_or_default("");
        if (auto date = p_dict["date"].get_or_default(""); !date.empty())
            p += " (" + date + ")";
        return std::move(p);
    }
    else if (auto p_str = aData["passage"].get_or_default(""); !p_str.empty()) {
        return std::move(p_str);
    }
    else
        return {};
}

Passage Acd1Antigen::passage() const
{
    return make_passage(data_);

} // Acd1Antigen::passage

Passage Acd1Serum::passage() const
{
    return make_passage(data_);

} // Acd1Serum::passage

// ----------------------------------------------------------------------

static inline Reassortant make_reassortant(const rjson::value& aData)
{
    if (const auto& r_dict = aData["reassortant"]; !r_dict.is_null()) {
        const auto& complete = r_dict["complete"];
        const auto& incomplete = r_dict["incomplete"];
        std::vector<std::string> composition;
        rjson::transform(complete, std::back_inserter(composition), [](const rjson::value& val) -> std::string { return static_cast<std::string>(val); }); // cannot use rjson::copy here
        rjson::transform(incomplete, std::back_inserter(composition), [](const rjson::value& val) -> std::string { return static_cast<std::string>(val); }); // cannot use rjson::copy here
        return string::join(" ", composition);
    }
    else if (auto r_str = aData["reassortant"].get_or_default(""); !r_str.empty()) {
        return std::move(r_str);
    }
    else
        return {};
}

Reassortant Acd1Antigen::reassortant() const
{
    return make_reassortant(data_);

} // Acd1Antigen::reassortant

Reassortant Acd1Serum::reassortant() const
{
    return make_reassortant(data_);

} // Acd1Serum::reassortant

// ----------------------------------------------------------------------

LabIds Acd1Antigen::lab_ids() const
{
    LabIds result;
    if (data_["lab_id"].is_array())
        rjson::transform(data_["lab_id"], std::back_inserter(result), [](const rjson::value& val) -> std::string { return static_cast<std::string>(val[0]) + '#' + static_cast<std::string>(val[1]); });
    else
        rjson::transform(data_["lab_id"], std::back_inserter(result), [](const std::string& key, const rjson::value& val) -> std::string { return string::concat(key, '#', static_cast<std::string_view>(val)); });
    return result;

} // Acd1Antigen::lab_ids

// ----------------------------------------------------------------------

static inline Annotations make_annotations(const rjson::value& aData)
{
    Annotations result;
      // mutations, extra, distinct, annotations, control_duplicate
    if (aData["distinct"].get_or_default(false) || aData["DISTINCT"].get_or_default(false) || !aData["control_duplicate"].get_or_default("").empty() || !aData["CONTROL_DUPLICATE"].get_or_default("").empty())
        result.push_back("DISTINCT");
    result.push_back(aData["extra"].get_or_default(""));
    result.push_back(aData["EXTRA"].get_or_default(""));
    rjson::transform(aData["annotations"], std::back_inserter(result), [](const rjson::value& val) -> std::string { return static_cast<std::string>(val); }); // cannot use rjson::copy here
    rjson::transform(aData["mutations"], std::back_inserter(result), [](const rjson::value& val) -> std::string { return static_cast<std::string>(val); }); // cannot use rjson::copy here
    return result;
}

Annotations Acd1Antigen::annotations() const
{
    return make_annotations(data_);

} // Acd1Antigen::annotations

Annotations Acd1Serum::annotations() const
{
    return make_annotations(data_);

} // Acd1Serum::annotations

// ----------------------------------------------------------------------

BLineage Acd1Antigen::lineage() const
{
    return data_["lineage"].get_or_default("");

} // Acd1Antigen::lineage

BLineage Acd1Serum::lineage() const
{
    return data_["lineage"].get_or_default("");

} // Acd1Serum::lineage

// ----------------------------------------------------------------------

SerumId Acd1Serum::serum_id() const
{
    if (const auto& s_dict = data_["serum_id"]; !s_dict.is_null()) {
        return s_dict["serum_id"];
    }
    else if (auto p_str = data_["serum_id"].get_or_default(""); !p_str.empty()) {
        return std::move(p_str);
    }
    else
        return {};

} // Acd1Serum::serum_id

// ----------------------------------------------------------------------

std::optional<size_t> Acd1Antigens::find_by_full_name(std::string_view aFullName) const
{
    if (mAntigenNameIndex.empty())
        make_name_index();
    const std::string name(::virus_name::name(aFullName));
    if (const auto found = mAntigenNameIndex.find(name); found != mAntigenNameIndex.end()) {
        for (auto index: found->second) {
            if (Acd1Antigen(data_[index]).full_name() == aFullName)
                return index;
        }
    }
    return {};

} // Acd1Antigens::find_by_full_name

// ----------------------------------------------------------------------

void Acd1Antigens::make_name_index() const
{
    rjson::for_each(data_, [this](const rjson::value& val, size_t index) {
        mAntigenNameIndex[make_name(val)].push_back(index);
    });

} // Acd1Antigens::make_name_index

// ----------------------------------------------------------------------

// std::string Acd1Projection::comment() const
// {
//     try {
//         return data_["comment"];
//     }
//     catch (std::exception&) {
//         return {};
//     }

// } // Acd1Projection::comment

// ----------------------------------------------------------------------

// std::shared_ptr<acmacs::Layout> Acd1Projection::layout() const
// {
//     if (!layout_)
//         layout_ = std::make_shared<rjson_import::Layout>(data_.get_or_empty_array("layout"));
//     return layout_;

// } // Acd1Projection::layout

// ----------------------------------------------------------------------

// size_t Acd1Projection::number_of_dimensions() const
// {
//     return rjson_import::number_of_dimensions(data_["layout"]);

// } // Acd1Projection::number_of_dimensions

// ----------------------------------------------------------------------

ColumnBasesP Acd1Projection::forced_column_bases() const
{
    if (const rjson::value& cb = data().get("stress_evaluator_parameters", "column_bases"); !cb.empty())
        return std::make_shared<Acd1ColumnBases>(cb);
    if (const rjson::value& cb = data().get("stress_evaluator_parameters", "columns_bases"); !cb.empty())
        return std::make_shared<Acd1ColumnBases>(cb);
    return nullptr;

} // Acd1Projection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::Transformation Acd1Projection::transformation() const
{
    acmacs::Transformation result(number_of_dimensions());
    if (const auto& array = data()["transformation"]; !array.empty()) {
        result.set(array[0][0], array[0][1], array[1][0], array[1][1]);
    }
    return result;

} // Acd1Projection::transformation

// ----------------------------------------------------------------------

PointIndexList Acd1Projection::make_attributes(size_t aAttr) const
{
    PointIndexList result;
    if (const rjson::value& attrs = data().get("stress_evaluator_parameters", "antigens_sera_attributes"); !attrs.is_null()) {
        rjson::for_each(attrs["antigens"], [&result, aAttr](const rjson::value& val, size_t index) {
            if (static_cast<size_t>(val) == aAttr)
                result.insert(index);
        });
        rjson::for_each(attrs["sera"], [&result, aAttr,number_of_antigens=attrs["antigens"].size()](const rjson::value& val, size_t index) {
            if (static_cast<size_t>(val) == aAttr)
                result.insert(index + number_of_antigens);
        });
    }
    return result;

} // Acd1Projection::make_attributes

// ----------------------------------------------------------------------

// static inline PointIndexList make_attributes(const rjson::value& aData, size_t aAttr)
// {
// }

// PointIndexList Acd1Projection::unmovable() const
// {
//     return make_attributes(data_, 1);

// } // Acd1Projection::unmovable

// // ----------------------------------------------------------------------

// PointIndexList Acd1Projection::disconnected() const
// {
//     return make_attributes(data_, 2);

// } // Acd1Projection::disconnected

// // ----------------------------------------------------------------------

// PointIndexList Acd1Projection::unmovable_in_the_last_dimension() const
// {
//     return make_attributes(data_, 3);

// } // Acd1Projection::unmovable_in_the_last_dimension

// ----------------------------------------------------------------------

AvidityAdjusts Acd1Projection::avidity_adjusts() const
{
    if (const rjson::value& titer_multipliers = data().get("stress_evaluator_parameters", "antigens_sera_titers_multipliers"); !titer_multipliers.empty()) {
        const rjson::value& antigens = titer_multipliers["antigens"];
        const rjson::value& sera = titer_multipliers["sera"];
        AvidityAdjusts aa(antigens.size() + sera.size());
        for (size_t ag_no = 0; ag_no < antigens.size(); ++ag_no)
            aa[ag_no] = antigens[ag_no];
        for (size_t sr_no = 0; sr_no < sera.size(); ++sr_no)
            aa[sr_no + antigens.size()] = sera[sr_no];
        return aa;
    }
    else
        return {};

} // Acd1Projection::avidity_adjusts

// ----------------------------------------------------------------------

DrawingOrder Acd1PlotSpec::drawing_order() const
{
    DrawingOrder result;
    rjson::for_each(data_["drawing_order"], [&result](const rjson::value& do1) { rjson::transform(do1, std::back_inserter(result), [](const rjson::value& val) -> size_t { return val; }); });
    return result;

} // Acd1PlotSpec::drawing_order

// ----------------------------------------------------------------------

Color Acd1PlotSpec::error_line_positive_color() const
{
    if (const auto& color = data_.get("error_line_positive", "color"); !color.is_null()) {
        if (color.is_string())
            return Color(static_cast<std::string_view>(color));
        if (color.is_number())
            return Color(static_cast<size_t>(color));
    }
    return "red";

} // Acd1PlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color Acd1PlotSpec::error_line_negative_color() const
{
    if (const auto& color = data_.get("error_line_negative", "color"); !color.is_null()) {
        if (color.is_string())
            return Color(static_cast<std::string_view>(color));
        if (color.is_number())
            return Color(static_cast<size_t>(color));
    }
    return "blue";

} // Acd1PlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle Acd1PlotSpec::style(size_t aPointNo) const
{
    try {
        const rjson::value& indices = data_["points"];
        const size_t style_no = indices[aPointNo];
        return extract(data_["styles"][style_no], aPointNo, style_no);
    }
    catch (std::exception& /*err*/) {
        // std::cerr << "WARNING: [acd1]: cannot get style for point " << aPointNo << ": " << err.what() << '\n';
    }
    return mChart.default_style(aPointNo);

} // Acd1PlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> Acd1PlotSpec::all_styles() const
{
    if (const rjson::value& indices = data_["points"]; !indices.empty()) {
        std::vector<acmacs::PointStyle> result(indices.size());
        for (auto [point_no, target]: acmacs::enumerate(result)) {
            try {
                const size_t style_no = indices[point_no];
                target = extract(data_["styles"][style_no], point_no, style_no);
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [acd1]: cannot get point " << point_no << " style: " << err.what() << '\n';
                target = mChart.default_style(point_no);
            }
        }
        return result;
    }
    else {
          // std::cerr << "WARNING: [ace]: no point styles stored, default is used\n";
        return mChart.default_all_styles();
    }

} // Acd1PlotSpec::all_styles

// ----------------------------------------------------------------------

size_t Acd1PlotSpec::number_of_points() const
{
    if (const rjson::value& indices = data_["points"]; !indices.empty())
        return indices.size();
    else
        return mChart.number_of_points();

} // Acd1PlotSpec::number_of_points

// ----------------------------------------------------------------------

acmacs::PointStyle Acd1PlotSpec::extract(const rjson::value& aSrc, size_t aPointNo, size_t aStyleNo) const
{
    acmacs::PointStyle result;
    rjson::for_each(aSrc, [&result, aPointNo, aStyleNo](const std::string& field_name, const rjson::value& field_value) {
        if (!field_name.empty()) {
            try {
                if (field_name == "shown")
                    result.shown = static_cast<bool>(field_value);
                else if (field_name == "fill_color")
                    result.fill = Color(static_cast<size_t>(field_value));
                else if (field_name == "outline_color")
                    result.outline = Color(static_cast<size_t>(field_value));
                else if (field_name == "outline_width")
                    result.outline_width = Pixels{field_value};
                else if (field_name == "line_width") // acmacs-b3
                    result.outline_width = Pixels{field_value};
                else if (field_name == "shape")
                    result.shape = static_cast<std::string>(field_value);
                else if (field_name == "size")
                    result.size = Pixels{static_cast<double>(field_value) * PointScale};
                else if (field_name == "rotation")
                    result.rotation = Rotation{field_value};
                else if (field_name == "aspect")
                    result.aspect = Aspect{field_value};
                else if (field_name == "show_label")
                    result.label.shown = static_cast<bool>(field_value);
                else if (field_name == "label_position_x")
                    result.label.offset.set().x(field_value);
                else if (field_name == "label_position_y")
                    result.label.offset.set().y(field_value);
                else if (field_name == "label")
                    result.label_text = static_cast<std::string>(field_value);
                else if (field_name == "label_size")
                    result.label.size = Pixels{static_cast<double>(field_value) * LabelScale};
                else if (field_name == "label_color")
                    result.label.color = Color(static_cast<size_t>(field_value));
                else if (field_name == "label_rotation")
                    result.label.rotation = Rotation{field_value};
                else if (field_name == "label_font_face")
                    result.label.style.font_family = static_cast<std::string>(field_value);
                else if (field_name == "label_font_slant")
                    result.label.style.slant = static_cast<std::string>(field_value);
                else if (field_name == "label_font_weight")
                    result.label.style.weight = static_cast<std::string>(field_value);
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [acd1]: point " << aPointNo << " style " << aStyleNo << " field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << rjson::to_string(field_value)
                          << '\n';
            }
        }
    });
    return result;

} // Acd1PlotSpec::extract

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
