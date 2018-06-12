#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/ace.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static void export_info(rjson::object& aTarget, acmacs::chart::InfoP aInfo);
static void export_antigens(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens);
static void export_sera(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera);
static void export_titers(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters);
static void export_projections(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Projections> aProjections);
static void export_plot_spec(rjson::object& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec);
static void export_style(rjson::array& target_styles, const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

std::string acmacs::chart::export_ace(const Chart& aChart, std::string aProgramName, size_t aIndent)
{
    rjson::value ace{rjson::object{{
                {"  version", rjson::string{"acmacs-ace-v1"}},
                {"?created", rjson::string{"AD " + aProgramName + " on " + acmacs::time_format()}},
                {"c", rjson::object{{
                            {"i", rjson::object{}},
                            {"a", rjson::array{}},
                            {"s", rjson::array{}},
                            {"t", rjson::object{}},
                        }}}
            }}};
    // Timeit ti_info("export info ");
    export_info(ace["c"]["i"], aChart.info());
    // ti_info.report();
    // Timeit ti_antigens("export antigens ");
    export_antigens(ace["c"]["a"], aChart.antigens());
    // ti_antigens.report();
    // Timeit ti_sera("export sera ");
    export_sera(ace["c"]["s"], aChart.sera());
    // ti_sera.report();
    // Timeit ti_titers("export titers ");
    export_titers(ace["c"]["t"], aChart.titers());
    // ti_titers.report();
    // Timeit ti_projections("export projections ");
    if (auto projections = aChart.projections(); !projections->empty())
        export_projections(ace["c"].set_field("P", rjson::array{}), projections);
    // ti_projections.report();
    // Timeit ti_plot_spec("export plot_spec ");
    if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty())
        export_plot_spec(ace["c"].set_field("p", rjson::object{}), plot_spec);
      // ti_plot_spec.report();
    if (aIndent)
        return ace.to_json_pp(aIndent);
    else
        return ace.to_json();

} // acmacs::chart::export_ace

// ----------------------------------------------------------------------

void export_info(rjson::object& aTarget, acmacs::chart::InfoP aInfo)
{
    auto do_export = [](rjson::object& target, acmacs::chart::InfoP info, bool /*for_source*/) {
        target.set_field_if_not_empty("v", info->virus());
        target.set_field_if_not_empty("V", info->virus_type());
        target.set_field_if_not_empty("A", info->assay());
        target.set_field_if_not_empty("D", info->date());
        target.set_field_if_not_empty("N", info->name());
        target.set_field_if_not_empty("l", info->lab());
        target.set_field_if_not_empty("r", info->rbc_species());
        target.set_field_if_not_empty("s", info->subset());
          //target.set_field_if_not_empty("T", info->table_type());
    };

    do_export(aTarget, aInfo, false);
    const auto number_of_sources = aInfo->number_of_sources();
    if (number_of_sources) {
        rjson::array& array = aTarget.set_field("S", rjson::array{});
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            do_export(array.insert(rjson::object{}), aInfo->source(source_no), true);
        }
    }

} // export_info

// ----------------------------------------------------------------------

static inline void export_lineage(rjson::object& object, acmacs::chart::BLineage lineage)
{
    switch (static_cast<acmacs::chart::BLineage::Lineage>(lineage)) {
      case acmacs::chart::BLineage::Victoria:
          object.set_field("L", rjson::string{"V"});
          break;
      case acmacs::chart::BLineage::Yamagata:
          object.set_field("L", rjson::string{"Y"});
          break;
      case acmacs::chart::BLineage::Unknown:
          break;
    }

} // export_lineage

// ----------------------------------------------------------------------

