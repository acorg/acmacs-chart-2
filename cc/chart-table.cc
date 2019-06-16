#include <numeric>

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
            auto chart = acmacs::chart::import_from_file((*opt.charts)[file_no]);
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            auto titers = chart->titers();
            const auto max_antigen_name = antigens->max_full_name();

            const auto column_width = 8;
            const auto table_prefix = 5;
            fmt::print("{: >{}s}  ", "", max_antigen_name + table_prefix);
            for (auto serum_no : acmacs::range(sera->size()))
                fmt::print("{: ^{}d}", serum_no, column_width);
            fmt::print("\n");
            fmt::print("{: >{}s}  ", "", max_antigen_name + table_prefix);
            for (auto serum : *sera)
                fmt::print("{: ^8s}", serum->abbreviated_location_year(), column_width);
            fmt::print("\n\n");

            for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
                fmt::print("{:3d} {: <{}s} ", ag_no, antigen->full_name(), max_antigen_name);
                for (auto serum_no : acmacs::range(sera->size()))
                    fmt::print("{: >{}s}", titers->titer(ag_no, serum_no), column_width);
                fmt::print("\n");
            }

            fmt::print("\n");
            for (auto [sr_no, serum] : acmacs::enumerate(*sera))
                fmt::print("{: >{}s} {:3d} {}\n", "", max_antigen_name + table_prefix, sr_no, serum->full_name());
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
