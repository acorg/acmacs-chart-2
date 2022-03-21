#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t>    p1{*this, 'p', "p1", dflt{0UL}, desc{"projection number of the first chart"}};
    option<size_t>    p2{*this, 'r', "p2", dflt{0UL}, desc{"projection number of the second chart"}};
    option<str>       subset{*this, "subset", dflt{"all"}, desc{"all, antigens, sera"}};
    option<str>       match{*this, "match", dflt{"auto"}, desc{"match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""}};
    option<bool>      scaling{*this, "scaling", desc{"use scaling"}};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};

    argument<str>     chart1{*this, arg_name{"chart"}, mandatory};
    argument<str>     chart2{*this, arg_name{"secondary-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);
        const auto match_level = acmacs::chart::CommonAntigensSera::match_level(opt.match);
        auto chart1 = acmacs::chart::import_from_file(opt.chart1);
        auto chart2 = opt.chart2 ? acmacs::chart::import_from_file(opt.chart2) : chart1;
        acmacs::chart::CommonAntigensSera common(*chart1, *chart2, match_level);
        if (opt.subset == "antigens")
            common.antigens_only();
        else if (opt.subset == "sera")
            common.sera_only();
        else if (opt.subset != "all")
            fmt::print(stderr, "WARNING: unrecognized --subset argument, \"all\" assumed\n");
        if (common) {
            const auto procrustes_data = acmacs::chart::procrustes(*chart1->projection(opt.p1), *chart2->projection(opt.p2), common.points(),
                                                             opt.scaling ? acmacs::chart::procrustes_scaling_t::yes : acmacs::chart::procrustes_scaling_t::no);
            fmt::print("common antigens: {} sera: {}\n", common.common_antigens(), common.common_sera());
            fmt::print("transformation: {}\nrms: {}\n", procrustes_data.transformation, procrustes_data.rms);
            // common.report();
        }
        else {
            fmt::print(stderr, "ERROR no common antigens/sera\n");
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
