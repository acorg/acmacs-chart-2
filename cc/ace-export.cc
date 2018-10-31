#include "acmacs-base/time.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/ace.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static void export_info(rjson::value& aTarget, acmacs::chart::InfoP aInfo);
static void export_antigens(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens);
static void export_sera(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera);
static void export_titers(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters);
static void export_forced_column_bases(rjson::value& aTarget, std::shared_ptr<acmacs::chart::ColumnBases> column_bases);
static void export_projections(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Projections> aProjections);
static void export_plot_spec(rjson::value& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec);
static void export_style(rjson::value& target_styles, const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

rjson::value acmacs::chart::export_ace_to_rjson(const Chart& aChart, std::string aProgramName)
{
    rjson::value ace{rjson::object{{
                {"  version", "acmacs-ace-v1"},
                {"?created", "AD " + aProgramName + " on " + acmacs::time_format()},
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
    export_forced_column_bases(ace["c"], aChart.forced_column_bases(MinimumColumnBasis{}));
    if (auto projections = aChart.projections(); !projections->empty())
        export_projections(ace["c"]["P"], projections);
    // ti_projections.report();
    // Timeit ti_plot_spec("export plot_spec ");
    if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty())
        export_plot_spec(ace["c"]["p"], plot_spec);
      // ti_plot_spec.report();
    return ace;

} // acmacs::chart::export_ace_to_rjson

// ----------------------------------------------------------------------

std::string acmacs::chart::export_ace(const Chart& aChart, std::string aProgramName, size_t aIndent)
{
    const auto ace = export_ace_to_rjson(aChart, aProgramName);
    if (aIndent)
        return rjson::pretty(ace, aIndent);
    else
        return rjson::to_string(ace);

} // acmacs::chart::export_ace

// ----------------------------------------------------------------------

void export_info(rjson::value& aTarget, acmacs::chart::InfoP aInfo)
{
    auto do_export = [](rjson::value& target, acmacs::chart::InfoP info, bool /*for_source*/) {
        rjson::set_field_if_not_empty(target, "v", info->virus());
        rjson::set_field_if_not_empty(target, "V", info->virus_type());
        rjson::set_field_if_not_empty(target, "A", info->assay());
        rjson::set_field_if_not_empty(target, "D", info->date());
        rjson::set_field_if_not_empty(target, "N", info->name());
        rjson::set_field_if_not_empty(target, "l", info->lab());
        rjson::set_field_if_not_empty(target, "r", info->rbc_species());
        rjson::set_field_if_not_empty(target, "s", info->subset());
          //rjson::set_field_if_not_empty(target, "T", info->table_type());
    };

    do_export(aTarget, aInfo, false);
    const auto number_of_sources = aInfo->number_of_sources();
    if (number_of_sources) {
        rjson::value& array = aTarget["S"] = rjson::array{};
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            do_export(array.append(rjson::object{}), aInfo->source(source_no), true);
        }
    }

} // export_info

// ----------------------------------------------------------------------

static inline void export_lineage(rjson::value& object, acmacs::chart::BLineage lineage)
{
    switch (static_cast<acmacs::chart::BLineage::Lineage>(lineage)) {
      case acmacs::chart::BLineage::Victoria:
          object["L"] = "V";
          break;
      case acmacs::chart::BLineage::Yamagata:
          object["L"] = "Y";
          break;
      case acmacs::chart::BLineage::Unknown:
          break;
    }

} // export_lineage

// ----------------------------------------------------------------------

void export_antigens(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens)
{
    for (auto antigen: *aAntigens) {
        std::string semantic;
        if (antigen->reference())
            semantic += 'R';
        if (antigen->passage().is_egg())
            semantic += 'E';

        auto& object = aTarget.append(rjson::object{});
        object["N"] = antigen->name();
        rjson::set_field_if_not_empty(object, "D", antigen->date());
        rjson::set_field_if_not_empty(object, "P", antigen->passage());
        rjson::set_field_if_not_empty(object, "R", antigen->reassortant());
        rjson::set_array_field_if_not_empty(object, "l", antigen->lab_ids());
        rjson::set_field_if_not_empty(object, "S", semantic);
        rjson::set_array_field_if_not_empty(object, "a", antigen->annotations());
        rjson::set_array_field_if_not_empty(object, "c", antigen->clades());
        export_lineage(object, antigen->lineage());
        rjson::set_field_if_not_empty(object, "C", antigen->continent());
    }

} // export_antigens

// ----------------------------------------------------------------------

void export_sera(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera)
{
    for (auto serum: *aSera) {
        std::string semantic;
        if (serum->passage().is_egg())
            semantic += 'E';

        auto& object = aTarget.append(rjson::object{});
        object["N"] = serum->name();
        rjson::set_field_if_not_empty(object, "P", serum->passage());
        rjson::set_field_if_not_empty(object, "R", serum->reassortant());
        rjson::set_field_if_not_empty(object, "I", serum->serum_id());
        rjson::set_array_field_if_not_empty(object, "a", serum->annotations());
        rjson::set_field_if_not_empty(object, "s", serum->serum_species());
        rjson::set_array_field_if_not_empty(object, "h", serum->homologous_antigens());
        rjson::set_field_if_not_empty(object, "S", semantic);
        export_lineage(object, serum->lineage());
    }

} // export_sera

// ----------------------------------------------------------------------

void export_titers(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters)
{
      // std::cerr << "number_of_non_dont_cares: " << aTiters->number_of_non_dont_cares() << '\n';
      // std::cerr << "percent_of_non_dont_cares: " << aTiters->percent_of_non_dont_cares() << '\n';
    const size_t number_of_antigens = aTiters->number_of_antigens();
    const size_t number_of_sera = aTiters->number_of_sera();

    auto fill_d = [number_of_antigens](rjson::value& aLayer, auto first, auto last) {
        for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no)
            aLayer.append(rjson::object{});
        for (; first != last; ++first)
            aLayer[first->antigen][first->serum] = first->titer;
    };

      // --------------------------------------------------

      // Timeit ti_titers("export titers ");
    bool titers_exported = false;
    try {
        aTarget["d"] = aTiters->rjson_list_dict();
        titers_exported = true;
    }
    catch (acmacs::chart::data_not_available&) {
    }

    if (!titers_exported) {
        try {
            aTarget["l"] = aTiters->rjson_list_list();
            titers_exported = true;
        }
        catch (acmacs::chart::data_not_available&) {
        }
    }

    if (!titers_exported) {
          // slow method
        if ((number_of_antigens < 100 && number_of_sera < 100) || (static_cast<double>(aTiters->number_of_non_dont_cares()) / (number_of_antigens * number_of_sera)) > acmacs::chart::Titers::dense_sparse_boundary) {
            auto& list = aTarget["l"] = rjson::array{};
            for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
                auto& row = list.append(rjson::array{});
                for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
                    row.append(aTiters->titer(ag_no, sr_no));
                }
            }
        }
        else {
            fill_d(aTarget["d"] = rjson::array{}, aTiters->begin(), aTiters->end());
        }
    }
      // ti_titers.report();

      // --------------------------------------------------
      // layers

      // Timeit ti_layers("export layers ");
    bool layers_exported = false;
    try {
        aTarget["L"] = aTiters->rjson_layers();
        layers_exported = true;
    }
    catch (acmacs::chart::data_not_available&) {
    }

    if (!layers_exported) {
          // slow method
        if (const size_t number_of_layers = aTiters->number_of_layers(); number_of_layers) {
            auto& layers = aTarget["L"] = rjson::array{};
            for (size_t layer_no = 0; layer_no < number_of_layers; ++layer_no) {
                fill_d(layers.append(rjson::array{}), aTiters->begin(layer_no), aTiters->end(layer_no));
            }
        }
    }
      // ti_layers.report();

} // export_titers

