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
static std::string point_style(const acmacs::PointStyle& aStyle);

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
    if (auto plot_spec = aChart->plot_spec(); !plot_spec->empty()) {
        result.append("  :PLOT-SPEC '(");
        auto antigens = aChart->antigens();
        const auto number_of_antigens = antigens->size();
        auto sera = aChart->sera();
        const auto number_of_sera = sera->size();
        for (size_t point_no = 0; point_no < (number_of_antigens + number_of_sera); ++point_no) {
            if (point_no)
                result.append("\n               ");
            std::string name;
            if (point_no < number_of_antigens) {
                auto antigen = (*antigens)[point_no];
                name = lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations()) + "-AG";
            }
            else {
                auto serum = (*sera)[point_no - number_of_antigens];
                name = lispmds_serum_name_encode(serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id()) + "-SR";
            }
            result.append('(' + name + point_style(plot_spec->style(point_no)) + ')');
        }
        result.append(")");
    }
    return result;

  // :RAISE-POINTS 'NIL
  // :LOWER-POINTS 'NIL
              // :PLOT-SPEC '((BI/16190/68-AG :CO "#ffdba5" :DS 4 :TR 0.0 :NM "BI/16190/68" :WN "BI/16190/68" :NS 10 :NC "#ffdba5" :SH "CIRCLE")

} // plot_spec

// ----------------------------------------------------------------------

std::string point_style(const acmacs::PointStyle& aStyle)
{
    std::string result;
    result.append(" :DS " + acmacs::to_string(aStyle.size * acmacs::lispmds::DS_SCALE));
    if (aStyle.label.shown) {
        if (aStyle.label_text.not_default())
            result.append(" :WN \"" + static_cast<std::string>(*aStyle.label_text) + "\"");
    }
    else
        result.append(" :WN \"\"");
    result.append(" :SH " + static_cast<std::string>(*aStyle.shape));
    result.append(" :NS " + acmacs::to_string(aStyle.label.size * acmacs::lispmds::NS_SCALE));
    result.append(" :NC \"" + *aStyle.label.color + '"');
    if (*aStyle.fill == TRANSPARENT)
        result.append(" :CO \"{}\"");
    else
        result.append(" :CO \"" + aStyle.fill->without_transparency() + '"');
    if (*aStyle.outline == TRANSPARENT)
        result.append(" :OC \"{}\"");
    else
        result.append(" :OC \"" + aStyle.outline->without_transparency() + '"');
    if (const auto alpha = aStyle.fill->alpha(); alpha < 1.0)
        result.append(" :TR " + acmacs::to_string(1.0 - alpha));

    return result;

} // point_style

// ----------------------------------------------------------------------


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
