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
    option<str_array> prepend_field{*this, "prepend", desc{"fields to prepend: \"ag\", \"name\", \"color\", default: \"ag\" \"name\""}};
    option<str_array> append_field{*this, "append", desc{"fields to append: \"ag\", \"name\", \"color\", default: nothing"}};

    argument<str> input_chart{*this, arg_name{"chart-file"}, mandatory};
    argument<str> output_layout{*this, arg_name{"output-layout.{txt,csv}[.xz]"}, mandatory};
};

static void write_csv(std::string_view aFilename, const Options& opt, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout);
static void write_text(std::string_view aFilename, const Options& opt, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout);
static std::string encode_name(std::string_view aName, std::string_view aFieldSeparator);

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;

    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        auto antigens = chart->antigens();
        auto sera = chart->sera();
        auto layout = chart->projection(opt.projection)->layout();
        if (acmacs::string::endswith(*opt.output_layout, ".csv"sv) || acmacs::string::endswith(*opt.output_layout, ".csv.xz"sv) || opt.field_separator == ","sv)
            write_csv(opt.output_layout, opt, antigens, sera, layout);
        else
            write_text(opt.output_layout, opt, antigens, sera, layout);
        // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void write_csv(std::string_view aFilename, const Options& opt, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout)
{
    std::vector<std::string> prepend{"ag", "name"}, append;
    AD_DEBUG("prep {}", prepend);
    if (!opt.prepend_field.empty()) {
        prepend.clear();
        std::transform(opt.prepend_field->begin(), opt.prepend_field->end(), std::back_inserter(prepend), [](const auto& src) { return std::string{src}; });
    }
    if (!opt.append_field.empty()) {
        append.clear();
        std::transform(opt.append_field->begin(), opt.append_field->end(), std::back_inserter(append), [](const auto& src) { return std::string{src}; });
    }
    AD_DEBUG("prep {}", prepend);

    acmacs::CsvWriter writer;
    const auto number_of_dimensions = layout->number_of_dimensions();
    if (opt.add_header) {
        for (const auto& prep : prepend)
            writer.add_field(prep);
        for (auto dim : acmacs::range(number_of_dimensions))
            writer.add_field(fmt::format("{}", dim));
        for (const auto& app : append)
            writer.add_field(app);
        writer.new_row();
    }
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        writer.add_field("AG");
        writer.add_field(antigen->full_name());
        for (auto dim : acmacs::range(number_of_dimensions))
            writer.add_field(acmacs::to_string(layout->coordinate(ag_no, dim)));
        writer.new_row();
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        writer.add_field("SR");
        writer.add_field(serum->full_name());
        for (auto dim : acmacs::range(number_of_dimensions))
            writer.add_field(acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
        writer.new_row();
    }
    acmacs::file::write(aFilename, writer);

} // write_csv

// ----------------------------------------------------------------------

void write_text(std::string_view aFilename, const Options& opt, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout)
{
    std::string result;
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        result += acmacs::string::concat("AG", opt.field_separator, encode_name(antigen->full_name(), opt.field_separator));
        for (auto dim : acmacs::range(number_of_dimensions))
            result += acmacs::string::concat(opt.field_separator, acmacs::to_string(layout->coordinate(ag_no, dim)));
        result += '\n';
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
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
