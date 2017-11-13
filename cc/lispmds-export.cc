#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart/lispmds-export.hh"
#include "acmacs-chart/lispmds-encode.hh"
#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

static std::string antigen_names(std::shared_ptr<acmacs::chart::Antigens> aAntigens);
static std::string serum_names(std::shared_ptr<acmacs::chart::Sera> aSera);
static std::string titers(std::shared_ptr<acmacs::chart::Titers> aTiters);
static std::string starting_coordss(std::shared_ptr<acmacs::chart::Chart> aChart);
static std::string batch_runs(std::shared_ptr<acmacs::chart::Chart> aChart);
static std::string coordinates(std::shared_ptr<acmacs::chart::Projection> aProjection, size_t number_of_points, size_t number_of_dimensions, size_t aIndent);
static std::string col_and_row_adjusts(std::shared_ptr<acmacs::chart::Chart> aChart, std::shared_ptr<acmacs::chart::Projection> aProjection, size_t aIndent);
static std::string reference_antigens(std::shared_ptr<acmacs::chart::Chart> aChart);
static std::string plot_spec(std::shared_ptr<acmacs::chart::Chart> aChart);

// static void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo);
// static void export_plot_spec(rjson::object& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec);
// static void compact_styles(const std::vector<acmacs::PointStyle>& aAllStyles, std::vector<acmacs::PointStyle>& aCompacted, std::vector<size_t>& aIndex);
// static void export_style(rjson::array& target_styles, const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_export(std::shared_ptr<acmacs::chart::Chart> aChart, std::string /*aProgramName*/)
{
    std::string result = R"(;; MDS configuration file (version acmacs-d-1).
;; Created on )" + acmacs::time_format() + R"(

(MAKE-MASTER-MDS-WINDOW
   (HI-IN '()" + antigen_names(aChart->antigens()) + R"()
          '()" + serum_names(aChart->sera()) + R"()
          '()" + titers(aChart->titers()) + R"()
          ')" + lispmds_encode(aChart->info()->name_non_empty()) + R"()
  )" + starting_coordss(aChart) + R"(
  )" + batch_runs(aChart) + R"(
)";
    if (auto projections = aChart->projections(); !projections->empty()) {
        auto projection = (*projections)[0];
        result.append("  :MDS-DIMENSIONS '" + acmacs::to_string(projection->number_of_dimensions()) + '\n');
        result.append("  :MOVEABLE-COORDS 'ALL\n  :UNMOVEABLE-COORDS '");
        if (auto unmovable = projection->unmovable(); !unmovable.empty()) {
            result
                    .append(1, '(')
                    .append(string::join(" ", unmovable.begin(), unmovable.end(), [](auto index) -> std::string { return acmacs::to_string(index); }))
                    .append(1, ')');
        }
        else
            result.append("NIL");
        result.append(R"(
  :CANVAS-COORD-TRANSFORMATIONS '(:CANVAS-WIDTH 800 :CANVAS-HEIGHT 800 :CANVAS-X-COORD-TRANSLATION 0.0 :CANVAS-Y-COORD-TRANSLATION 0.0
                                  :CANVAS-X-COORD-SCALE 32.94357699173962d0 :CANVAS-Y-COORD-SCALE 32.94357699173962d0
                                  :FIRST-DIMENSION 0 :SECOND-DIMENSION 1 :BASIS-VECTOR-POINT-INDICES (0 1 2) :BASIS-VECTOR-POINT-INDICES-BACKUP NIL
                                  :BASIS-VECTOR-X-COORD-TRANSLATION 0 :BASIS-VECTOR-Y-COORD-TRANSLATION 0
                                  :TRANSLATE-TO-FIT-MDS-WINDOW T :SCALE-TO-FIT-MDS-WINDOW T
                                  :BASIS-VECTOR-X-COORD-SCALE 1 :BASIS-VECTOR-Y-COORD-SCALE 1
)");
        auto transformation = projection->transformation();
        result.append("                                  :CANVAS-BASIS-VECTOR-0 (" + acmacs::to_string(transformation.a) + ' ' + acmacs::to_string(transformation.c) + ") :CANVAS-BASIS-VECTOR-1 (" + acmacs::to_string(transformation.b) + ' ' + acmacs::to_string(transformation.d) + "))\n");
    }
    result.append(reference_antigens(aChart));
    result.append(plot_spec(aChart));
    result.append(1, ')');
    return result;

} // acmacs::chart::lispmds_export

// ----------------------------------------------------------------------

std::string antigen_names(std::shared_ptr<acmacs::chart::Antigens> aAntigens)
{
    return string::join(" ", aAntigens->begin(), aAntigens->end(), [](auto antigen) { return lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations()); });

} // antigen_names

