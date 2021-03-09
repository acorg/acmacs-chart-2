#include "acmacs-base/argv.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str_array> charts{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        for (size_t file_no = 0; file_no < opt.charts->size(); ++file_no) {
            const auto& filename = (*opt.charts)[file_no];
            auto chart = acmacs::chart::import_from_file(filename);
            auto antigens = chart->antigens();
            for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
                if (antigen->passage().empty())
                    fmt::print("{} {:3d} {}\n", filename, ag_no, antigen->format("{name_full}"));
            }
        }
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
