#include "acmacs-base/argv.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/read-file.hh"
// #include "acmacs-base/csv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> projection_no{*this, "projection", dflt{0UL}};

    argument<str> input_chart{*this, arg_name{"chart-file"}, mandatory};
    argument<str> output_distances{*this, arg_name{"output-distances.{txt,csv,json}[.xz]"}, mandatory};
};

static void write_csv(std::string_view aFilename, size_t projection_no, const acmacs::chart::Chart& chart);
static void write_json(std::string_view aFilename, size_t projection_no, const acmacs::chart::Chart& chart);
static void write_text(std::string_view aFilename, size_t projection_no, const acmacs::chart::Chart& chart);

// static std::string encode_name(std::string_view aName, std::string_view aFieldSeparator);
// static std::string field(const acmacs::chart::Chart& chart, std::string_view field_name, size_t point_no);

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;

    int exit_code = 0;
    try {
        Options opt(argc, argv);

        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        if (acmacs::string::endswith(*opt.output_distances, ".csv"sv) || acmacs::string::endswith(*opt.output_distances, ".csv.xz"sv))
            write_csv(opt.output_distances, opt.projection_no, *chart);
        else if (acmacs::string::endswith(*opt.output_distances, ".json"sv) || acmacs::string::endswith(*opt.output_distances, ".json.xz"sv))
            write_json(opt.output_distances, opt.projection_no, *chart);
        else
            write_text(opt.output_distances, opt.projection_no, *chart);
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void write_csv(std::string_view /*aFilename*/, size_t /*projection_no*/, const acmacs::chart::Chart& /*chart*/)
{
    throw std::runtime_error{"csv output not implemented"};
}

// ----------------------------------------------------------------------

void write_json(std::string_view /*aFilename*/, size_t /*projection_no*/, const acmacs::chart::Chart& /*chart*/)
{
    throw std::runtime_error{"json output not implemented"};
}

// ----------------------------------------------------------------------

// c2: acmacs/core/chart.py:1103 distances_between_all_points
void write_text(std::string_view aFilename, size_t projection_no, const acmacs::chart::Chart& chart)
{
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto projection = (*chart.projections())[projection_no];
    auto layout = projection->layout();
    const auto number_of_antigens = antigens->size();
    const auto name = [number_of_antigens, &antigens, &sera](size_t p_no) {
        if (p_no < number_of_antigens)
            return fmt::format("{}-AG", antigens->at(p_no)->format("{name_full}"));
        else
            return fmt::format("{}-SR", sera->at(p_no - number_of_antigens)->format("{name_full}"));
    };

    const auto number_of_points = layout->number_of_points();
    fmt::memory_buffer out;
    for (const auto p1 : range_from_0_to(number_of_points - 1)) {
        for (const auto p2 : range_from_to(p1 + 1, number_of_points)) {
            const auto distance = layout->distance(p1, p2);
            fmt::format_to_mb(out, "{}\t{}\t{}\n", name(p1), name(p2), distance);
        }
    }
    acmacs::file::write(aFilename, fmt::to_string(out));
}

// ----------------------------------------------------------------------

// void write_csv(std::string_view aFilename, const Options& opt, const acmacs::chart::Chart& chart, const std::vector<std::string>& prepend_fields, const std::vector<std::string>& append_fields)
// {
//     auto antigens = chart.antigens();
//     auto sera = chart.sera();
//     auto layout = chart.projection(opt.projection)->layout();
//     acmacs::CsvWriter writer;
//     const auto number_of_dimensions = layout->number_of_dimensions();
//     if (opt.add_header) {
//         for (const auto& prep : prepend_fields)
//             writer.add_field(prep);
//         for (auto dim : acmacs::range(number_of_dimensions))
//             writer.add_field(fmt::format("{}", dim));
//         for (const auto& app : append_fields)
//             writer.add_field(app);
//         writer.new_row();
//     }
//     for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
//         for (const auto& prep : prepend_fields)
//             writer.add_field(field(chart, prep, ag_no));
//         for (auto dim : acmacs::range(number_of_dimensions))
//             writer.add_field(acmacs::to_string(layout->coordinate(ag_no, dim)));
//         for (const auto& app : append_fields)
//             writer.add_field(field(chart, app, ag_no));
//         writer.new_row();
//     }
//     const auto number_of_antigens = antigens->size();
//     for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
//         for (const auto& prep : prepend_fields)
//             writer.add_field(field(chart, prep, sr_no + number_of_antigens));
//         for (auto dim : acmacs::range(number_of_dimensions))
//             writer.add_field(acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
//         for (const auto& app : append_fields)
//             writer.add_field(field(chart, app, sr_no + number_of_antigens));
//         writer.new_row();
//     }
//     acmacs::file::write(aFilename, writer);

// } // write_csv

// // ----------------------------------------------------------------------

// void write_text(std::string_view aFilename, const Options& opt, const acmacs::chart::Chart& chart, const std::vector<std::string>& /*prepend_fields*/, const std::vector<std::string>& /*append_fields*/)
// {
//     auto antigens = chart.antigens();
//     auto sera = chart.sera();
//     auto layout = chart.projection(opt.projection)->layout();

//     std::string result;
//     const auto number_of_dimensions = layout->number_of_dimensions();
//     for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
//         result += fmt::format("AG{}{}{}{}", opt.field_separator, ag_no, opt.field_separator, encode_name(antigen->format("{name_full}"), opt.field_separator));
//         for (auto dim : acmacs::range(number_of_dimensions))
//             result += acmacs::string::concat(opt.field_separator, acmacs::to_string(layout->coordinate(ag_no, dim)));
//         result += '\n';
//     }
//     const auto number_of_antigens = antigens->size();
//     for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
//         result += fmt::format("SR{}{}{}{}", opt.field_separator, sr_no, opt.field_separator, encode_name(serum->format("{name_full}"), opt.field_separator));
//         for (auto dim : acmacs::range(number_of_dimensions))
//             result += acmacs::string::concat(opt.field_separator, acmacs::to_string(layout->coordinate(sr_no + number_of_antigens, dim)));
//         result += '\n';
//     }
//     acmacs::file::write(aFilename, result);

// } // write_text

// // ----------------------------------------------------------------------

// std::string encode_name(std::string_view aName, std::string_view aFieldSeparator)
// {
//     if (!aFieldSeparator.empty() && aFieldSeparator[0] == ' ')
//         return string::replace(aName, " ", "_");
//     else
//         return std::string(aName);

// } // encode_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
