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

// std::string InfoModify::name(Compute aCompute) const
// {
//     std::string result{mMain.get_or_default("name", "")};
//     if (result.empty()) {
//         if (const auto& sources{mMain.get_or_empty_array("sources")}; !sources.empty()) {
//             std::vector<std::string> composition;
//             std::transform(std::begin(sources), std::end(sources), std::back_inserter(composition), [](const auto& sinfo) { return sinfo.get_or_default("name", ""); });
//             result = string::join(" + ", composition);
//         }
//     }
//     if (result.empty()) {
//         result = string::join(" ", {virus_not_influenza(aCompute), virus_type(aCompute), subset(aCompute), assay(aCompute), lab(aCompute), rbc_species(aCompute), date(aCompute)});
//     }
//     return result;

// } // InfoModify::name

// // ----------------------------------------------------------------------

// std::string InfoModify::make_field(const char* aField, const char* aSeparator, Compute aCompute) const
// {
//     std::string result{mMain.get_or_default(aField, "")};
//     if (result.empty() && aCompute == Compute::Yes) {
//         if (const auto& sources{mMain.get_or_empty_array("sources")}; !sources.empty()) {
//             std::set<std::string> composition;
//             std::transform(std::begin(sources), std::end(sources), std::inserter(composition, composition.begin()), [aField](const auto& sinfo) { return sinfo.get_or_default(aField, ""); });
//             result = string::join(aSeparator, composition);
//         }
//     }
//     return result;

// } // InfoModify::make_field

// // ----------------------------------------------------------------------

// std::string InfoModify::date(Compute aCompute) const
// {
//     std::string result{mMain.get_or_default("date", "")};
//     if (result.empty() && aCompute == Compute::Yes) {
//         const auto& sources{mMain.get_or_empty_array("sources")};
//         if (!sources.empty()) {
//             std::vector<std::string> composition{sources.size()};
//             std::transform(std::begin(sources), std::end(sources), std::begin(composition), [](const auto& sinfo) { return sinfo.get_or_default("date", ""); });
//             std::sort(std::begin(composition), std::end(composition));
//             result = string::join("-", {composition.front(), composition.back()});
//         }
//     }
//     return result;

// } // InfoModify::date

// // ----------------------------------------------------------------------

// static inline Name make_name(const rjson::object& aData)
// {
//     if (auto name = aData.get_or_default("_name", ""); !name.empty())
//         return name;
//     if (auto isolation_number = aData.get_or_default("isolation_number", ""); !isolation_number.empty()) {
//         std::string host = aData.get_or_default("host", "");
//         if (host == "HUMAN")
//             host.clear();
//         return string::join("/", {aData.get_or_default("virus_type", ""), host, aData.get_or_empty_object("location").get_or_default("name", ""), isolation_number, aData.get_or_default("year", "")});
//     }
//     else if (auto raw_name = aData.get_or_default("raw_name", ""); !raw_name.empty()) {
//         return raw_name;
//     }
//     else {
//         const std::string cdc_abbreviation = aData.get_or_empty_object("location").get_or_default("cdc_abbreviation", "");
//         std::string name = aData.get_or_default("name", "");
//         if (!cdc_abbreviation.empty() && name.size() > 3 && name[2] == '-' && name[0] == cdc_abbreviation[0] && name[1] == cdc_abbreviation[1])
//             name.erase(0, 3);   // old cdc name (acmacs-b?) begins with cdc_abbreviation
//         return string::join(" ", {cdc_abbreviation, name});
//     }
// }

// Name AntigenModify::name() const
// {
//     return make_name(mMain);

// } // AntigenModify::name

// Name SerumModify::name() const
// {
//     return make_name(mMain);

// } // SerumModify::name

// // ----------------------------------------------------------------------

// static inline Passage make_passage(const rjson::object& aData)
// {
//     if (auto [p_dict_present, p_dict] = aData.get_object_if("passage"); p_dict_present) {
//         std::string p = p_dict["passage"];
//         if (auto date = p_dict.get_or_default("date", ""); !date.empty())
//             p += " (" + date + ")";
//         return p;
//     }
//     else if (auto p_str = aData.get_or_default("passage", ""); !p_str.empty()) {
//         return p_str;
//     }
//     else
//         return {};
// }

// Passage AntigenModify::passage() const
// {
//     return make_passage(mMain);

