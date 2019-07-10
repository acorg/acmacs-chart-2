#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/csv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

static void write_csv(std::string aFilename, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout);
static void write_text(std::string aFilename, std::string_view aFieldSeparator, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout);
static std::string encode_name(std::string_view aName, std::string_view aFieldSeparator);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> projection{*this, "projection", dflt{0UL}};
    option<str>    field_separator{*this, "field-separator", dflt{" "}};

    argument<str> input_chart{*this, arg_name{"chart-file"}, mandatory};
    argument<str> output_layout{*this, arg_name{"output-layout.{txt,csv}[.xz]"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        auto antigens = chart->antigens();
        auto sera = chart->sera();
        auto layout = chart->projection(opt.projection)->layout();
        if (*opt.output_layout == "-")
            write_text(std::string{opt.output_layout}, opt.field_separator, antigens, sera, layout);
        else if (string::ends_with(*opt.output_layout, ".csv") || string::ends_with(*opt.output_layout, ".csv.xz"))
            write_csv(std::string{opt.output_layout}, antigens, sera, layout);
        else
            write_text(std::string{opt.output_layout}, opt.field_separator, antigens, sera, layout);
        // const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void write_csv(std::string aFilename, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout)
{
    acmacs::CsvWriter writer;
    const auto number_of_dimensions = layout->number_of_dimensions();
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

void write_text(std::string aFilename, std::string_view aFieldSeparator, std::shared_ptr<acmacs::chart::Antigens> antigens, std::shared_ptr<acmacs::chart::Sera> sera, std::shared_ptr<acmacs::Layout> layout)
{
    std::string result;
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
        result += string::concat("AG", aFieldSeparator, encode_name(antigen->full_name(), aFieldSeparator));
        for (auto dim : acmacs::range(number_of_dimensions))
            result += string::concat(aFieldSeparator, acmacs::to_string(layout->coordinate(ag_no, dim)));
        result += '\n';
    }
    const auto number_of_antigens = antigens->size();
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        result += string::concat("SR", aFieldSeparator, encode_name(serum->full_name(), aFieldSeparator));
        for (auto dim : acmacs::range(number_of_dimensions))
            result += string::concat(aFieldSeparator, acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
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


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
