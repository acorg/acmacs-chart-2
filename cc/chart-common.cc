#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> match{*this, "match", dflt{"auto"}, desc{"match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""}};

    argument<str> chart1{*this, arg_name{"chart1"}, mandatory};
    argument<str> chart2{*this, arg_name{"chart2"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto match_level = acmacs::chart::CommonAntigensSera::match_level(*opt.match);
        if (*opt.chart1 == *opt.chart2) {
            auto chart1 = acmacs::chart::import_from_file(*opt.chart1, acmacs::chart::Verify::None);
            acmacs::chart::CommonAntigensSera common(*chart1);
            fmt::print("{}\n", common.report());
        }
        else {
            auto chart1 = acmacs::chart::import_from_file(opt.chart1, acmacs::chart::Verify::None);
            auto chart2 = acmacs::chart::import_from_file(opt.chart2, acmacs::chart::Verify::None);
            acmacs::chart::CommonAntigensSera common(*chart1, *chart2, match_level);
            fmt::print("{}\n", common.report());
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