// } // AntigenModify::passage

// Passage SerumModify::passage() const
// {
//     return make_passage(mMain);

// } // SerumModify::passage

// // ----------------------------------------------------------------------

// static inline Reassortant make_reassortant(const rjson::object& aData)
// {
//     if (auto [r_dict_present, r_dict] = aData.get_object_if("reassortant"); r_dict_present) {
//         const rjson::array& complete = r_dict.get_or_empty_array("complete");
//         const rjson::array& incomplete = r_dict.get_or_empty_array("incomplete");
//         std::vector<std::string> composition;
//         std::transform(complete.begin(), complete.end(), std::back_inserter(composition), [](const auto& val) -> std::string { return val; });
//         std::transform(incomplete.begin(), incomplete.end(), std::back_inserter(composition), [](const auto& val) -> std::string { return val; });
//         return string::join(" ", composition);
//     }
//     else if (auto r_str = aData.get_or_default("reassortant", ""); !r_str.empty()) {
//         return r_str;
//     }
//     else
//         return {};
// }

// Reassortant AntigenModify::reassortant() const
// {
//     return make_reassortant(mMain);

// } // AntigenModify::reassortant

// Reassortant SerumModify::reassortant() const
// {
//     return make_reassortant(mMain);

// } // SerumModify::reassortant

// // ----------------------------------------------------------------------

// LabIds AntigenModify::lab_ids() const
// {
//     LabIds result;
//     if (auto [li_present, li] = mMain.get_array_if("lab_id"); li_present) {
//         for(const auto& entry: li)
//             result.push_back(entry[0].str() + '#' + entry[1].str());
//     }
//     return result;

// } // AntigenModify::lab_ids

// // ----------------------------------------------------------------------

// static inline Annotations make_annotations(const rjson::object& aData)
// {
//     Annotations result;
//       // mutations, extra, distinct, annotations, control_duplicate
//     if (aData.get_or_default("distinct", false) || ! aData.get_or_default("control_duplicate", "").empty())
//         result.push_back("DISTINCT");
//     result.push_back(aData.get_or_default("extra", ""));
//     for (const auto& annotation: aData.get_or_empty_array("annotations"))
//         result.push_back(annotation);
//     for (const auto& mutation: aData.get_or_empty_array("mutations"))
//         result.push_back(mutation);
//     return result;
// }

// Annotations AntigenModify::annotations() const
// {
//     return make_annotations(mMain);

// } // AntigenModify::annotations

// Annotations SerumModify::annotations() const
// {
//     return make_annotations(mMain);

// } // SerumModify::annotations

// // ----------------------------------------------------------------------

// BLineage AntigenModify::lineage() const
// {
//     return mMain.get_or_default("lineage", "");

// } // AntigenModify::lineage

// BLineage SerumModify::lineage() const
// {
//     return mMain.get_or_default("lineage", "");

// } // SerumModify::lineage

// // ----------------------------------------------------------------------

// SerumId SerumModify::serum_id() const
// {
//     if (auto [s_dict_present, s_dict] = mMain.get_object_if("serum_id"); s_dict_present) {
//         return s_dict["serum_id"];
//     }
//     else if (auto p_str = mMain.get_or_default("serum_id", ""); !p_str.empty()) {
//         return p_str;
//     }
//     else
//         return {};

// } // SerumModify::serum_id

// // ----------------------------------------------------------------------

// std::optional<size_t> AntigensModify::find_by_full_name(std::string aFullName) const
// {
//     if (mAntigenNameIndex.empty())
//         make_name_index();
//     const std::string name{virus_name::name(aFullName)};
//     if (const auto found = mAntigenNameIndex.find(name); found != mAntigenNameIndex.end()) {
//         for (auto index: found->second) {
//             if (AntigenModify(mMain[index]).full_name() == aFullName)
//                 return index;
//         }
//     }
//     return {};

// } // AntigensModify::find_by_full_name

// // ----------------------------------------------------------------------

// void AntigensModify::make_name_index() const
// {
//     for (auto iter = mMain.begin(); iter != mMain.end(); ++iter) {
//         mAntigenNameIndex[make_name(*iter)].push_back(static_cast<size_t>(iter - mMain.begin()));
//     }

// } // AntigensModify::make_name_index

// // ----------------------------------------------------------------------