// ----------------------------------------------------------------------

std::string serum_names(std::shared_ptr<acmacs::chart::Sera> aSera)
{
    return string::join(" ", aSera->begin(), aSera->end(), [](auto serum) { return lispmds_serum_name_encode(serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id()); });

} // serum_names

// ----------------------------------------------------------------------

std::string reference_antigens(std::shared_ptr<acmacs::chart::Chart> aChart)
{
    auto antigens = aChart->antigens();
    return "  :REFERENCE-ANTIGENS '("
            + string::join(" ", antigens->begin(), antigens->end(), [](auto antigen) { return antigen->reference() ? lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations()) : std::string{}; })
            + ")\n";

} // reference_antigens

// ----------------------------------------------------------------------

std::string titers(std::shared_ptr<acmacs::chart::Titers> aTiters)
{
    std::string result;
    const size_t number_of_antigens = aTiters->number_of_antigens();
    const size_t number_of_sera = aTiters->number_of_sera();

    for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
        if (ag_no)
            result.append(1, '\n').append(12, ' ');
        result.append(1, '(');
        for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
            if (sr_no)
                result.append(1, ' ');
            result.append(aTiters->titer(ag_no, sr_no).logged_as_string());
        }
        result.append(1, ')');
    }

    return result;

} // titers

// ----------------------------------------------------------------------

std::string starting_coordss(std::shared_ptr<acmacs::chart::Chart> aChart)
{
    auto projections = aChart->projections();
    if (projections->empty())
        return {};
    auto projection = (*projections)[0];
    if (const auto number_of_points = projection->number_of_points(), number_of_dimensions = projection->number_of_dimensions(); number_of_points && number_of_dimensions)
        return "  :STARTING-COORDSS '(" + coordinates(projection, number_of_points, number_of_dimensions, 22) + col_and_row_adjusts(aChart, projection, 22) + ')';
    else
        return {};

} // starting_coordss

// ----------------------------------------------------------------------

std::string batch_runs(std::shared_ptr<acmacs::chart::Chart> aChart)
{
    auto projections = aChart->projections();
    if (projections->size() < 2)
        return {};
    std::string result = "  :BATCH-RUNS '(";
    for (size_t projection_no = 1; projection_no < projections->size(); ++projection_no) {
        auto projection = (*projections)[projection_no];
        if (projection_no > 1)
            result.append("\n                ");
        result.append("((" + coordinates(projection, projection->number_of_points(), projection->number_of_dimensions(), 18) + col_and_row_adjusts(aChart, projection, 18) + ')');
        result.append("\n                 " + acmacs::to_string(projection->stress()) + " MULTIPLE-END-CONDITIONS NIL)");
    }
    result.append(1, ')');
    return result;

} // batch_runs

// ----------------------------------------------------------------------

std::string coordinates(std::shared_ptr<acmacs::chart::Projection> aProjection, size_t number_of_points, size_t number_of_dimensions, size_t aIndent)
{
    std::string result;
    for (size_t point_no = 0; point_no < number_of_points; ++point_no) {
        if (point_no)
            result.append(1, '\n').append(aIndent, ' ');
        result.append(1, '(');
        for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
            if (dim)
                result.append(1, ' ');
            const auto c = aProjection->coordinate(point_no, dim);
            if (std::isnan(c))
                result.append("-1000"); // disconnected point
            else
                result.append(acmacs::to_string(c));
        }
        result.append(1, ')');
    }
    return result;

} // coordinates

// ----------------------------------------------------------------------

std::string col_and_row_adjusts(std::shared_ptr<acmacs::chart::Chart> aChart, std::shared_ptr<acmacs::chart::Projection> aProjection, size_t aIndent)
{
    std::string result{"\n"};
    result
            .append(aIndent, ' ')
            .append("((COL-AND-ROW-ADJUSTS\n")
            .append(aIndent + 2, ' ')
            .append(1, '(');
    const auto number_of_antigens = aChart->number_of_antigens();
    const auto number_of_sera = aChart->number_of_sera();
    for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
        if (ag_no)
            result.append(1, ' ');
        result.append("-1.0d+7");
    }
    result.append(1, '\n').append(aIndent + 3, ' ');
    auto cb = aProjection->forced_column_bases();
    if (!cb->exists())
        cb = aChart->computed_column_bases(aProjection->minimum_column_basis());
    for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
        if (sr_no)
            result.append(1, ' ');
        result.append(acmacs::to_string(cb->column_basis(sr_no)));
    }
    result.append(1, '\n').append(aIndent + 2, ' ');
    if (auto avidity_adjusts = aProjection->avidity_adjusts(); !avidity_adjusts.empty()) {
        for (double aa: avidity_adjusts)
            result.append(1, ' ').append(acmacs::to_string(std::log2(aa)));
    }
    else {
        for (size_t point_no = 0; point_no < (number_of_antigens + number_of_sera); ++point_no)
            result.append(" 0");
    }
    result.append(1, '\n').append(aIndent + 3, ' ');
    result.append(")))");
    return result;

} // col_and_row_adjusts

