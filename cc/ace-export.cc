#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/ace-export.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo);
static void export_antigens(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens);
static void export_sera(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera);
static void export_titers(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters);
static void export_projections(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Projections> aProjections);
static void export_plot_spec(rjson::object& aTarget, std::shared_ptr<acmacs::chart::PlotSpec> aPlotSpec);
static void compact_styles(const std::vector<acmacs::PointStyle>& aAllStyles, std::vector<acmacs::PointStyle>& aCompacted, std::vector<size_t>& aIndex);
static void export_style(rjson::array& target_styles, const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

std::string acmacs::chart::ace_export(const Chart& aChart, std::string aProgramName)
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
    return ace.to_json_pp(1);

} // acmacs::chart::ace_export

// ----------------------------------------------------------------------

void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo)
{
    aTarget.set_field_if_not_empty("v", aInfo->virus());
    aTarget.set_field_if_not_empty("V", aInfo->virus_type());
    aTarget.set_field_if_not_empty("A", aInfo->assay());
    aTarget.set_field_if_not_empty("D", aInfo->date());
    aTarget.set_field_if_not_empty("N", aInfo->name());
    aTarget.set_field_if_not_empty("l", aInfo->lab());
    aTarget.set_field_if_not_empty("r", aInfo->rbc_species());
    aTarget.set_field_if_not_empty("s", aInfo->subset());
      //aTarget.set_field_if_not_empty("T", aInfo->table_type());

    const auto number_of_sources = aInfo->number_of_sources();
    if (number_of_sources) {
        rjson::array& array = aTarget.set_field("S", rjson::array{});
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            export_info(array.insert(rjson::object{}), aInfo->source(source_no));
        }
    }

} // export_info

// ----------------------------------------------------------------------

void export_antigens(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens)
{
    for (auto antigen: *aAntigens) {
        rjson::object& object = aTarget.insert(rjson::object{});

        object.set_field("N", rjson::string{antigen->name()});
        object.set_field_if_not_empty("D", static_cast<const std::string&>(antigen->date()));
        object.set_field_if_not_empty("P", static_cast<const std::string&>(antigen->passage()));
        object.set_field_if_not_empty("R", static_cast<const std::string&>(antigen->reassortant()));
        object.set_array_field_if_not_empty("l", antigen->lab_ids());
        if (antigen->reference())
            object.set_field("S", rjson::string{"R"});
        object.set_array_field_if_not_empty("a", antigen->annotations());
        object.set_array_field_if_not_empty("c", antigen->clades());

        switch (antigen->lineage()) {
          case acmacs::chart::BLineage::Victoria:
              object.set_field("L", rjson::string{"V"});
              break;
          case acmacs::chart::BLineage::Yamagata:
              object.set_field("L", rjson::string{"Y"});
              break;
          case acmacs::chart::BLineage::Unknown:
              break;
        }
    }

} // export_antigens

// ----------------------------------------------------------------------