// Titer TitersModify::titer(size_t aAntigenNo, size_t aSerumNo) const
// {
//     if (auto [present, list] = mMain.get_array_if("titers_list_of_list"); present) {
//         return list[aAntigenNo][aSerumNo];
//     }
//     else {
//         return titer_in_d(mMain["titers_list_of_dict"], aAntigenNo, aSerumNo);
//     }

// } // TitersModify::titer

// // ----------------------------------------------------------------------

// Titer TitersModify::titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const
// {
//     return titer_in_d(mMain["layers_dict_for_antigen"][aLayerNo], aAntigenNo, aSerumNo);

// } // TitersModify::titer_of_layer

// // ----------------------------------------------------------------------

// size_t TitersModify::number_of_antigens() const
// {
//     if (auto [present, list] = mMain.get_array_if("titers_list_of_list"); present) {
//         // std::cerr << "number_of_antigens " << list << '\n';
//         return list.size();
//     }
//     else {
//         return static_cast<const rjson::array&>(mMain["titers_list_of_dict"]).size();
//     }

// } // TitersModify::number_of_antigens

// // ----------------------------------------------------------------------

// size_t TitersModify::number_of_sera() const
// {
//     if (auto [present, list] = mMain.get_array_if("titers_list_of_list"); present) {
//         // std::cerr << "number_of_sera " << list << '\n';
//         return static_cast<const rjson::array&>(list[0]).size();
//     }
//     else {
//         const rjson::array& d = mMain["titers_list_of_dict"];
//         auto max_index = [](const rjson::object& obj) -> size_t {
//                              size_t result = 0;
//                              for (auto key_value: obj) {
//                                  if (const size_t ind = std::stoul(key_value.first); ind > result)
//                                      result = ind;
//                              }
//                              return result;
//                          };
//         return max_index(*std::max_element(d.begin(), d.end(), [max_index](const rjson::object& a, const rjson::object& b) { return max_index(a) < max_index(b); })) + 1;
//     }

// } // TitersModify::number_of_sera

// // ----------------------------------------------------------------------

// size_t TitersModify::number_of_non_dont_cares() const
// {
//     size_t result = 0;
//     if (auto [present, list] = mMain.get_array_if("titers_list_of_list"); present) {
//         for (const rjson::array& row: list) {
//             for (const Titer titer: row) {
//                 if (!titer.is_dont_care())
//                     ++result;
//             }
//         }
//     }
//     else {
//         const rjson::array& d = mMain["titers_list_of_dict"];
//         result = std::accumulate(d.begin(), d.end(), 0U, [](size_t a, const rjson::object& row) -> size_t { return a + row.size(); });
//     }
//     return result;

// } // TitersModify::number_of_non_dont_cares

// // ----------------------------------------------------------------------

// std::string ProjectionModify::comment() const
// {
//     try {
//         return mMain["comment"];
//     }
//     catch (std::exception&) {
//         return {};
//     }

// } // ProjectionModify::comment

// // ----------------------------------------------------------------------

// class LayoutModify : public acmacs::chart::Layout
// {
//  public:
//     inline LayoutModify(const rjson::object& aData) : mMain{aData} {}

//     size_t number_of_points() const noexcept override
//         {
//             return mMain.get_or_empty_array("layout").size();
//         }

//     inline size_t number_of_dimensions() const noexcept override
//         {
//             try {
//                 for (const rjson::array& row: static_cast<const rjson::array&>(mMain["layout"])) {
//                     if (!row.empty())
//                         return row.size();
//                 }
//             }
//             catch (rjson::field_not_found&) {
//             }
//             catch (std::bad_variant_access&) {
//             }
//             return 0;
//         }

//     Coordinates operator[](size_t aPointNo) const override
//         {
//             const rjson::array& point = mMain.get_or_empty_array("layout")[aPointNo];
//             Coordinates result(number_of_dimensions(), std::numeric_limits<double>::quiet_NaN());
//             std::transform(point.begin(), point.end(), result.begin(), [](const auto& coord) -> double { return coord; });
//             return result;
//         }

//     double coordinate(size_t aPointNo, size_t aDimensionNo) const override
//         {
//             const auto& point = mMain.get_or_empty_array("layout")[aPointNo];
//             try {
//                 return point[aDimensionNo];
//             }
//             catch (std::exception&) {
//                 return std::numeric_limits<double>::quiet_NaN();
//             }
//         }