void export_antigens(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens)
{
    for (auto antigen: *aAntigens) {
        std::string semantic;
        if (antigen->reference())
            semantic += 'R';
        if (antigen->passage().is_egg())
            semantic += 'E';

        rjson::object& object = aTarget.insert(rjson::object{});

        object.set_field("N", rjson::string{antigen->name()});
        object.set_field_if_not_empty("D", static_cast<const std::string&>(antigen->date()));
        object.set_field_if_not_empty("P", static_cast<const std::string&>(antigen->passage()));
        object.set_field_if_not_empty("R", static_cast<const std::string&>(antigen->reassortant()));
        object.set_array_field_if_not_empty("l", antigen->lab_ids());
        object.set_field_if_not_empty("S", semantic);
        object.set_array_field_if_not_empty("a", antigen->annotations());
        object.set_array_field_if_not_empty("c", antigen->clades());
        export_lineage(object, antigen->lineage());
        object.set_field_if_not_empty("C", static_cast<const std::string&>(antigen->continent()));
    }

} // export_antigens

// ----------------------------------------------------------------------

void export_sera(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera)
{
    for (auto serum: *aSera) {
        std::string semantic;
        if (serum->passage().is_egg())
            semantic += 'E';

        rjson::object& object = aTarget.insert(rjson::object{});

        object.set_field("N", rjson::string{serum->name()});
        object.set_field_if_not_empty("P", static_cast<const std::string&>(serum->passage()));
        object.set_field_if_not_empty("R", static_cast<const std::string&>(serum->reassortant()));
        object.set_field_if_not_empty("I", static_cast<const std::string&>(serum->serum_id()));
        object.set_array_field_if_not_empty("a", serum->annotations());
        object.set_field_if_not_empty("s", static_cast<const std::string&>(serum->serum_species()));
        object.set_array_field_if_not_empty("h", serum->homologous_antigens());
        object.set_field_if_not_empty("S", semantic);
        export_lineage(object, serum->lineage());
    }

} // export_sera

// ----------------------------------------------------------------------

void export_titers(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters)
{
      // std::cerr << "number_of_non_dont_cares: " << aTiters->number_of_non_dont_cares() << '\n';
      // std::cerr << "percent_of_non_dont_cares: " << aTiters->percent_of_non_dont_cares() << '\n';
    const size_t number_of_antigens = aTiters->number_of_antigens();
    const size_t number_of_sera = aTiters->number_of_sera();

    auto fill_d = [number_of_antigens, number_of_sera](rjson::array& aLayer, std::function<acmacs::chart::Titer (size_t, size_t)> aGetTiter) {
        for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
            rjson::object& row = aLayer.insert(rjson::object{});
            for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
                const auto titer = aGetTiter(ag_no, sr_no);
                if (!titer.is_dont_care())
                    row.set_field(acmacs::to_string(sr_no), rjson::string{titer});
            }
        }
    };

      // --------------------------------------------------

      // Timeit ti_titers("export titers ");
    bool titers_exported = false;
    try {
        aTarget.set_field("d", aTiters->rjson_list_dict());
        titers_exported = true;
    }
    catch (acmacs::chart::data_not_available&) {
    }

    if (!titers_exported) {
        try {
            aTarget.set_field("l", aTiters->rjson_list_list());
            titers_exported = true;
        }
        catch (acmacs::chart::data_not_available&) {
        }
    }

    if (!titers_exported) {
          // slow method
        if ((number_of_antigens < 100 && number_of_sera < 100) || (static_cast<double>(aTiters->number_of_non_dont_cares()) / (number_of_antigens * number_of_sera)) > acmacs::chart::Titers::dense_sparse_boundary) {
            rjson::array& list = aTarget.set_field("l", rjson::array{});
            for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
                rjson::array& row = list.insert(rjson::array{});
                for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
                    row.insert(rjson::string{aTiters->titer(ag_no, sr_no)});
                }
            }
        }
        else {
            fill_d(aTarget.set_field("d", rjson::array{}), [aTiters](size_t ag_no, size_t sr_no) { return aTiters->titer(ag_no, sr_no); });
        }
    }
      // ti_titers.report();

      // --------------------------------------------------
      // layers

      // Timeit ti_layers("export layers ");
    bool layers_exported = false;
    try {
        aTarget.set_field("L", aTiters->rjson_layers());
        layers_exported = true;
    }
    catch (acmacs::chart::data_not_available&) {
    }

    if (!layers_exported) {
          // slow method
        if (const size_t number_of_layers = aTiters->number_of_layers(); number_of_layers) {
            rjson::array& layers = aTarget.set_field("L", rjson::array{});
            for (size_t layer_no = 0; layer_no < number_of_layers; ++layer_no) {
                fill_d(layers.insert(rjson::array{}), [aTiters, layer_no](size_t ag_no, size_t sr_no) { return aTiters->titer_of_layer(layer_no, ag_no, sr_no); });
            }
        }
    }
      // ti_layers.report();

} // export_titers

