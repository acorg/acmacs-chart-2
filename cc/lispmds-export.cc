#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/lispmds-export.hh"
#include "acmacs-chart-2/lispmds-encode.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static std::string antigen_names(std::shared_ptr<acmacs::chart::Antigens> aAntigens);
static std::string serum_names(std::shared_ptr<acmacs::chart::Sera> aSera);
static std::string titers(std::shared_ptr<acmacs::chart::Titers> aTiters);
static std::string starting_coordss(const acmacs::chart::Chart& aChart);
static std::string batch_runs(const acmacs::chart::Chart& aChart);
static std::string coordinates(std::shared_ptr<acmacs::chart::Layout> aLayout, size_t number_of_points, size_t number_of_dimensions, size_t aIndent);
static std::string col_and_row_adjusts(const acmacs::chart::Chart& aChart, std::shared_ptr<acmacs::chart::Projection> aProjection, size_t aIndent);
static std::string reference_antigens(const acmacs::chart::Chart& aChart);
static std::string plot_spec(const acmacs::chart::Chart& aChart);
static std::string point_style(const acmacs::PointStyle& aStyle);
static std::string point_shape(const acmacs::PointShape& aShape);
static std::string make_color(Color aColor);

// ----------------------------------------------------------------------

std::string acmacs::chart::lispmds_export(const acmacs::chart::Chart& aChart, std::string aProgramName)
{
    std::string result = ";; MDS configuration file (version 0.6).\n;; Created by AD " + aProgramName + " on " + acmacs::time_format() + "\n\n";
    result += R"((MAKE-MASTER-MDS-WINDOW
   (HI-IN '()" + antigen_names(aChart.antigens()) + R"()
          '()" + serum_names(aChart.sera()) + R"()
          '()" + titers(aChart.titers()) + R"()
          ')" + lispmds_encode(aChart.info()->name_non_empty()) + R"()
  )" + starting_coordss(aChart) + R"(
  )" + batch_runs(aChart) + R"(
)";
    if (auto projections = aChart.projections(); !projections->empty()) {
        auto projection = (*projections)[0];
        result.append("  :MDS-DIMENSIONS '" + acmacs::to_string(projection->layout()->number_of_dimensions()) + '\n');
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
  :CANVAS-COORD-TRANSFORMATIONS '(:CANVAS-WIDTH 800 :CANVAS-HEIGHT 800
                                  :CANVAS-X-COORD-TRANSLATION 0.0 :CANVAS-Y-COORD-TRANSLATION 0.0
                                  :CANVAS-X-COORD-SCALE 1 :CANVAS-Y-COORD-SCALE 1
                                  :FIRST-DIMENSION 0 :SECOND-DIMENSION 1
                                  :BASIS-VECTOR-POINT-INDICES NIL :BASIS-VECTOR-POINT-INDICES-BACKUP NIL
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

std::string reference_antigens(const acmacs::chart::Chart& aChart)
{
    auto antigens = aChart.antigens();
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

std::string starting_coordss(const acmacs::chart::Chart& aChart)
{
    auto projections = aChart.projections();
    if (projections->empty())
        return {};
    auto projection = (*projections)[0];
    auto layout = projection->layout();
    if (const auto number_of_points = layout->number_of_points(), number_of_dimensions = layout->number_of_dimensions(); number_of_points && number_of_dimensions)
        return "  :STARTING-COORDSS '(" + coordinates(layout, number_of_points, number_of_dimensions, 22) + col_and_row_adjusts(aChart, projection, 22) + ')';
    else
        return {};

} // starting_coordss

// ----------------------------------------------------------------------

std::string batch_runs(const acmacs::chart::Chart& aChart)
{
    auto projections = aChart.projections();
    if (projections->size() < 2)
        return {};
    std::string result = "  :BATCH-RUNS '(";
    for (size_t projection_no = 1; projection_no < projections->size(); ++projection_no) {
        auto projection = (*projections)[projection_no];
        auto layout = projection->layout();
        if (projection_no > 1)
            result.append("\n                ");
        result.append("((" + coordinates(layout, layout->number_of_points(), layout->number_of_dimensions(), 18) + col_and_row_adjusts(aChart, projection, 18) + ')');
        result.append("\n                 " + acmacs::to_string(projection->stress()) + " MULTIPLE-END-CONDITIONS NIL)");
    }
    result.append(1, ')');
    return result;

} // batch_runs

// ----------------------------------------------------------------------

std::string coordinates(std::shared_ptr<acmacs::chart::Layout> aLayout, size_t number_of_points, size_t number_of_dimensions, size_t aIndent)
{
    std::string result;
    for (size_t point_no = 0; point_no < number_of_points; ++point_no) {
        if (point_no)
            result.append(1, '\n').append(aIndent, ' ');
        result.append(1, '(');
        for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
            if (dim)
                result.append(1, ' ');
            const auto c = aLayout->coordinate(point_no, dim);
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

std::string col_and_row_adjusts(const acmacs::chart::Chart& aChart, std::shared_ptr<acmacs::chart::Projection> aProjection, size_t aIndent)
{
    std::string result{"\n"};
    result
            .append(aIndent, ' ')
            .append("((COL-AND-ROW-ADJUSTS\n")
            .append(aIndent + 2, ' ')
            .append(1, '(');
    const auto number_of_antigens = aChart.number_of_antigens();
    const auto number_of_sera = aChart.number_of_sera();
    for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
        if (ag_no)
            result.append(1, ' ');
        result.append("-1.0d+7");
    }
    result.append(1, '\n').append(aIndent + 3, ' ');
    auto cb = aProjection->forced_column_bases();
    if (!cb->exists())
        cb = aChart.computed_column_bases(aProjection->minimum_column_basis());
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

std::string plot_spec(const acmacs::chart::Chart& aChart)
{
    std::string result;
    if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty()) {
        result.append("  :PLOT-SPEC '(");
        auto antigens = aChart.antigens();
        const auto number_of_antigens = antigens->size();
        auto sera = aChart.sera();
        const auto number_of_sera = sera->size();
        for (size_t point_no = 0; point_no < (number_of_antigens + number_of_sera); ++point_no) {
            if (point_no)
                result.append("\n               ");
            std::string name, nm;
            if (point_no < number_of_antigens) {
                auto antigen = (*antigens)[point_no];
                name = lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations()) + "-AG";
                nm = string::join(" ", {antigen->name(), antigen->reassortant(), antigen->passage(), string::join(" ", antigen->annotations())});
            }
            else {
                auto serum = (*sera)[point_no - number_of_antigens];
                name = lispmds_serum_name_encode(serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id()) + "-SR";
                nm = string::join(" ", {serum->name(), serum->reassortant(), string::join(" ", serum->annotations()), serum->serum_id()});
            }
            result.append('(' + name + " :NM \"" + nm + '"' + point_style(plot_spec->style(point_no)) + ')');
        }
        result.append(")");
    }
    return result;

  // :RAISE-POINTS 'NIL
  // :LOWER-POINTS 'NIL

} // plot_spec

// ----------------------------------------------------------------------

std::string point_shape(const acmacs::PointShape& aShape)
{
    switch (static_cast<acmacs::PointShape::Shape>(aShape)) {
      case acmacs::PointShape::Circle:
          return "CIRCLE";
      case acmacs::PointShape::Box:
          return "RECTANGLE";
      case acmacs::PointShape::Triangle:
          return "TRIANGLE";
    }
    return "CIRCLE"; // gcc 7.2 wants this

} // point_shape

// ----------------------------------------------------------------------

std::string make_color(Color aColor)
{
    const std::string source = aColor.without_transparency();
    std::string result;
    std::copy_if(source.begin(), source.end(), std::back_inserter(result), [](auto c) -> bool { return c != ' '; });
    return result;

} // make_color

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
    result.append(" :SH \"" + point_shape(*aStyle.shape) + '"');
    result.append(" :NS " + acmacs::to_string(std::lround(aStyle.label.size * acmacs::lispmds::NS_SCALE))); // :NS must be integer (otherwise tk complains)
    result.append(" :NC \"" + make_color(*aStyle.label.color) + '"');
    if (*aStyle.fill == TRANSPARENT)
        result.append(" :CO \"{}\"");
    else
        result.append(" :CO \"" + make_color(*aStyle.fill) + '"');
    if (*aStyle.outline == TRANSPARENT)
        result.append(" :OC \"{}\"");
    else
        result.append(" :OC \"" + make_color(*aStyle.outline) + '"');
    if (const auto alpha = aStyle.fill->alpha(); alpha < 1.0)
        result.append(" :TR " + acmacs::to_string(1.0 - alpha));

    return result;

} // point_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