//     void set(size_t /*aPointNo*/, const Coordinates& /*aCoordinates*/) override { throw acmacs::chart::chart_is_read_only{"LayoutModify::set: cannot modify"}; }

//  private:
//     const rjson::object& mMain;

// }; // class LayoutModify

// // ----------------------------------------------------------------------

// std::shared_ptr<acmacs::chart::Layout> ProjectionModify::layout() const
// {
//     return std::make_shared<LayoutModify>(mMain);

// } // ProjectionModify::layout

// // ----------------------------------------------------------------------

// ColumnBasesP ProjectionModify::forced_column_bases() const
// {
//     const rjson::object& sep = mMain.get_or_empty_object("stress_evaluator_parameters");
//     if (const rjson::array& cb = sep.get_or_empty_array("column_bases"); !cb.empty())
//         return std::make_shared<ColumnBasesModify>(cb);
//     return std::make_shared<ColumnBasesModify>(sep.get_or_empty_array("columns_bases"));

// } // ProjectionModify::forced_column_bases

// // ----------------------------------------------------------------------

// acmacs::Transformation ProjectionModify::transformation() const
// {
//     acmacs::Transformation result;
//     if (auto [present, array] = mMain.get_array_if("transformation"); present) {
//         result.set(array[0][0], array[0][1], array[1][0], array[1][1]);
//     }
//     return result;

// } // ProjectionModify::transformation

// // ----------------------------------------------------------------------

// static inline PointIndexList make_attributes(const rjson::object& aData, size_t aAttr)
// {
//     const rjson::object& attrs = aData.get_or_empty_object("stress_evaluator_parameters").get_or_empty_object("antigens_sera_attributes");
//     PointIndexList result;
//     for (auto [ag_no, attr]: acmacs::enumerate(attrs.get_or_empty_array("antigens"))) {
//         if (static_cast<size_t>(attr) == aAttr)
//             result.push_back(ag_no);
//     }
//     const size_t number_of_antigens = attrs.get_or_empty_array("antigens").size();
//     for (auto [sr_no, attr]: acmacs::enumerate(attrs.get_or_empty_array("sera"))) {
//         if (static_cast<size_t>(attr) == aAttr)
//             result.push_back(sr_no + number_of_antigens);
//     }

//     return result;
// }

// PointIndexList ProjectionModify::unmovable() const
// {
//     return make_attributes(mMain, 1);

// } // ProjectionModify::unmovable

// // ----------------------------------------------------------------------

// PointIndexList ProjectionModify::disconnected() const
// {
//     return make_attributes(mMain, 2);

// } // ProjectionModify::disconnected

// // ----------------------------------------------------------------------

// PointIndexList ProjectionModify::unmovable_in_the_last_dimension() const
// {
//     return make_attributes(mMain, 3);

// } // ProjectionModify::unmovable_in_the_last_dimension

// // ----------------------------------------------------------------------

// AvidityAdjusts ProjectionModify::avidity_adjusts() const
// {
//     if (const rjson::object& titer_multipliers = mMain.get_or_empty_object("stress_evaluator_parameters").get_or_empty_object("antigens_sera_titers_multipliers"); !titer_multipliers.empty()) {
//         const rjson::array& antigens = titer_multipliers.get_or_empty_array("antigens");
//         const rjson::array& sera = titer_multipliers.get_or_empty_array("sera");
//         AvidityAdjusts aa(antigens.size() + sera.size());
//         for (size_t ag_no = 0; ag_no < antigens.size(); ++ag_no)
//             aa[ag_no] = antigens[ag_no];
//         for (size_t sr_no = 0; sr_no < sera.size(); ++sr_no)
//             aa[sr_no + antigens.size()] = sera[sr_no];
//         return aa;
//     }
//     else
//         return {};

// } // ProjectionModify::avidity_adjusts

// // ----------------------------------------------------------------------

// DrawingOrder PlotSpecModify::drawing_order() const
// {
//     DrawingOrder result;
//     if (const rjson::array& do1 = mMain.get_or_empty_array("drawing_order"); !do1.empty()) {
//         for (const rjson::array& do2: do1)
//             for (size_t index: do2)
//                 result.push_back(index);
//     }
//     return result;

// } // PlotSpecModify::drawing_order

