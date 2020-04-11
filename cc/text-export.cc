#include "acmacs-base/string-join.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/text-export.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::export_text(const Chart& chart)
{
    fmt::memory_buffer result;
    fmt::format_to(result, "{}\n\n{}", export_info_to_text(chart), export_table_to_text(chart));
    return fmt::to_string(result);

    // export_info(ace["c"]["i"], aChart.info());
    // export_forced_column_bases(ace["c"], aChart.forced_column_bases(MinimumColumnBasis{}));
    // if (auto projections = aChart.projections(); !projections->empty())
    //     export_projections(ace["c"]["P"], projections);
    // // ti_projections.report();
    // // Timeit ti_plot_spec("export plot_spec ");
    // if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty())
    //     export_plot_spec(ace["c"]["p"], plot_spec);
    //   // ti_plot_spec.report();
    // if (const auto& ext = aChart.extension_fields(); ext.is_object())
    //      ace["c"]["x"] = ext;
} // acmacs::chart::export_text

// ----------------------------------------------------------------------

std::string acmacs::chart::export_table_to_text(const Chart& chart, std::optional<size_t> just_layer)
{
    fmt::memory_buffer result;
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto titers = chart.titers();
    const auto max_antigen_name = antigens->max_full_name();

    if (just_layer.has_value()) {
        if (*just_layer >= titers->number_of_layers())
            throw std::runtime_error(fmt::format("Invalid layer: {}, number of layers in the chart: {}", *just_layer, titers->number_of_layers()));
        fmt::format_to(result, "Layer {}   {}\n\n", *just_layer, chart.info()->source(*just_layer)->make_name());
    }

    const auto column_width = 8;
    const auto table_prefix = 5;
    fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name + table_prefix);
    for (auto serum_no : acmacs::range(sera->size()))
        fmt::format_to(result, "{: ^{}d}", serum_no, column_width);
    fmt::format_to(result, "\n");
    fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name + table_prefix);
    for (auto serum : *sera)
        fmt::format_to(result, "{: ^8s}", serum->abbreviated_location_year(), column_width);
    fmt::format_to(result, "\n\n");

    const auto ag_no_num_digits = static_cast<int>(std::log10(antigens->size())) + 1;
    if (!just_layer.has_value()) {
        // merged table
        for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
            fmt::format_to(result, "{:{}d} {: <{}s} ", ag_no, ag_no_num_digits, antigen->full_name(), max_antigen_name);
            for (auto serum_no : acmacs::range(sera->size()))
                fmt::format_to(result, "{: >{}s}", *titers->titer(ag_no, serum_no), column_width);
            fmt::format_to(result, "\n");
        }
    }
    else {
        // just layer
        const auto [antigens_of_layer, sera_of_layer] = titers->antigens_sera_of_layer(*just_layer);
        for (auto ag_no : antigens_of_layer) {
            auto antigen = antigens->at(ag_no);
            fmt::format_to(result, "{:{}d} {: <{}s} ", ag_no, ag_no_num_digits, antigen->full_name(), max_antigen_name);
            for (auto serum_no : acmacs::range(sera->size()))
                fmt::format_to(result, "{: >{}s}", *titers->titer_of_layer(*just_layer, ag_no, serum_no), column_width);
            fmt::format_to(result, "\n");
        }
    }

    fmt::format_to(result, "\n");
    for (auto [sr_no, serum] : acmacs::enumerate(*sera))
        fmt::format_to(result, "{: >{}s} {:3d} {}\n", "", max_antigen_name + table_prefix, sr_no, serum->full_name());

    return fmt::to_string(result);

} // acmacs::chart::export_table_to_text

// ----------------------------------------------------------------------

std::string acmacs::chart::export_info_to_text(const Chart& chart)
{
    fmt::memory_buffer result;

    const auto do_export = [&result](acmacs::chart::InfoP info) {
        fmt::format_to(result, "{}", acmacs::string::join(" ", info->virus(), info->virus_type(), info->assay(), info->date(), info->name(), info->lab(), info->rbc_species(), info->subset()));
        // info->table_type()
    };

    auto info = chart.info();
    do_export(info);
    if (const auto number_of_sources = info->number_of_sources(); number_of_sources) {
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            fmt::format_to(result, "\n{:2d}  ", source_no);
            do_export(info->source(source_no));
        }
    }
    return ::string::strip(fmt::to_string(result));

} // acmacs::chart::export_info_to_text

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
