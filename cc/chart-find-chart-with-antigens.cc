#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static void process(const std::vector<std::string_view>& names, const std::vector<std::string_view>& chart_file_names);

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> names{*this, arg_name{"names"}, mandatory, desc{"file with names"}};
    argument<str> charts{*this, arg_name{"charts"}, mandatory, desc{"file with chart file names"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        process(acmacs::string::split(static_cast<std::string>(acmacs::file::read(opt.names)), "\n"), acmacs::string::split(static_cast<std::string>(acmacs::file::read(opt.charts)), "\n"));
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void process(const std::vector<std::string_view>& names, const std::vector<std::string_view>& chart_file_names)
{
    for (const auto& filename : chart_file_names) {
        auto chart = acmacs::chart::import_from_file(filename);
        if (chart->number_of_projections()) {
            fmt::print("INFO: {}\n", filename);
        }
    }

} // process

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
