#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/text-export.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static std::string export_forced_column_bases_to_text(const acmacs::chart::Chart& chart);
static std::string export_projections_to_text(const acmacs::chart::Chart& chart);
static std::string export_plot_spec_to_text(const acmacs::chart::Chart& chart);
static std::string export_extensions_to_text(const acmacs::chart::Chart& chart);
static std::string export_style_to_text(const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

std::string acmacs::chart::export_text(const Chart& chart)
{
    fmt::memory_buffer result;
    fmt::format_to_mb(result, "{}",
                   acmacs::string::join(acmacs::string::join_sep_t{"\n\n"}, export_info_to_text(chart), export_table_to_text(chart), export_forced_column_bases_to_text(chart), export_projections_to_text(chart),
                                        export_plot_spec_to_text(chart), export_extensions_to_text(chart)),
                   "\n", acmacs::string::Split::StripKeepEmpty);
    return fmt::to_string(result);

    // // Timeit ti_plot_spec("export plot_spec ");
    // if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty())
    //     export_plot_spec(ace["c"]["p"], plot_spec);
    //   // ti_plot_spec.report();
    // if (const auto& ext = aChart.extension_fields(); ext.is_object())
    //      ace["c"]["x"] = ext;
} // acmacs::chart::export_text

// ----------------------------------------------------------------------

std::string acmacs::chart::export_table_to_text(const Chart& chart, std::optional<size_t> just_layer, bool sort)
{
    using namespace std::string_view_literals;
    fmt::memory_buffer result;
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto titers = chart.titers();
    const auto max_antigen_name = max_full_name(*antigens);

    if (just_layer.has_value()) {
        if (*just_layer >= titers->number_of_layers())
            throw std::runtime_error(fmt::format("Invalid layer: {}, number of layers in the chart: {}", *just_layer, titers->number_of_layers()));
        fmt::format_to_mb(result, "Layer {}   {}\n\n", *just_layer, chart.info()->source(*just_layer)->make_name());
    }

    auto antigen_order = range_from_0_to(antigens->size()) | ranges::to_vector;
    auto serum_order = range_from_0_to(sera->size()) | ranges::to_vector;
    if (sort) {
        ranges::sort(antigen_order, [&antigens](auto i1, auto i2) { return antigens->at(i1)->name_full() < antigens->at(i2)->name_full(); });
        ranges::sort(serum_order, [&sera](auto i1, auto i2) { return sera->at(i1)->name_full() < sera->at(i2)->name_full(); });
    }

    const auto column_width = 8;
    const auto table_prefix = 5;
    const std::string_view reference_marker{":ref"};
    fmt::format_to_mb(result, "{: >{}s}  ", "", max_antigen_name + table_prefix + (reference_marker.size() + 1));
    for (auto serum_no : range_from_0_to(sera->size()))
        fmt::format_to_mb(result, "{: ^{}d}", serum_no, column_width);
    fmt::format_to_mb(result, "\n");
    fmt::format_to_mb(result, "{: >{}s}  ", "", max_antigen_name + table_prefix + (reference_marker.size() + 1));
    for (auto serum_no : serum_order)
        fmt::format_to_mb(result, "{: ^8s}", sera->at(serum_no)->format("{location_abbreviated}/{year2}"), column_width);
    fmt::format_to_mb(result, "\n\n");

    const auto ag_no_num_digits = chart.number_of_digits_for_antigen_serum_index_formatting();
    if (!just_layer.has_value()) {
        // merged table
        for (auto [ag_no, antigen_no] : acmacs::enumerate(antigen_order)) {
            auto antigen = antigens->at(antigen_no);
            fmt::format_to_mb(result, "{:{}d} {: <{}s} {:<{}s}", ag_no, ag_no_num_digits, antigen->name_full(), max_antigen_name, antigen->reference() ? reference_marker : ""sv,
                           reference_marker.size() + 1);
            for (auto serum_no : serum_order)
                fmt::format_to_mb(result, "{: >{}s}", *titers->titer(antigen_no, serum_no), column_width);
            if (const auto date = antigen->date(); !date.empty())
                fmt::format_to_mb(result, "{:{}s}[{}]", "", column_width, date);
            fmt::format_to_mb(result, "\n");
        }
    }
    else {
        // just layer
        const auto [antigens_of_layer, sera_of_layer] = titers->antigens_sera_of_layer(*just_layer);
        for (auto ag_no : antigens_of_layer) {
            auto antigen = antigens->at(ag_no);
            fmt::format_to_mb(result, "{:{}d} {: <{}s} {:<6s}", ag_no, ag_no_num_digits, antigen->name_full(), max_antigen_name, antigen->reference() ? "<ref>"sv : ""sv);
            for (auto serum_no : serum_order)
                fmt::format_to_mb(result, "{: >{}s}", *titers->titer_of_layer(*just_layer, ag_no, serum_no), column_width);
            fmt::format_to_mb(result, "\n");
        }
    }

    fmt::format_to_mb(result, "\n");
    for (auto [sr_no, serum_no] : acmacs::enumerate(serum_order))
        fmt::format_to_mb(result, "{: >{}s} {:3d} {}\n", "", max_antigen_name + table_prefix, sr_no, sera->at(serum_no)->format("{name_full_passage} {serum_id}"));

    return fmt::to_string(result);

} // acmacs::chart::export_table_to_text

// ----------------------------------------------------------------------

std::string acmacs::chart::export_info_to_text(const Chart& chart)
{
    fmt::memory_buffer result;

    const auto do_export = [&result](acmacs::chart::InfoP info) {
        fmt::format_to_mb(result, "{}", acmacs::string::join(acmacs::string::join_space, info->virus(), info->virus_type(), info->assay(), info->date(), info->name(), info->lab(), info->rbc_species(), info->subset()));
        // info->table_type()
    };

    auto info = chart.info();
    do_export(info);
    if (const auto number_of_sources = info->number_of_sources(); number_of_sources) {
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            fmt::format_to_mb(result, "\n{:2d}  ", source_no);
            do_export(info->source(source_no));
        }
    }
    return fmt::to_string(result);

} // acmacs::chart::export_info_to_text