// ----------------------------------------------------------------------

void export_forced_column_bases(rjson::value& aTarget, std::shared_ptr<acmacs::chart::ColumnBases> column_bases)
{
    if (column_bases) {
        auto& ar = aTarget["C"] = rjson::array{};
        for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
            ar.append(column_bases->column_basis(sr_no));
    }
}

// ----------------------------------------------------------------------

void export_projections(rjson::value& aTarget, std::shared_ptr<acmacs::chart::Projections> aProjections)
{
    if (aTarget.is_null())
        aTarget = rjson::array{};
    for (const auto projection: *aProjections) {
        auto& target = aTarget.append(rjson::object{});

        auto layout = projection->layout();
        if (const auto number_of_points = layout->number_of_points(), number_of_dimensions = layout->number_of_dimensions(); number_of_points && number_of_dimensions) {
            rjson::value& ar = target["l"] = rjson::array{};
            for (size_t p_no = 0; p_no < number_of_points; ++p_no) {
                auto& p = ar.append(rjson::array{});
                for (size_t dim = 0; dim < number_of_dimensions; ++dim) {
                    const auto c = layout->coordinate(p_no, dim);
                    if (std::isnan(c))
                        break;
                    p.append(c);
                }
            }
        }

        rjson::set_field_if_not_empty(target, "c", projection->comment());
        rjson::set_field_if_not_default(target, "s", projection->stress(), acmacs::chart::InvalidStress, 8);
        if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
            target["m"] = static_cast<std::string>(minimum_column_basis);
        export_forced_column_bases(target, projection->forced_column_bases());
        if (const auto transformation = projection->transformation(); transformation != acmacs::Transformation{})
            target["t"] = rjson::array{transformation.a, transformation.b, transformation.c, transformation.d};
        rjson::set_field_if_not_default(target, "d", projection->dodgy_titer_is_regular(), false);
        rjson::set_field_if_not_default(target, "e", projection->stress_diff_to_stop(), 0.0);
        if (const auto unmovable = projection->unmovable(); ! unmovable.empty())
            target["U"] = rjson::array(unmovable.begin(), unmovable.end());
        if (const auto disconnected = projection->disconnected(); ! disconnected.empty())
            target["D"] = rjson::array(disconnected.begin(), disconnected.end());
        if (const auto unmovable_in_the_last_dimension = projection->unmovable_in_the_last_dimension(); ! unmovable_in_the_last_dimension.empty())
            target["u"] = rjson::array(unmovable_in_the_last_dimension.begin(), unmovable_in_the_last_dimension.end());
        if (const auto avidity_adjusts = projection->avidity_adjusts(); ! avidity_adjusts.empty())
            target["f"] = rjson::array(avidity_adjusts.begin(), avidity_adjusts.end());

        // "i": 600,               // number of iterations?
        // "g": [],            // antigens_sera_gradient_multipliers, double for each point
    }

} // export_projections