// ----------------------------------------------------------------------

std::string plot_spec(std::shared_ptr<acmacs::chart::Chart> aChart)
{
    std::string result;
    return result;

  // :RAISE-POINTS 'NIL
  // :LOWER-POINTS 'NIL
              // :PLOT-SPEC '((BI/16190/68-AG :CO "#ffdba5" :DS 4 :TR 0.0 :NM "BI/16190/68" :WN "BI/16190/68" :NS 10 :NC "#ffdba5" :SH "CIRCLE")

} // plot_spec

// ----------------------------------------------------------------------

// void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo)
// {
//     aTarget.set_field_if_not_empty("v", aInfo->virus());
//     aTarget.set_field_if_not_empty("V", aInfo->virus_type());
//     aTarget.set_field_if_not_empty("A", aInfo->assay());
//     aTarget.set_field_if_not_empty("D", aInfo->date());
//     aTarget.set_field_if_not_empty("N", aInfo->name());
//     aTarget.set_field_if_not_empty("l", aInfo->lab());
//     aTarget.set_field_if_not_empty("r", aInfo->rbc_species());
//     aTarget.set_field_if_not_empty("s", aInfo->subset());
//       //aTarget.set_field_if_not_empty("T", aInfo->table_type());

//     const auto number_of_sources = aInfo->number_of_sources();
//     if (number_of_sources) {
//         rjson::array& array = aTarget.set_field("S", rjson::array{});
//         for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
//             export_info(array.insert(rjson::object{}), aInfo->source(source_no));
//         }
//     }

// } // export_info

// // ----------------------------------------------------------------------

// void export_projections(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Projections> aProjections)
// {
//     for (const auto projection: *aProjections) {
//         rjson::object& target = aTarget.insert(rjson::object{});

//         if (const auto number_of_points = projection->number_of_points(), number_of_dimensions = projection->number_of_dimensions(); number_of_points && number_of_dimensions) {
//             rjson::array& ar = target.set_field("l", rjson::array{});
//             for (size_t p_no = 0; p_no < number_of_points; ++p_no) {
//                 rjson::array& p = ar.insert(rjson::array{});
//                 for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
//                     const auto c = projection->coordinate(p_no, dim);
//                     if (std::isnan(c))
//                         break;
//                     p.insert(rjson::to_value(c));
//                 }
//             }
//         }

//         target.set_field_if_not_empty("c", static_cast<const std::string&>(projection->comment()));
//         target.set_field_if_not_default("s", projection->stress(), 0.0);
//         if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
//             target.set_field("m", rjson::string{minimum_column_basis});
//         if (const auto forced_column_bases = projection->forced_column_bases(); forced_column_bases->exists()) {
//             rjson::array& ar = target.set_field("C", rjson::array{});
//             for (size_t sr_no = 0; sr_no < forced_column_bases->size(); ++sr_no)
//                 ar.insert(rjson::to_value(forced_column_bases->column_basis(sr_no)));
//         }
//         if (const auto transformation = projection->transformation(); transformation != acmacs::Transformation{})
//             target.set_field("t", rjson::array{transformation.a, transformation.b, transformation.c, transformation.d});
//         target.set_field_if_not_default("d", projection->dodgy_titer_is_regular(), false);
//         target.set_field_if_not_default("e", projection->stress_diff_to_stop(), 0.0);
//         if (const auto unmovable = projection->unmovable(); ! unmovable.empty())
//             target.set_field("U", rjson::array(rjson::array::use_iterator, unmovable.begin(), unmovable.end()));
//         if (const auto disconnected = projection->disconnected(); ! disconnected.empty())
//             target.set_field("D", rjson::array(rjson::array::use_iterator, disconnected.begin(), disconnected.end()));
//         if (const auto unmovable_in_the_last_dimension = projection->unmovable_in_the_last_dimension(); ! unmovable_in_the_last_dimension.empty())
//             target.set_field("u", rjson::array(rjson::array::use_iterator, unmovable_in_the_last_dimension.begin(), unmovable_in_the_last_dimension.end()));

