#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>

#include "acmacs-base/argv.hh"
#include "acmacs-base/time.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using fields_t = std::map<std::string, std::vector<std::string>>;

static void print_plot_spec(std::string_view chart_filename);
static std::map<std::string, size_t> field_max_length(const fields_t& aFields);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> chart{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        print_plot_spec(opt.chart);
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void print_plot_spec(std::string_view chart_filename)
{
    auto chart = acmacs::chart::import_from_file(chart_filename);
    auto antigens = chart->antigens();
    auto sera = chart->sera();
    auto plot_spec = chart->plot_spec();
    const auto drawing_order = plot_spec->drawing_order();
    const auto number_of_antigens = chart->number_of_antigens();
    const auto number_of_sera = chart->number_of_sera();

    auto bool_to_string = [](bool b) -> std::string { return b ? "True" : "False"; };

    fields_t antigen_fields;
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        const size_t point_no = ag_no;
        antigen_fields["I"].push_back(acmacs::to_string(ag_no));
        antigen_fields["name"].emplace_back(antigen->name());
        antigen_fields["reassortant"].emplace_back(antigen->reassortant());
        antigen_fields["annotations"].push_back(antigen->annotations().join());
        antigen_fields["passage"].emplace_back(antigen->passage());
        const auto style = plot_spec->style(ag_no);
        antigen_fields["fill_color"].push_back(style.fill->to_string());
        antigen_fields["outline_color"].push_back(style.outline->to_string());
        antigen_fields["outline_width"].push_back(acmacs::to_string(*style.outline_width));
        antigen_fields["size"].push_back(acmacs::to_string(*style.size));
        antigen_fields["shown"].push_back(bool_to_string(*style.shown));
        antigen_fields["aspect"].push_back(acmacs::to_string(*style.aspect));
        antigen_fields["rotation"].push_back(acmacs::to_string(*style.rotation));
        antigen_fields["shape"].push_back(*style.shape);
        antigen_fields["label_shown"].push_back(bool_to_string(style.label.shown));
        antigen_fields["label_color"].push_back(style.label.color->to_string());
        antigen_fields["label_font_face"].push_back(*style.label.style.font_family);
        antigen_fields["label_font_slant"].push_back(*style.label.style.slant);
        antigen_fields["label_font_weight"].push_back(*style.label.style.weight);
        antigen_fields["label_position_x"].push_back(acmacs::to_string(style.label.offset->x()));
        antigen_fields["label_position_y"].push_back(acmacs::to_string(style.label.offset->y()));
        antigen_fields["label_rotation"].push_back(acmacs::to_string(*style.label.rotation));
        antigen_fields["label_size"].push_back(acmacs::to_string(*style.label.size));
        antigen_fields["label"].push_back(*style.label_text);
        antigen_fields["drawing_level"].push_back(acmacs::to_string(drawing_order.index_of(point_no)));
    }
      // sort antigens by name on output
    auto antigen_fields_indices = acmacs::filled_with_indexes<size_t>(number_of_antigens);
    std::sort(antigen_fields_indices.begin(), antigen_fields_indices.end(), [&antigen_fields](size_t a, size_t b) -> bool { return antigen_fields["name"][a] < antigen_fields["name"][b]; });
    auto antigen_field_lengths = field_max_length(antigen_fields);

    fields_t serum_fields;
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        const size_t point_no = number_of_antigens + sr_no;
        serum_fields["I"].push_back(acmacs::to_string(sr_no));
        serum_fields["name"].emplace_back(serum->name());
        serum_fields["reassortant"].emplace_back(serum->reassortant());
        serum_fields["annotations"].push_back(serum->annotations().join());
        serum_fields["passage"].emplace_back(serum->passage());
        serum_fields["serum_id"].emplace_back(serum->serum_id());
        serum_fields["serum_species"].emplace_back(serum->serum_species());
        auto style = plot_spec->style(point_no);
        serum_fields["fill_color"].push_back(style.fill->to_string());
        serum_fields["outline_color"].push_back(style.outline->to_string());
        serum_fields["outline_width"].push_back(acmacs::to_string(*style.outline_width));
        serum_fields["size"].push_back(acmacs::to_string(*style.size));
        serum_fields["shown"].push_back(bool_to_string(*style.shown));
        serum_fields["aspect"].push_back(acmacs::to_string(*style.aspect));
        serum_fields["rotation"].push_back(acmacs::to_string(*style.rotation));
        serum_fields["shape"].push_back(*style.shape);
        serum_fields["label_shown"].push_back(bool_to_string(style.label.shown));
        serum_fields["label_color"].push_back(style.label.color->to_string());
        serum_fields["label_font_face"].push_back(*style.label.style.font_family);
        serum_fields["label_font_slant"].push_back(*style.label.style.slant);
        serum_fields["label_font_weight"].push_back(*style.label.style.weight);
        serum_fields["label_position_x"].push_back(acmacs::to_string(style.label.offset->x()));
        serum_fields["label_position_y"].push_back(acmacs::to_string(style.label.offset->y()));
        serum_fields["label_rotation"].push_back(acmacs::to_string(*style.label.rotation));
        serum_fields["label_size"].push_back(acmacs::to_string(*style.label.size));
        serum_fields["label"].push_back(*style.label_text);
        serum_fields["drawing_level"].push_back(acmacs::to_string(drawing_order.index_of(point_no)));
    }
      // sort sera by name on output
    auto serum_fields_indices = acmacs::filled_with_indexes<size_t>(number_of_sera);
    std::sort(serum_fields_indices.begin(), serum_fields_indices.end(), [&serum_fields](size_t a, size_t b) -> bool { return serum_fields["name"][a] < serum_fields["name"][b]; });
    auto serum_field_lengths = field_max_length(serum_fields);

    std::cout << "plot_style version 1" << '\n'
              << "# generated by chart-plot-spec on " << acmacs::time_format("%c %Z") << '\n'
              << "#\n# drawing_level - point with the greatest level drawn on top of other ones\n"
              << "# shape - CIRCLE, BOX, TRIANGLE\n"
              << "#\n# Antigens\n#\n";
    for (const auto ag_no: antigen_fields_indices) {
        std::cout << "T=\"AG\"";
        for (std::string field_name: {
                "I", "name", "reassortant", "annotations", "passage", "fill_color", "outline_color", "outline_width", "size", "shown", "aspect", "rotation", "shape",
                "label_shown", "label_color", "label_font_face", "label_font_slant", "label_font_weight", "label_position_x", "label_position_y", "label_rotation", "label_size", "drawing_level"}) {
            const std::string value = field_name + "=\"" + antigen_fields[field_name][ag_no] + '"';
            std::cout << ' ' << std::left << std::setw(static_cast<int>(antigen_field_lengths[field_name] + field_name.size() + 3)) << value;
        }
        std::cout << '\n';
    }
    std::cout << "#\n# Sera\n#\n";
    for (const auto sr_no: serum_fields_indices) {
        std::cout << "T=\"SR\"";
        for (std::string field_name: {
                "I", "name", "reassortant", "annotations", "serum_id", "serum_species", "passage", "fill_color", "outline_color", "outline_width", "size", "shown", "aspect", "rotation", "shape",
                "label_shown", "label_color", "label_font_face", "label_font_slant", "label_font_weight", "label_position_x", "label_position_y", "label_rotation", "label_size", "drawing_level"}) {
            const std::string value = field_name + "=\"" + serum_fields[field_name][sr_no] + '"';
            std::cout << ' ' << std::left << std::setw(static_cast<int>(serum_field_lengths[field_name] + field_name.size() + 3)) << value;
        }
        std::cout << '\n';
    }

} // print_plot_spec

// ----------------------------------------------------------------------

std::map<std::string, size_t> field_max_length(const fields_t& aFields)
{
    std::map<std::string, size_t> result;
    for (auto [name, values]: aFields) {
        result[name] = std::max_element(std::begin(values), std::end(values), [](const auto& a, const auto& b) { return a.size() < b.size(); })->size();
    }
    return result;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