// ----------------------------------------------------------------------

void export_plot_spec(rjson::value& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec)
{
    if (aTarget.is_null())
        aTarget = rjson::object{};
    if (const auto drawing_order = aPlotSpec->drawing_order(); ! drawing_order.empty())
        aTarget["d"] = rjson::array(drawing_order.begin(), drawing_order.end());
    if (const auto color = aPlotSpec->error_line_positive_color(); color != RED)
        aTarget["E"] = rjson::object{{"c", color.to_string()}};
    if (const auto color = aPlotSpec->error_line_negative_color(); color != BLUE)
        aTarget["e"] = rjson::object{{"c", color.to_string()}};

    const auto compacted = aPlotSpec->compacted();
    aTarget["p"] = rjson::array(compacted.index.begin(), compacted.index.end());
    auto& target_styles = aTarget["P"] = rjson::array{};
    for (const auto& style: compacted.styles)
        export_style(target_styles, style);

      // "g": {},                  // ? grid data
      // "l": [],                  // ? for each procrustes line, index in the "L" list
      // "L": []                    // ? list of procrustes lines styles
      // "s": [],                  // list of point indices for point shown on all maps in the time series
      // "t": {}                    // title style?

} // export_plot_spec

// ----------------------------------------------------------------------

// namespace rjson
// {
//     template <> struct content_type<Color> { using type = rjson::string; };
//     template <> struct content_type<acmacs::PointShape> { using type = rjson::string; };
//     template <> struct content_type<acmacs::FontSlant> { using type = rjson::string; };
//     template <> struct content_type<acmacs::FontWeight> { using type = rjson::string; };

//     template <char Tag> inline value to_value(acmacs::detail::SizeScale<Tag> aValue) { return to_value(aValue.value()); }

//     inline value to_value(const acmacs::Offset aValue) { return array{aValue.x(), aValue.y()}; }

// } // namespace rjson

template <typename T> inline void set_field(rjson::value& target, const char* name, const acmacs::detail::field_optional_with_default<T>& field)
{
    if (field.not_default()) {
        if constexpr (std::is_same_v<T, Color>)
            target[name] = field->to_hex_string();
        else if constexpr (std::is_same_v<T, Pixels> || std::is_same_v<T, Scaled> || std::is_same_v<T, Rotation> || std::is_same_v<T, Aspect>)
            target[name] = field->value();
        else if constexpr (std::is_same_v<T, acmacs::PointShape> || std::is_same_v<T, acmacs::FontSlant> || std::is_same_v<T, acmacs::FontWeight>)
            target[name] = static_cast<std::string>(*field);
        else if constexpr (std::is_same_v<T, acmacs::Offset>)
            target[name] = rjson::array{field->x(), field->y()};
        else
            target[name] = *field;
    }
}

void export_style(rjson::value& target_styles, const acmacs::PointStyle& aStyle)
{
    auto& st = target_styles.append(rjson::object{});
    set_field(st, "+", aStyle.shown);
    set_field(st, "F", aStyle.fill);
    set_field(st, "O", aStyle.outline);
    set_field(st, "o", aStyle.outline_width);
    if (aStyle.size.not_default())
        st["s"] = aStyle.size->value() / acmacs::chart::ace::PointScale;
    set_field(st, "r", aStyle.rotation);
    set_field(st, "a", aStyle.aspect);
    set_field(st, "S", aStyle.shape);

    rjson::value ls{rjson::object{}};
    set_field(ls, "+", aStyle.label.shown);
    set_field(ls, "t", aStyle.label_text);
    set_field(ls, "f", aStyle.label.style.font_family);
    set_field(ls, "S", aStyle.label.style.slant);
    set_field(ls, "W", aStyle.label.style.weight);
    if (aStyle.label.size.not_default())
        ls["s"] = aStyle.label.size->value() / acmacs::chart::ace::LabelScale;
    set_field(ls, "c", aStyle.label.color);
    set_field(ls, "r", aStyle.label.rotation);
    set_field(ls, "i", aStyle.label.interline);
    set_field(ls, "p", aStyle.label.offset);
    if (!ls.empty())
        st["l"] = std::move(ls);

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