// ----------------------------------------------------------------------

void export_projections(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Projections> aProjections)
{
    for (const auto projection: *aProjections) {
        rjson::object& target = aTarget.insert(rjson::object{});

        auto layout = projection->layout();
        if (const auto number_of_points = layout->number_of_points(), number_of_dimensions = layout->number_of_dimensions(); number_of_points && number_of_dimensions) {
            rjson::array& ar = target.set_field("l", rjson::array{});
            for (size_t p_no = 0; p_no < number_of_points; ++p_no) {
                rjson::array& p = ar.insert(rjson::array{});
                for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
                    const auto c = layout->coordinate(p_no, dim);
                    if (std::isnan(c))
                        break;
                    p.insert(rjson::to_value(c));
                }
            }
        }

        target.set_field_if_not_empty("c", static_cast<const std::string&>(projection->comment()));
        target.set_field_if_not_default("s", projection->stress(), acmacs::chart::InvalidStress, 8);
        if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
            target.set_field("m", rjson::string{minimum_column_basis});
        if (const auto forced_column_bases = projection->forced_column_bases(); forced_column_bases) {
            rjson::array& ar = target.set_field("C", rjson::array{});
            for (size_t sr_no = 0; sr_no < forced_column_bases->size(); ++sr_no)
                ar.insert(rjson::to_value(forced_column_bases->column_basis(sr_no)));
        }
        if (const auto transformation = projection->transformation(); transformation != acmacs::Transformation{})
            target.set_field("t", rjson::array{transformation.a, transformation.b, transformation.c, transformation.d});
        target.set_field_if_not_default("d", projection->dodgy_titer_is_regular(), false);
        target.set_field_if_not_default("e", projection->stress_diff_to_stop(), 0.0);
        if (const auto unmovable = projection->unmovable(); ! unmovable.empty())
            target.set_field("U", rjson::array(rjson::array::use_iterator, unmovable.begin(), unmovable.end()));
        if (const auto disconnected = projection->disconnected(); ! disconnected.empty())
            target.set_field("D", rjson::array(rjson::array::use_iterator, disconnected.begin(), disconnected.end()));
        if (const auto unmovable_in_the_last_dimension = projection->unmovable_in_the_last_dimension(); ! unmovable_in_the_last_dimension.empty())
            target.set_field("u", rjson::array(rjson::array::use_iterator, unmovable_in_the_last_dimension.begin(), unmovable_in_the_last_dimension.end()));
        if (const auto avidity_adjusts = projection->avidity_adjusts(); ! avidity_adjusts.empty())
            target.set_field("f", rjson::array(rjson::array::use_iterator, avidity_adjusts.begin(), avidity_adjusts.end()));

        // "i": 600,               // number of iterations?
        // "g": [],            // antigens_sera_gradient_multipliers, double for each point
    }

} // export_projections

// ----------------------------------------------------------------------

void export_plot_spec(rjson::object& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec)
{
    if (const auto drawing_order = aPlotSpec->drawing_order(); ! drawing_order.empty())
        aTarget.set_field("d", rjson::array(rjson::array::use_iterator, drawing_order.begin(), drawing_order.end()));
    if (const auto color = aPlotSpec->error_line_positive_color(); color != RED)
        aTarget.set_field("E", rjson::object{{{"c", rjson::string{color.to_string()}}}});
    if (const auto color = aPlotSpec->error_line_negative_color(); color != BLUE)
        aTarget.set_field("e", rjson::object{{{"c", rjson::string{color.to_string()}}}});

    const auto compacted = aPlotSpec->compacted();
    aTarget.set_field("p", rjson::array(rjson::array::use_iterator, compacted.index.begin(), compacted.index.end()));
    rjson::array& target_styles = aTarget.set_field("P", rjson::array{});
    for (const auto& style: compacted.styles)
        export_style(target_styles, style);

      // "g": {},                  // ? grid data
      // "l": [],                  // ? for each procrustes line, index in the "L" list
      // "L": []                    // ? list of procrustes lines styles
      // "s": [],                  // list of point indices for point shown on all maps in the time series
      // "t": {}                    // title style?

} // export_plot_spec

