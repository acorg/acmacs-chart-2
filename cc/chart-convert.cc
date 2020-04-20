#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> format{*this, 'f', "format", desc{"ace, save, text, table"}};

    argument<str> input_chart{*this, arg_name{"input-chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        if (!opt.format && (!opt.output_chart || opt.output_chart == "-" || opt.output_chart == "="))
            throw std::runtime_error{"either -f or output-chart-file must be present"};
        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        fmt::print("{}\n", chart->make_info());
        if (opt.format) {
            auto fmt{acmacs::chart::export_format::ace};
            if (opt.format == "save" || opt.format == "lispmds")
                fmt = acmacs::chart::export_format::save;
            else if (opt.format == "text" || opt.format == "txt")
                fmt = acmacs::chart::export_format::text;
            else if (opt.format == "table")
                fmt = acmacs::chart::export_format::text_table;
            else if (!(opt.format == "ace"))
                throw std::runtime_error{fmt::format("unrecognized format: {}", opt.format)};
            acmacs::file::write(opt.output_chart ? *opt.output_chart : "-"sv, acmacs::chart::export_factory(*chart, fmt, opt.program_name()));
        }
        else
            acmacs::chart::export_factory(*chart, opt.output_chart, opt.program_name());
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