//         // "i": 600,               // number of iterations?
//         // "g": [],            // antigens_sera_gradient_multipliers, double for each point
//         // "f": [],            // antigens_sera_titers_multipliers, double for each point
//     }

// } // export_projections

// // ----------------------------------------------------------------------

// void export_plot_spec(rjson::object& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec)
// {
//     if (const auto drawing_order = aPlotSpec->drawing_order(); ! drawing_order.empty())
//         aTarget.set_field("d", rjson::array(rjson::array::use_iterator, drawing_order.begin(), drawing_order.end()));
//     if (const auto color = aPlotSpec->error_line_positive_color(); color != RED)
//         aTarget.set_field("E", rjson::object{{{"c", rjson::string{color}}}});
//     if (const auto color = aPlotSpec->error_line_negative_color(); color != BLUE)
//         aTarget.set_field("e", rjson::object{{{"c", rjson::string{color}}}});

//     std::vector<acmacs::PointStyle> compacted;
//     std::vector<size_t> p_index;
//     compact_styles(aPlotSpec->all_styles(), compacted, p_index);
//     aTarget.set_field("p", rjson::array(rjson::array::use_iterator, p_index.begin(), p_index.end()));
//     rjson::array& target_styles = aTarget.set_field("P", rjson::array{});
//     for (const auto& style: compacted)
//         export_style(target_styles, style);

//       // "g": {},                  // ? grid data
//       // "l": [],                  // ? for each procrustes line, index in the "L" list
//       // "L": []                    // ? list of procrustes lines styles
//       // "s": [],                  // list of point indices for point shown on all maps in the time series
//       // "t": {}                    // title style?

// } // export_plot_spec

// // ----------------------------------------------------------------------

// void compact_styles(const std::vector<acmacs::PointStyle>& aAllStyles, std::vector<acmacs::PointStyle>& aCompacted, std::vector<size_t>& aIndex)
// {
//     for (const auto& style: aAllStyles) {
//         if (auto found = std::find(aCompacted.begin(), aCompacted.end(), style); found == aCompacted.end()) {
//             aCompacted.push_back(style);
//             aIndex.push_back(aCompacted.size() - 1);
//         }
//         else {
//             aIndex.push_back(static_cast<size_t>(found - aCompacted.begin()));
//         }
//     }

// } // compact_styles

// // ----------------------------------------------------------------------

// namespace rjson
// {
//     template <> struct content_type<Color> { using type = rjson::string; };
//     template <> struct content_type<acmacs::PointShape> { using type = rjson::string; };
//     template <> struct content_type<acmacs::FontSlant> { using type = rjson::string; };
//     template <> struct content_type<acmacs::FontWeight> { using type = rjson::string; };

//     template <char Tag> inline value to_value(_acmacs_base_internal::SizeScale<Tag> aValue) { return to_value(aValue.value()); }

//     inline value to_value(const acmacs::Offset aValue) { return array{aValue.x, aValue.y}; }

// } // namespace rjson

// template <typename T> inline void set_field(rjson::object& target, const char* name, const acmacs::internal::field_optional_with_default<T>& field)
// {
//     if (field.not_default())
//         target.set_field(name, rjson::to_value(*field));
// }

// void export_style(rjson::array& target_styles, const acmacs::PointStyle& aStyle)
// {
//     rjson::object& st = target_styles.insert(rjson::object{});
//     set_field(st, "+", aStyle.shown);
//     set_field(st, "F", aStyle.fill);
//     set_field(st, "O", aStyle.outline);
//     set_field(st, "o", aStyle.outline_width);
//     set_field(st, "s", aStyle.size);
//     set_field(st, "r", aStyle.rotation);
//     set_field(st, "a", aStyle.aspect);
//     set_field(st, "S", aStyle.shape);

//     rjson::object ls;
//     set_field(ls, "+", aStyle.label.shown);
//     set_field(ls, "t", aStyle.label_text);
//     set_field(ls, "f", aStyle.label.style.font_family);
//     set_field(ls, "S", aStyle.label.style.slant);
//     set_field(ls, "W", aStyle.label.style.weight);
//     set_field(ls, "s", aStyle.label.size);
//     set_field(ls, "c", aStyle.label.color);
//     set_field(ls, "r", aStyle.label.rotation);
//     set_field(ls, "i", aStyle.label.interline);
//     set_field(ls, "p", aStyle.label.offset);
//     if (!ls.empty())
//         st.set_field("l", std::move(ls));

// } // export_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
