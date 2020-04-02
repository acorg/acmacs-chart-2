#include "acmacs-base/argv.hh"
#include "acmacs-base/format-double.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> recalculate{*this, 'r', "recalculate"};
    argument<str> chart{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(*opt.chart, acmacs::chart::Verify::None, report_time::no);
        auto projections = chart->projections();
        for (auto projection : *projections)
            fmt::print("{}\n", acmacs::format_double(projection->stress(opt.recalculate ? acmacs::chart::RecalculateStress::yes : acmacs::chart::RecalculateStress::if_necessary)));
    }
    catch (std::exception& err) {
        fmt::print(stderr, "> ERROR {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