void export_sera(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera)
{
    for (auto serum: *aSera) {
        rjson::object& object = aTarget.insert(rjson::object{});

        object.set_field("N", rjson::string{serum->name()});
        object.set_field_if_not_empty("P", static_cast<const std::string&>(serum->passage()));
        object.set_field_if_not_empty("R", static_cast<const std::string&>(serum->reassortant()));
        object.set_field_if_not_empty("I", static_cast<const std::string&>(serum->serum_id()));
        object.set_array_field_if_not_empty("a", serum->annotations());
        object.set_field_if_not_empty("s", static_cast<const std::string&>(serum->serum_species()));
        object.set_array_field_if_not_empty("h", serum->homologous_antigens());

        switch (serum->lineage()) {
          case acmacs::chart::BLineage::Victoria:
              object.set_field("L", rjson::string{"V"});
              break;
          case acmacs::chart::BLineage::Yamagata:
              object.set_field("L", rjson::string{"Y"});
              break;
          case acmacs::chart::BLineage::Unknown:
              break;
        }
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
        if (const double percent_of_non_dont_cares = static_cast<double>(aTiters->number_of_non_dont_cares()) / (number_of_antigens * number_of_sera); percent_of_non_dont_cares > 0.7) {
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
        target.set_field_if_not_default("s", projection->stress(), 0.0);
        if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
            target.set_field("m", rjson::string{minimum_column_basis});
        if (const auto forced_column_bases = projection->forced_column_bases(); forced_column_bases->exists()) {
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
        aTarget.set_field("E", rjson::object{{{"c", rjson::string{color}}}});
    if (const auto color = aPlotSpec->error_line_negative_color(); color != BLUE)
        aTarget.set_field("e", rjson::object{{{"c", rjson::string{color}}}});

    std::vector<acmacs::PointStyle> compacted;
    std::vector<size_t> p_index;
    compact_styles(aPlotSpec->all_styles(), compacted, p_index);
    aTarget.set_field("p", rjson::array(rjson::array::use_iterator, p_index.begin(), p_index.end()));
    rjson::array& target_styles = aTarget.set_field("P", rjson::array{});
    for (const auto& style: compacted)
        export_style(target_styles, style);

      // "g": {},                  // ? grid data
      // "l": [],                  // ? for each procrustes line, index in the "L" list
      // "L": []                    // ? list of procrustes lines styles
      // "s": [],                  // list of point indices for point shown on all maps in the time series
      // "t": {}                    // title style?

} // export_plot_spec

// ----------------------------------------------------------------------

void compact_styles(const std::vector<acmacs::PointStyle>& aAllStyles, std::vector<acmacs::PointStyle>& aCompacted, std::vector<size_t>& aIndex)
{
    for (const auto& style: aAllStyles) {
        if (auto found = std::find(aCompacted.begin(), aCompacted.end(), style); found == aCompacted.end()) {
            aCompacted.push_back(style);
            aIndex.push_back(aCompacted.size() - 1);
        }
        else {
            aIndex.push_back(static_cast<size_t>(found - aCompacted.begin()));
        }
    }

} // compact_styles

// ----------------------------------------------------------------------

namespace rjson
{
    template <> struct content_type<Color> { using type = rjson::string; };
    template <> struct content_type<acmacs::PointShape> { using type = rjson::string; };
    template <> struct content_type<acmacs::FontSlant> { using type = rjson::string; };
    template <> struct content_type<acmacs::FontWeight> { using type = rjson::string; };

    template <char Tag> inline value to_value(_acmacs_base_internal::SizeScale<Tag> aValue) { return to_value(aValue.value()); }

    inline value to_value(const acmacs::Offset aValue) { return array{aValue.x, aValue.y}; }

} // namespace rjson

template <typename T> inline void set_field(rjson::object& target, const char* name, const acmacs::internal::field_optional_with_default<T>& field)
{
    if (field.not_default())
        target.set_field(name, rjson::to_value(*field));
}

void export_style(rjson::array& target_styles, const acmacs::PointStyle& aStyle)
{
    rjson::object& st = target_styles.insert(rjson::object{});
    set_field(st, "+", aStyle.shown);
    set_field(st, "F", aStyle.fill);
    set_field(st, "O", aStyle.outline);
    set_field(st, "o", aStyle.outline_width);
    set_field(st, "s", aStyle.size);
    set_field(st, "r", aStyle.rotation);
    set_field(st, "a", aStyle.aspect);
    set_field(st, "S", aStyle.shape);

    rjson::object ls;
    set_field(ls, "+", aStyle.label.shown);
    set_field(ls, "t", aStyle.label_text);
    set_field(ls, "f", aStyle.label.style.font_family);
    set_field(ls, "S", aStyle.label.style.slant);
    set_field(ls, "W", aStyle.label.style.weight);
    set_field(ls, "s", aStyle.label.size);
    set_field(ls, "c", aStyle.label.color);
    set_field(ls, "r", aStyle.label.rotation);
    set_field(ls, "i", aStyle.label.interline);
    set_field(ls, "p", aStyle.label.offset);
    if (!ls.empty())
        st.set_field("l", std::move(ls));

} // export_style

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