// ----------------------------------------------------------------------

std::string export_forced_column_bases_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;
    if (const auto column_bases = chart.forced_column_bases(acmacs::chart::MinimumColumnBasis{}); column_bases) {
        fmt::format_to_mb(result, "forced-column-bases:");
        for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
            fmt::format_to_mb(result, "{}", column_bases->column_basis(sr_no));
    }
    return fmt::to_string(result);

} // export_forced_column_bases_to_text

// ----------------------------------------------------------------------

std::string export_projections_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;
    if (auto projections = chart.projections(); !projections->empty()) {
        fmt::format_to_mb(result, "projections: {}\n\n", projections->size());
        for (size_t projection_no = 0; projection_no < projections->size(); ++projection_no) {
            fmt::format_to_mb(result, "projection {}\n", projection_no);
            auto projection = (*projections)[projection_no];
            if (const auto comment = projection->comment(); !comment.empty())
                fmt::format_to_mb(result, "  comment: {}\n", comment);
            if (const auto stress = projection->stress(); !std::isnan(stress) && stress >= 0)
                fmt::format_to_mb(result, "  stress: {:.10f}\n", stress); // to avoid problems comparing exported charts during test on linux
            if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
                fmt::format_to_mb(result, "  minimum-column-basis: {}\n", minimum_column_basis);
            if (const auto column_bases = projection->forced_column_bases(); column_bases) {
                fmt::format_to_mb(result, "  forced-column-bases:");
                for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
                    fmt::format_to_mb(result, "{}", column_bases->column_basis(sr_no));
                fmt::format_to_mb(result, "\n");
            }
            if (const auto transformation = projection->transformation(); transformation != acmacs::Transformation{} && transformation.valid())
                fmt::format_to_mb(result, "  transformation: {}\n", transformation.as_vector());
            if (projection->dodgy_titer_is_regular() == acmacs::chart::dodgy_titer_is_regular::yes)
                fmt::format_to_mb(result, "  dodgy-titer-is-regular: {}\n", true);
            if (projection->stress_diff_to_stop() > 0)
                fmt::format_to_mb(result, "  stress-diff-to-stop: {}\n", projection->stress_diff_to_stop());
            if (const auto unmovable = projection->unmovable(); !unmovable->empty())
                fmt::format_to_mb(result, "  unmovable: {}\n", unmovable);
            if (const auto disconnected = projection->disconnected(); !disconnected->empty())
                fmt::format_to_mb(result, "  disconnected: {}\n", disconnected);
            if (const auto unmovable_in_the_last_dimension = projection->unmovable_in_the_last_dimension(); !unmovable_in_the_last_dimension->empty())
                fmt::format_to_mb(result, "  unmovable-in-the-last-dimension: {}\n", unmovable_in_the_last_dimension);
            if (const auto avidity_adjusts = projection->avidity_adjusts(); !avidity_adjusts.empty())
                fmt::format_to_mb(result, "  avidity-adjusts: {}\n", avidity_adjusts);

            auto layout = projection->layout();
            const auto number_of_dimensions = layout->number_of_dimensions();
            fmt::format_to_mb(result, "  layout {} x {}\n", layout->number_of_points(), number_of_dimensions);
            if (const auto number_of_points = layout->number_of_points(); number_of_points && acmacs::valid(number_of_dimensions)) {
                for (size_t p_no = 0; p_no < number_of_points; ++p_no)
                    fmt::format_to_mb(result, fmt::runtime("    {:4d} {:13.10f}\n"), p_no, layout->at(p_no)); // to avoid problems comparing exported charts during test on linux
            }
        }
    }
    return fmt::to_string(result);

} // export_projections_to_text