// // ----------------------------------------------------------------------

// Color PlotSpecModify::error_line_positive_color() const
// {
//     try {
//         return static_cast<std::string_view>(mMain["error_line_positive"]["color"]);
//     }
//     catch (std::exception&) {
//         return "red";
//     }

// } // PlotSpecModify::error_line_positive_color

// // ----------------------------------------------------------------------

// Color PlotSpecModify::error_line_negative_color() const
// {
//     try {
//         return static_cast<std::string_view>(mMain["error_line_negative"]["color"]);
//     }
//     catch (std::exception&) {
//         return "blue";
//     }

// } // PlotSpecModify::error_line_negative_color

// // ----------------------------------------------------------------------

// acmacs::PointStyle PlotSpecModify::style(size_t aPointNo) const
// {
//     try {
//         const rjson::array& indices = mMain["points"];
//         const size_t style_no = indices[aPointNo];
//         return extract(mMain["styles"][style_no], aPointNo, style_no);
//     }
//     catch (std::exception& err) {
//         std::cerr << "WARNING: [acd1]: cannot get style for point " << aPointNo << ": " << err.what() << '\n';
//     }
//     return {};

// } // PlotSpecModify::style

// // ----------------------------------------------------------------------

// std::vector<acmacs::PointStyle> PlotSpecModify::all_styles() const
// {
//     try {
//         const rjson::array& indices = mMain["points"];
//         std::vector<acmacs::PointStyle> result(indices.size());
//         for (auto [point_no, target]: acmacs::enumerate(result)) {
//             const size_t style_no = indices[point_no];
//             target = extract(mMain["styles"][style_no], point_no, style_no);
//         }
//         return result;
//     }
//     catch (std::exception& err) {
//         std::cerr << "WARNING: [acd1]: cannot get point styles: " << err.what() << '\n';
//     }
//     return {};

// } // PlotSpecModify::all_styles

// // ----------------------------------------------------------------------

// acmacs::PointStyle PlotSpecModify::extract(const rjson::object& aSrc, size_t aPointNo, size_t aStyleNo) const
// {
//     acmacs::PointStyle result;
//     for (auto [field_name_v, field_value]: aSrc) {
//         const std::string_view field_name(field_name_v);
//         try {
//             if (field_name == "shown")
//                 result.shown = field_value;
//             else if (field_name == "fill_color")
//                 result.fill = Color(static_cast<size_t>(field_value));
//             else if (field_name == "outline_color")
//                 result.outline = Color(static_cast<size_t>(field_value));
//             else if (field_name == "outline_width")
//                 result.outline_width = Pixels{field_value};
//             else if (field_name == "line_width") // acmacs-b3
//                 result.outline_width = Pixels{field_value};
//             else if (field_name == "shape")
//                 result.shape = field_value.str();
//             else if (field_name == "size")
//                 result.size = Pixels{static_cast<double>(field_value) * 5.0};
//             else if (field_name == "rotation")
//                 result.rotation = Rotation{field_value};
//             else if (field_name == "aspect")
//                 result.aspect = Aspect{field_value};
//             else if (field_name == "show_label")
//                 result.label.shown = field_value;
//             else if (field_name == "label_position_x")
//                 result.label.offset.set().x = field_value;
//             else if (field_name == "label_position_y")
//                 result.label.offset.set().y = field_value;
//             else if (field_name == "label")
//                 result.label_text = field_value.str();
//             else if (field_name == "label_size")
//                 result.label.size = Pixels{static_cast<double>(field_value) * 10.0};
//             else if (field_name == "label_color")
//                 result.label.color = Color(static_cast<size_t>(field_value));
//             else if (field_name == "label_rotation")
//                 result.label.rotation = Rotation{field_value};
//             else if (field_name == "label_font_face")
//                 result.label.style.font_family = field_value.str();
//             else if (field_name == "label_font_slant")
//                 result.label.style.slant = field_value.str();
//             else if (field_name == "label_font_weight")
//                 result.label.style.weight = field_value.str();
//         }
//         catch (std::exception& err) {
//             std::cerr << "WARNING: [acd1]: point " << aPointNo << " style " << aStyleNo << " field \"" << field_name << "\" value is wrong: " << err.what() << " value: " << field_value.to_json() << '\n';
//         }
//     }
//     return result;

// } // PlotSpecModify::extract

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