// ----------------------------------------------------------------------

namespace rjson
{
    template <> struct content_type<Color> { using type = rjson::string; };
    template <> struct content_type<acmacs::PointShape> { using type = rjson::string; };
    template <> struct content_type<acmacs::FontSlant> { using type = rjson::string; };
    template <> struct content_type<acmacs::FontWeight> { using type = rjson::string; };

    template <char Tag> inline value to_value(acmacs::internal::SizeScale<Tag> aValue) { return to_value(aValue.value()); }

    inline value to_value(const acmacs::Offset aValue) { return array{aValue.x(), aValue.y()}; }

} // namespace rjson

template <typename T> inline void set_field(rjson::object& target, const char* name, const acmacs::internal::field_optional_with_default<T>& field)
{
    if (field.not_default()) {
        if constexpr (std::is_same_v<T, Color>)
            target.set_field(name, rjson::to_value(field->to_hex_string()));
        else
            target.set_field(name, rjson::to_value(*field));
    }
}

void export_style(rjson::array& target_styles, const acmacs::PointStyle& aStyle)
{
    rjson::object& st = target_styles.insert(rjson::object{});
    set_field(st, "+", aStyle.shown);
    set_field(st, "F", aStyle.fill);
    set_field(st, "O", aStyle.outline);
    set_field(st, "o", aStyle.outline_width);
    if (aStyle.size.not_default())
        st.set_field("s", rjson::to_value(aStyle.size->value() / acmacs::chart::ace::PointScale));
    set_field(st, "r", aStyle.rotation);
    set_field(st, "a", aStyle.aspect);
    set_field(st, "S", aStyle.shape);

    rjson::object ls;
    set_field(ls, "+", aStyle.label.shown);
    set_field(ls, "t", aStyle.label_text);
    set_field(ls, "f", aStyle.label.style.font_family);
    set_field(ls, "S", aStyle.label.style.slant);
    set_field(ls, "W", aStyle.label.style.weight);
    if (aStyle.label.size.not_default())
        ls.set_field("s", rjson::to_value(aStyle.label.size->value() / acmacs::chart::ace::LabelScale));
    set_field(ls, "c", aStyle.label.color);
    set_field(ls, "r", aStyle.label.rotation);
    set_field(ls, "i", aStyle.label.interline);
    set_field(ls, "p", aStyle.label.offset);
    if (!ls.empty())
        st.set_field("l", std::move(ls));

} // export_style

// ----------------------------------------------------------------------

template <typename DF> std::string acmacs::chart::export_layout(const Chart& aChart, size_t aProjectionNo)
{
    auto antigens = aChart.antigens();
    const auto number_of_antigens = antigens->size();
    auto sera = aChart.sera();
    auto layout = aChart.projection(aProjectionNo)->layout();
    const auto number_of_dimensions = layout->number_of_dimensions();

    std::string result;

    for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
        DF::first_field(result, "AG");
        DF::second_field(result, antigen->full_name());
        for (auto dim : acmacs::range(number_of_dimensions))
            DF::second_field(result, (*layout)(ag_no, dim));
        DF::end_of_record(result);
    }

    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        DF::first_field(result, "SR");
        DF::second_field(result, serum->full_name());
        for (auto dim : acmacs::range(number_of_dimensions))
            DF::second_field(result, (*layout)(sr_no + number_of_antigens, dim));
        DF::end_of_record(result);
    }

    return result;

} // acmacs::chart::export_layout

template std::string acmacs::chart::export_layout<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
template std::string acmacs::chart::export_layout<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

// ----------------------------------------------------------------------