// ----------------------------------------------------------------------

std::string export_plot_spec_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;

    if (auto plot_spec = chart.plot_spec(); !plot_spec->empty()) {
        fmt::format_to_mb(result, "plot-spec:\n");
        if (const auto drawing_order = plot_spec->drawing_order(); !drawing_order->empty()) {
            fmt::format_to_mb(result, "  drawing-order:");
            for (const auto& index : drawing_order)
                fmt::format_to_mb(result, " {}", index);
            fmt::format_to_mb(result, "\n");
        }
        if (const auto color = plot_spec->error_line_positive_color(); color != RED)
            fmt::format_to_mb(result, "  error-line-positive-color: {}\n", color);
        if (const auto color = plot_spec->error_line_negative_color(); color != BLUE)
            fmt::format_to_mb(result, "  error-line-negative-color: {}\n", color);

        const auto compacted = plot_spec->compacted();

        fmt::format_to_mb(result, "  plot-style-per-point:");
        for (const auto& index : compacted.index)
            fmt::format_to_mb(result, " {}", index);
        fmt::format_to_mb(result, "\n");

        fmt::format_to_mb(result, "  styles: {}\n", compacted.styles.size());
        for (size_t style_no = 0; style_no < compacted.styles.size(); ++style_no)
            fmt::format_to_mb(result, "    {:2d} {}\n", style_no, export_style_to_text(compacted.styles[style_no]));

        // "g": {},                  // ? grid data
        // "l": [],                  // ? for each procrustes line, index in the "L" list
        // "L": []                    // ? list of procrustes lines styles
        // "s": [],                  // list of point indices for point shown on all maps in the time series
        // "t": {}                    // title style?
    }
    return fmt::to_string(result);

} // export_plot_spec_to_text

// ----------------------------------------------------------------------

std::string export_style_to_text(const acmacs::PointStyle& aStyle)
{
    fmt::memory_buffer result;
    fmt::format_to_mb(result, "shown:{} fill:\"{}\" outline:\"{}\" outline_width:{} size:{} rotation:{} aspect:{} shape:{}", aStyle.shown(), aStyle.fill(), aStyle.outline(), aStyle.outline_width().value(),
                   aStyle.size().value(), aStyle.rotation(), aStyle.aspect(), aStyle.shape());
    fmt::format_to_mb(result, " label:{{ shown:{} text:\"{}\" font_family:\"{}\" slant:\"{}\" weight:\"{}\" size:{} color:\"{}\" rotation:{} interline:{} offset:{}",
                   aStyle.label().shown, aStyle.label_text(), aStyle.label().style.font_family, aStyle.label().style.slant, aStyle.label().style.weight, aStyle.label().size.value(), aStyle.label().color,
                   aStyle.label().rotation, aStyle.label().interline, aStyle.label().offset);

    return fmt::to_string(result);

} // export_style_to_text

// ----------------------------------------------------------------------

std::string export_extensions_to_text(const acmacs::chart::Chart& chart)
{
    fmt::memory_buffer result;
    if (const auto& ext = chart.extension_fields(); ext.is_object())
        fmt::format_to_mb(result, "extensions:\n{}\n", rjson::pretty(ext));
    return fmt::to_string(result);

} // export_extensions_to_text

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
