#include "acmacs-base/argv.hh"
#include "acmacs-base/debug.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/csv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> projection{*this, "projection", dflt{0UL}};
    option<str>    field_separator{*this, 'f', "field-separator", dflt{" "}};
    option<bool>   add_header{*this, "header"};
    option<str_array> prepend_field{*this, "prepend", desc{R"(fields to prepend: "ag", "name", "no0", "no1", "fill", default: "ag" "name")"}};
    option<str_array> append_field{*this, "append", desc{"fields to append: \"ag\", \"name\", \"fill\", default: nothing"}};

    argument<str> input_chart{*this, arg_name{"chart-file"}, mandatory};
    argument<str> output_layout{*this, arg_name{"output-layout.{txt,csv}[.xz]"}, mandatory};
};

static void write_csv(std::string_view aFilename, const Options& opt, const acmacs::chart::Chart& chart, const std::vector<std::string>& prepend_fields, const std::vector<std::string>& append_fields);
static void write_text(std::string_view aFilename, const Options& opt, const acmacs::chart::Chart& chart, const std::vector<std::string>& prepend_fields, const std::vector<std::string>& append_fields);
static std::string encode_name(std::string_view aName, std::string_view aFieldSeparator);
static std::string field(const acmacs::chart::Chart& chart, std::string_view field_name, size_t point_no);

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;

    int exit_code = 0;
    try {
        Options opt(argc, argv);

        std::vector<std::string> prepend_fields{"ag", "name"}, append_fields;
        if (!opt.prepend_field.empty()) {
            prepend_fields.clear();
            std::transform(opt.prepend_field->begin(), opt.prepend_field->end(), std::back_inserter(prepend_fields), [](const auto& src) { return std::string{src}; });
        }
        if (!opt.append_field.empty()) {
            append_fields.clear();
            std::transform(opt.append_field->begin(), opt.append_field->end(), std::back_inserter(append_fields), [](const auto& src) { return std::string{src}; });
        }

        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        if (acmacs::string::endswith(*opt.output_layout, ".csv"sv) || acmacs::string::endswith(*opt.output_layout, ".csv.xz"sv) || opt.field_separator == ","sv)
            write_csv(opt.output_layout, opt, *chart, prepend_fields, append_fields);
        else
            write_text(opt.output_layout, opt, *chart, prepend_fields, append_fields);
        // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

std::string field(const acmacs::chart::Chart& chart, std::string_view field_name, size_t point_no)
{
    auto antigens = chart.antigens();
    if (point_no < antigens->size()) {
        if (field_name == "ag")
            return "AG";
        else if (field_name == "no0")
            return fmt::format("{}", point_no);
        else if (field_name == "no1")
            return fmt::format("{}", point_no + 1);
        else if (field_name == "name")
            return antigens->at(point_no)->full_name();
        else if (field_name == "fill")
            return fmt::format("{}", chart.plot_spec()->all_styles()[point_no].fill());
        else
            return fmt::format("?{}", field_name);
    }
    else {
        if (field_name == "ag")
            return "SR";
        else if (field_name == "no0")
            return fmt::format("{}", point_no - antigens->size());
        else if (field_name == "no1")
            return fmt::format("{}", point_no + 1 - antigens->size());
        else if (field_name == "name")
            return chart.sera()->at(point_no - antigens->size())->full_name();
        else if (field_name == "fill")
            return fmt::format("{}", chart.plot_spec()->all_styles()[point_no].fill());
        else
            return fmt::format("?{}", field_name);
    }

} // field

// ----------------------------------------------------------------------

void write_csv(std::string_view aFilename, const Options& opt, const acmacs::chart::Chart& chart, const std::vector<std::string>& prepend_fields, const std::vector<std::string>& append_fields)
{
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto layout = chart.projection(opt.projection)->layout();
    acmacs::CsvWriter writer;
    const auto number_of_dimensions = layout->number_of_dimensions();
    if (opt.add_header) {
        for (const auto& prep : prepend_fields)
            writer.add_field(prep);
        for (auto dim : acmacs::range(number_of_dimensions))
            writer.add_field(fmt::format("{}", dim));
        for (const auto& app : append_fields)
            writer.add_field(app);
        writer.new_row();
    }
    for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
        for (const auto& prep : prepend_fields)
            writer.add_field(field(chart, prep, ag_no));
        for (auto dim : acmacs::range(number_of_dimensions))
            writer.add_field(acmacs::to_string(layout->coordinate(ag_no, dim)));
        for (const auto& app : append_fields)
            writer.add_field(field(chart, app, ag_no));
        writer.new_row();
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        for (const auto& prep : prepend_fields)
            writer.add_field(field(chart, prep, sr_no + number_of_antigens));
        for (auto dim : acmacs::range(number_of_dimensions))
            writer.add_field(acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
        for (const auto& app : append_fields)
            writer.add_field(field(chart, app, sr_no + number_of_antigens));
        writer.new_row();
    }
    acmacs::file::write(aFilename, writer);

} // write_csv

// ----------------------------------------------------------------------

void write_text(std::string_view aFilename, const Options& opt, const acmacs::chart::Chart& chart, const std::vector<std::string>& /*prepend_fields*/, const std::vector<std::string>& /*append_fields*/)
{
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto layout = chart.projection(opt.projection)->layout();

    std::string result;
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
        result += acmacs::string::concat("AG", opt.field_separator, encode_name(antigen->full_name(), opt.field_separator));
        for (auto dim : acmacs::range(number_of_dimensions))
            result += acmacs::string::concat(opt.field_separator, acmacs::to_string(layout->coordinate(ag_no, dim)));
        result += '\n';
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        result += acmacs::string::concat("SR", opt.field_separator, encode_name(serum->full_name(), opt.field_separator));
        for (auto dim : acmacs::range(number_of_dimensions))
            result += acmacs::string::concat(opt.field_separator, acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
        result += '\n';
    }
    acmacs::file::write(aFilename, result);

} // write_text

// ----------------------------------------------------------------------

std::string encode_name(std::string_view aName, std::string_view aFieldSeparator)
{
    if (!aFieldSeparator.empty() && aFieldSeparator[0] == ' ')
        return string::replace(aName, " ", "_");
    else
        return std::string(aName);

} // encode_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
