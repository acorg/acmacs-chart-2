#include <set>
#include <map>
#include <vector>
#include <limits>

#include "acmacs-base/timeit.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/virus-name.hh"
#include "acmacs-chart-2/acd1-import.hh"
#include "acmacs-chart-2/rjson-import.hh"

using namespace std::string_literals;
using namespace acmacs::chart;

constexpr const double PointScale = 5.0;
constexpr const double LabelScale = 10.0;

static std::string convert_to_json(const std::string_view& aData);
static void convert_set(std::string& aData, const std::vector<size_t>& aPerhapsSet);

// ----------------------------------------------------------------------

ChartP acmacs::chart::acd1_import(const std::string_view& aData, Verify aVerify)
{
    const std::string json = convert_to_json(aData);
    try {
        auto chart = std::make_shared<Acd1Chart>(rjson::parse_string(json));
        chart->verify_data(aVerify);
        return chart;
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
        // if (static_cast<size_t>(mData["version"]) != 4)
        //     throw import_error("invalid version");
        const auto& antigens = mData["table"].get_or_empty_array("antigens");
        if (antigens.empty())
            throw import_error("no antigens");
        const auto& sera = mData["table"].get_or_empty_array("sera");
        if (sera.empty())
            throw import_error("no sera");
        const auto& titers = mData["table"].get_or_empty_object("titers");
        if (titers.empty())
            throw import_error("no titers");
        if (auto [ll_present, ll] = titers.get_array_if("titers_list_of_list"); ll_present) {
            if (ll.size() != antigens.size())
                throw import_error("number of the titer rows (" + acmacs::to_string(ll.size()) + ") does not correspond to the number of antigens (" + acmacs::to_string(antigens.size()) + ")");
        }
        else if (auto [dd_present, dd] = titers.get_array_if("titers_list_of_dict"); dd_present) {
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
    return std::make_shared<Acd1Info>(mData["chart_info"]);

} // Acd1Chart::info

// ----------------------------------------------------------------------

AntigensP Acd1Chart::antigens() const
{
    return std::make_shared<Acd1Antigens>(mData["table"].get_or_empty_array("antigens"), mAntigenNameIndex);

} // Acd1Chart::antigens

// ----------------------------------------------------------------------

SeraP Acd1Chart::sera() const
{
    auto sera = std::make_shared<Acd1Sera>(mData["table"].get_or_empty_array("sera"));
    set_homologous(false, sera);
    return sera;

} // Acd1Chart::sera

// ----------------------------------------------------------------------

TitersP Acd1Chart::titers() const
{
    return std::make_shared<Acd1Titers>(mData["table"].get_or_empty_object("titers"));

} // Acd1Chart::titers

// ----------------------------------------------------------------------

ColumnBasesP Acd1Chart::forced_column_bases() const
{
    if (const auto& cb = mData["table"].get_or_empty_array("column_bases"); !cb.empty())
        return std::make_shared<Acd1ColumnBases>(cb);
    return nullptr;

} // Acd1Chart::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP Acd1Chart::projections() const
{
    return std::make_shared<Acd1Projections>(mData.get_or_empty_array("projections"));

} // Acd1Chart::projections

// ----------------------------------------------------------------------

PlotSpecP Acd1Chart::plot_spec() const
{
    return std::make_shared<Acd1PlotSpec>(mData.get_or_empty_object("plot_spec"), *this);

} // Acd1Chart::plot_spec

// ----------------------------------------------------------------------

bool Acd1Chart::is_merge() const
{
    return !mData["table"].get_or_empty_object("titers").get_or_empty_array("layers_dict_for_antigen").empty();

} // Acd1Chart::is_merge

// ----------------------------------------------------------------------

std::string Acd1Info::name(Compute aCompute) const
{
    std::string result{mData.get_or_default("name", "")};
    if (result.empty()) {
        if (const auto& sources{mData.get_or_empty_array("sources")}; !sources.empty()) {
            std::vector<std::string> composition;
            std::transform(std::begin(sources), std::end(sources), std::back_inserter(composition), [](const auto& sinfo) { return sinfo.get_or_default("name", ""); });
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
    std::string result{mData.get_or_default(aField, "")};
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources{mData.get_or_empty_array("sources")}; !sources.empty()) {
            std::set<std::string> composition;
            std::transform(std::begin(sources), std::end(sources), std::inserter(composition, composition.begin()), [aField](const auto& sinfo) { return sinfo.get_or_default(aField, ""); });
            result = string::join(aSeparator, composition);
        }
    }
    return result;

} // Acd1Info::make_field

// ----------------------------------------------------------------------

std::string Acd1Info::date(Compute aCompute) const
{
    std::string result{mData.get_or_default("date", "")};
    if (result.empty() && aCompute == Compute::Yes) {
        const auto& sources{mData.get_or_empty_array("sources")};
        if (!sources.empty()) {
            std::vector<std::string> composition{sources.size()};
            std::transform(std::begin(sources), std::end(sources), std::begin(composition), [](const auto& sinfo) { return sinfo.get_or_default("date", ""); });
            std::sort(std::begin(composition), std::end(composition));
            result = string::join("-", {composition.front(), composition.back()});
        }
    }
    return result;

} // Acd1Info::date

// ----------------------------------------------------------------------

static inline Name make_name(const rjson::object& aData)
{
    if (auto name = aData.get_or_default("_name", ""); !name.empty())
        return name;
    if (auto isolation_number = aData.get_or_default("isolation_number", ""); !isolation_number.empty()) {
        std::string host = aData.get_or_default("host", "");
        if (host == "HUMAN")
            host.clear();
        return string::join("/", {aData.get_or_default("virus_type", ""), host, aData.get_or_empty_object("location").get_or_default("name", ""), isolation_number, aData.get_or_default("year", "")});
    }
    else if (auto raw_name = aData.get_or_default("raw_name", ""); !raw_name.empty()) {
        return raw_name;
    }
    else {
        const std::string cdc_abbreviation = aData.get_or_empty_object("location").get_or_default("cdc_abbreviation", "");
        std::string name = aData.get_or_default("name", "");
        if (!cdc_abbreviation.empty() && name.size() > 3 && name[2] == '-' && name[0] == cdc_abbreviation[0] && name[1] == cdc_abbreviation[1])
            name.erase(0, 3);   // old cdc name (acmacs-b?) begins with cdc_abbreviation
        return string::join(" ", {cdc_abbreviation, name});
    }
}

Name Acd1Antigen::name() const
{
    return make_name(mData);

} // Acd1Antigen::name

Name Acd1Serum::name() const
{
    return make_name(mData);

} // Acd1Serum::name

// ----------------------------------------------------------------------

static inline Passage make_passage(const rjson::object& aData)
{
    if (auto [p_dict_present, p_dict] = aData.get_object_if("passage"); p_dict_present) {
        std::string p = p_dict["passage"];
        if (auto date = p_dict.get_or_default("date", ""); !date.empty())
            p += " (" + date + ")";
        return p;
    }
    else if (auto p_str = aData.get_or_default("passage", ""); !p_str.empty()) {
        return p_str;
    }
    else
        return {};
}

Passage Acd1Antigen::passage() const
{
    return make_passage(mData);

} // Acd1Antigen::passage

Passage Acd1Serum::passage() const
{
    return make_passage(mData);

} // Acd1Serum::passage

// ----------------------------------------------------------------------

static inline Reassortant make_reassortant(const rjson::object& aData)
{
    if (auto [r_dict_present, r_dict] = aData.get_object_if("reassortant"); r_dict_present) {
        const rjson::array& complete = r_dict.get_or_empty_array("complete");
        const rjson::array& incomplete = r_dict.get_or_empty_array("incomplete");
        std::vector<std::string> composition;
        std::transform(complete.begin(), complete.end(), std::back_inserter(composition), [](const auto& val) -> std::string { return val; });
        std::transform(incomplete.begin(), incomplete.end(), std::back_inserter(composition), [](const auto& val) -> std::string { return val; });
        return string::join(" ", composition);
    }
    else if (auto r_str = aData.get_or_default("reassortant", ""); !r_str.empty()) {
        return r_str;
    }
    else
        return {};
}

Reassortant Acd1Antigen::reassortant() const
{
    return make_reassortant(mData);

} // Acd1Antigen::reassortant

Reassortant Acd1Serum::reassortant() const
{
    return make_reassortant(mData);

} // Acd1Serum::reassortant

// ----------------------------------------------------------------------

LabIds Acd1Antigen::lab_ids() const
{
    LabIds result;
    if (auto [li_present, li] = mData.get_array_if("lab_id"); li_present) {
        for(const auto& entry: li)
            result.push_back(entry[0].str() + '#' + entry[1].str());
    }
    return result;

} // Acd1Antigen::lab_ids

// ----------------------------------------------------------------------

static inline Annotations make_annotations(const rjson::object& aData)
{
    Annotations result;
      // mutations, extra, distinct, annotations, control_duplicate
    if (aData.get_or_default("distinct", false) || aData.get_or_default("DISTINCT", false) || !aData.get_or_default("control_duplicate", "").empty() || !aData.get_or_default("CONTROL_DUPLICATE", "").empty())
        result.push_back("DISTINCT");
    result.push_back(aData.get_or_default("extra", ""));
    result.push_back(aData.get_or_default("EXTRA", ""));
    for (const auto& annotation: aData.get_or_empty_array("annotations"))
        result.push_back(annotation);
    for (const auto& mutation: aData.get_or_empty_array("mutations"))
        result.push_back(mutation);
    return result;
}

Annotations Acd1Antigen::annotations() const
{
    return make_annotations(mData);

} // Acd1Antigen::annotations

Annotations Acd1Serum::annotations() const
{
    return make_annotations(mData);

} // Acd1Serum::annotations

// ----------------------------------------------------------------------

BLineage Acd1Antigen::lineage() const
{
    return mData.get_or_default("lineage", "");

} // Acd1Antigen::lineage

BLineage Acd1Serum::lineage() const
{
    return mData.get_or_default("lineage", "");

} // Acd1Serum::lineage

// ----------------------------------------------------------------------

SerumId Acd1Serum::serum_id() const
{
    if (auto [s_dict_present, s_dict] = mData.get_object_if("serum_id"); s_dict_present) {
        return s_dict["serum_id"];
    }
    else if (auto p_str = mData.get_or_default("serum_id", ""); !p_str.empty()) {
        return p_str;
    }
    else
        return {};

} // Acd1Serum::serum_id

// ----------------------------------------------------------------------

std::optional<size_t> Acd1Antigens::find_by_full_name(std::string aFullName) const
{
    if (mAntigenNameIndex.empty())
        make_name_index();
    const std::string name{virus_name::name(aFullName)};
    if (const auto found = mAntigenNameIndex.find(name); found != mAntigenNameIndex.end()) {
        for (auto index: found->second) {
            if (Acd1Antigen(mData[index]).full_name() == aFullName)
                return index;
        }
    }
    return {};

} // Acd1Antigens::find_by_full_name

// ----------------------------------------------------------------------

void Acd1Antigens::make_name_index() const
{
    for (auto iter = mData.begin(); iter != mData.end(); ++iter) {
        mAntigenNameIndex[make_name(*iter)].push_back(static_cast<size_t>(iter - mData.begin()));
    }

} // Acd1Antigens::make_name_index

// ----------------------------------------------------------------------

Titer Acd1Titers::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    if (auto [present, list] = mData.get_array_if("titers_list_of_list"); present) {
        return list[aAntigenNo][aSerumNo];
    }
    else {
        return rjson_import::titer_in_d(mData["titers_list_of_dict"], aAntigenNo, aSerumNo);
    }

} // Acd1Titers::titer

// ----------------------------------------------------------------------

Titer Acd1Titers::titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const
{
    return rjson_import::titer_in_d(mData["layers_dict_for_antigen"][aLayerNo], aAntigenNo, aSerumNo);

} // Acd1Titers::titer_of_layer

// ----------------------------------------------------------------------

std::vector<Titer> Acd1Titers::titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const
{
    return rjson_import::titers_for_layers(mData.get_or_empty_array("layers_dict_for_antigen"), aAntigenNo, aSerumNo);

} // Acd1Titers::titers_for_layers

// ----------------------------------------------------------------------

size_t Acd1Titers::number_of_antigens() const
{
    return rjson_import::number_of_antigens(mData, "titers_list_of_list", "titers_list_of_dict");

} // Acd1Titers::number_of_antigens

// ----------------------------------------------------------------------

size_t Acd1Titers::number_of_sera() const
{
    return rjson_import::number_of_sera(mData, "titers_list_of_list", "titers_list_of_dict");

} // Acd1Titers::number_of_sera

// ----------------------------------------------------------------------

size_t Acd1Titers::number_of_non_dont_cares() const
{
    return rjson_import::number_of_non_dont_cares(mData, "titers_list_of_list", "titers_list_of_dict");

} // Acd1Titers::number_of_non_dont_cares

// ----------------------------------------------------------------------

void Acd1Titers::update(TableDistances<float>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust) const
{
    if (number_of_sera()) {
        rjson_import::update(mData, "titers_list_of_list", "titers_list_of_dict", table_distances, column_bases, disconnected, dodgy_titer_is_regular, multiply_antigen_titer_until_column_adjust);
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
    }

} // Acd1Titers::update

// ----------------------------------------------------------------------

void Acd1Titers::update(TableDistances<double>& table_distances, const ColumnBases& column_bases, const PointIndexList& disconnected, bool dodgy_titer_is_regular, bool multiply_antigen_titer_until_column_adjust) const
{
    if (number_of_sera()) {
        rjson_import::update(mData, "titers_list_of_list", "titers_list_of_dict", table_distances, column_bases, disconnected, dodgy_titer_is_regular, multiply_antigen_titer_until_column_adjust);
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
    }

} // Acd1Titers::update

// ----------------------------------------------------------------------

std::string Acd1Projection::comment() const
{
    try {
        return mData["comment"];
    }
    catch (std::exception&) {
        return {};
    }

} // Acd1Projection::comment

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::Layout> Acd1Projection::layout() const
{
    return std::make_shared<rjson_import::Layout>(mData.get_or_empty_array("layout"));

} // Acd1Projection::layout

// ----------------------------------------------------------------------

size_t Acd1Projection::number_of_dimensions() const
{
    return rjson_import::number_of_dimensions(mData["layout"]);

} // Acd1Projection::number_of_dimensions

// ----------------------------------------------------------------------

ColumnBasesP Acd1Projection::forced_column_bases() const
{
    const rjson::object& sep = mData.get_or_empty_object("stress_evaluator_parameters");
    if (const rjson::array& cb = sep.get_or_empty_array("column_bases"); !cb.empty())
        return std::make_shared<Acd1ColumnBases>(cb);
    if (const rjson::array& cb = sep.get_or_empty_array("columns_bases"); !cb.empty())
        return std::make_shared<Acd1ColumnBases>(cb);
    return nullptr;

} // Acd1Projection::forced_column_bases

// ----------------------------------------------------------------------

acmacs::Transformation Acd1Projection::transformation() const
{
    acmacs::Transformation result;
    if (auto [present, array] = mData.get_array_if("transformation"); present) {
        result.set(array[0][0], array[0][1], array[1][0], array[1][1]);
    }
    return result;

} // Acd1Projection::transformation

// ----------------------------------------------------------------------

static inline PointIndexList make_attributes(const rjson::object& aData, size_t aAttr)
{
    const rjson::object& attrs = aData.get_or_empty_object("stress_evaluator_parameters").get_or_empty_object("antigens_sera_attributes");
    PointIndexList result;
    for (auto [ag_no, attr]: acmacs::enumerate(attrs.get_or_empty_array("antigens"))) {
        if (static_cast<size_t>(attr) == aAttr)
            result.push_back(ag_no);
    }
    const size_t number_of_antigens = attrs.get_or_empty_array("antigens").size();
    for (auto [sr_no, attr]: acmacs::enumerate(attrs.get_or_empty_array("sera"))) {
        if (static_cast<size_t>(attr) == aAttr)
            result.push_back(sr_no + number_of_antigens);
    }

    return result;
}

PointIndexList Acd1Projection::unmovable() const
{
    return make_attributes(mData, 1);

} // Acd1Projection::unmovable

// ----------------------------------------------------------------------

PointIndexList Acd1Projection::disconnected() const
{
    return make_attributes(mData, 2);

} // Acd1Projection::disconnected

// ----------------------------------------------------------------------

PointIndexList Acd1Projection::unmovable_in_the_last_dimension() const
{
    return make_attributes(mData, 3);

} // Acd1Projection::unmovable_in_the_last_dimension

// ----------------------------------------------------------------------

AvidityAdjusts Acd1Projection::avidity_adjusts() const
{
    if (const rjson::object& titer_multipliers = mData.get_or_empty_object("stress_evaluator_parameters").get_or_empty_object("antigens_sera_titers_multipliers"); !titer_multipliers.empty()) {
        const rjson::array& antigens = titer_multipliers.get_or_empty_array("antigens");
        const rjson::array& sera = titer_multipliers.get_or_empty_array("sera");
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
    if (const rjson::array& do1 = mData.get_or_empty_array("drawing_order"); !do1.empty()) {
        for (const rjson::array& do2: do1)
            for (size_t index: do2)
                result.push_back(index);
    }
    return result;

} // Acd1PlotSpec::drawing_order

// ----------------------------------------------------------------------

Color Acd1PlotSpec::error_line_positive_color() const
{
    try {
        return static_cast<std::string_view>(mData["error_line_positive"]["color"]);
    }
    catch (std::exception&) {
        return "red";
    }

} // Acd1PlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color Acd1PlotSpec::error_line_negative_color() const
{
    try {
        return static_cast<std::string_view>(mData["error_line_negative"]["color"]);
    }
    catch (std::exception&) {
        return "blue";
    }

} // Acd1PlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle Acd1PlotSpec::style(size_t aPointNo) const
{
    try {
        const rjson::array& indices = mData["points"];
        const size_t style_no = indices[aPointNo];
        return extract(mData["styles"][style_no], aPointNo, style_no);
    }
    catch (std::exception& /*err*/) {
        // std::cerr << "WARNING: [acd1]: cannot get style for point " << aPointNo << ": " << err.what() << '\n';
    }
    return mChart.default_style(aPointNo);

} // Acd1PlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> Acd1PlotSpec::all_styles() const
{
    const rjson::array& indices = mData.get_or_empty_array("points");
    if (!indices.empty()) {
        std::vector<acmacs::PointStyle> result(indices.size());
        for (auto [point_no, target]: acmacs::enumerate(result)) {
            try {
                const size_t style_no = indices[point_no];
                target = extract(mData["styles"][style_no], point_no, style_no);
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

acmacs::PointStyle Acd1PlotSpec::extract(const rjson::object& aSrc, size_t aPointNo, size_t aStyleNo) const
{
    acmacs::PointStyle result;
    for (auto [field_name_v, field_value]: aSrc) {
        const std::string_view field_name(field_name_v);
        try {
            if (field_name == "shown")
                result.shown = field_value;
            else if (field_name == "fill_color")
                result.fill = Color(static_cast<size_t>(field_value));
            else if (field_name == "outline_color")
                result.outline = Color(static_cast<size_t>(field_value));
            else if (field_name == "outline_width")
                result.outline_width = Pixels{field_value};
            else if (field_name == "line_width") // acmacs-b3
                result.outline_width = Pixels{field_value};
            else if (field_name == "shape")
                result.shape = field_value.str();
            else if (field_name == "size")
                result.size = Pixels{static_cast<double>(field_value) * PointScale};
            else if (field_name == "rotation")
                result.rotation = Rotation{field_value};
            else if (field_name == "aspect")
                result.aspect = Aspect{field_value};
            else if (field_name == "show_label")
                result.label.shown = field_value;
            else if (field_name == "label_position_x")
                result.label.offset.set().x = field_value;
            else if (field_name == "label_position_y")
                result.label.offset.set().y = field_value;
            else if (field_name == "label")
                result.label_text = field_value.str();
            else if (field_name == "label_size")
                result.label.size = Pixels{static_cast<double>(field_value) * LabelScale};
            else if (field_name == "label_color")
                result.label.color = Color(static_cast<size_t>(field_value));
            else if (field_name == "label_rotation")
                result.label.rotation = Rotation{field_value};
            else if (field_name == "label_font_face")
                result.label.style.font_family = field_value.str();
            else if (field_name == "label_font_slant")
                result.label.style.slant = field_value.str();
            else if (field_name == "label_font_weight")
                result.label.style.weight = field_value.str();
        }
        catch (std::exception& err) {
            std::cerr << "WARNING: [acd1]: point " << aPointNo << " style " << aStyleNo << " field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << field_value.to_json() << '\n';
        }
    }
    return result;

} // Acd1PlotSpec::extract

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