template <typename DF> std::string acmacs::chart::export_table_map_distances(const Chart& aChart, size_t aProjectionNo)
{
    auto projection = aChart.projection(aProjectionNo);
    auto layout = projection->layout();

    const auto table_distances = acmacs::chart::table_distances(aChart, projection->minimum_column_basis(), projection->dodgy_titer_is_regular());
    const MapDistances map_distances(*layout, table_distances);

    auto antigens = aChart.antigens();
    auto sera = aChart.sera();
    auto point_name = [&antigens, &sera](size_t point_no) -> std::string {
        return point_no < antigens->size() ? antigens->at(point_no)->full_name() : sera->at(point_no - antigens->size())->full_name();
    };

    std::string result;
    for (auto td = table_distances.regular().begin(), md = map_distances.regular().begin(); td != table_distances.regular().end(); ++td, ++md) {
        DF::first_field(result, point_name(td->point_1));
        DF::second_field(result, point_name(td->point_2));
        DF::second_field(result, td->distance);
        DF::second_field(result, md->distance);
        DF::end_of_record(result);
    }
    return result;

} // acmacs::chart::export_table_map_distances

template std::string acmacs::chart::export_table_map_distances<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
template std::string acmacs::chart::export_table_map_distances<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

// ----------------------------------------------------------------------

template <typename DF> std::string acmacs::chart::export_distances_between_all_points(const Chart& aChart, size_t aProjectionNo)
{
    auto projection = aChart.projection(aProjectionNo);
    auto layout = projection->layout();
    auto antigens = aChart.antigens();
    auto sera = aChart.sera();
    const auto number_of_antigens = antigens->size();
    const auto number_of_points = number_of_antigens + sera->size();

    std::string result;
    DF::first_field(result, "AG1");
    DF::second_field(result, "No1");
    DF::second_field(result, "Name1");
    DF::second_field(result, "AG2");
    DF::second_field(result, "No2");
    DF::second_field(result, "Name2");
    DF::second_field(result, "Distance");
    DF::end_of_record(result);

    for (auto point_1 : acmacs::range(number_of_points)) {
        const auto ag_1 = point_1 < number_of_antigens;
        const auto no_1 = ag_1 ? point_1 : (point_1 - number_of_antigens);
        const auto name_1 = ag_1 ? antigens->at(no_1)->full_name() : sera->at(no_1)->full_name();
        for (auto point_2 : acmacs::range(point_1 + 1, number_of_points)) {
            const auto ag_2 = point_2 < number_of_antigens;
            const auto no_2 = ag_2 ? point_2 : (point_2 - number_of_antigens);
            const auto name_2 = ag_2 ? antigens->at(no_2)->full_name() : sera->at(no_2)->full_name();
            const auto distance = layout->distance(point_1, point_2);

            DF::first_field(result, ag_1 ? "AG" : "SR");
            DF::second_field(result, no_1);
            DF::second_field(result, name_1);
            DF::second_field(result, ag_2 ? "AG" : "SR");
            DF::second_field(result, no_2);
            DF::second_field(result, name_2);
            DF::second_field(result, distance);
            DF::end_of_record(result);
        }
    }

    return result;

} // acmacs::chart::export_distances_between_all_points

template std::string acmacs::chart::export_distances_between_all_points<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
template std::string acmacs::chart::export_distances_between_all_points<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

// ----------------------------------------------------------------------

template <typename DF> std::string acmacs::chart::export_error_lines(const Chart& aChart, size_t aProjectionNo)
{
    auto antigens = aChart.antigens();
    auto sera = aChart.sera();
    auto point_name = [&antigens, &sera](size_t point_no) -> std::string {
        return point_no < antigens->size() ? antigens->at(point_no)->full_name() : sera->at(point_no - antigens->size())->full_name();
    };

    const auto error_lines = aChart.projection(aProjectionNo)->error_lines();

    std::string result;
    for (const auto& el : error_lines) {
        DF::first_field(result, point_name(el.point_1));
        DF::second_field(result, point_name(el.point_2));
        DF::second_field(result, el.error_line);
        DF::end_of_record(result);
    }
    return result;

} // acmacs::chart::export_error_lines

template std::string acmacs::chart::export_error_lines<acmacs::DataFormatterSpaceSeparated>(const Chart& aChart, size_t aProjectionNo);
template std::string acmacs::chart::export_error_lines<acmacs::DataFormatterCSV>(const Chart& aChart, size_t aProjectionNo);

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
