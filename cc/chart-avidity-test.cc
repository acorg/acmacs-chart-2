#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
// #include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/avidity-test.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/optimize.hh"
#include "acmacs-chart-2/log.hh"
#include "acmacs-chart-2/command-helper.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<double> adjust_step{*this, "adjust-step", dflt{1.0}};
    option<double> min_adjust{*this, "min-adjust", dflt{-6.0}};
    option<double> max_adjust{*this, "max-adjust", dflt{6.0}};
    option<size_t> projection{*this, "projection", dflt{0ul}};
    option<bool>   rough{*this, "rough"};
    option<str>    method{*this, "method", dflt{"alglib-cg"}, desc{"method: alglib-lbfgs, alglib-cg, optim-bfgs, optim-differential-evolution"}};

    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};

    argument<str>  source_chart{*this, arg_name{"source-chart"}, mandatory};
    // argument<str>  output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {

        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);
        // acmacs::log::enable(acmacs::log::relax);

        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source_chart, acmacs::chart::Verify::None)};
        if (chart.number_of_projections() == 0)
            throw std::runtime_error("chart has no projections");
        if (static_cast<size_t>(opt.projection) >= chart.number_of_projections())
            throw std::runtime_error("invalid projection number");
        const acmacs::chart::optimization_options opt_opt(acmacs::chart::optimization_method_from_string(opt.method),
                                                          opt.rough ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine);

        using namespace acmacs::chart;
        using namespace acmacs::chart::avidity;
        const auto results = test(chart, opt.projection, Settings{.step = opt.adjust_step, .min_adjust = opt.min_adjust, .max_adjust = opt.max_adjust},
             optimization_options{optimization_method_from_string(opt.method), opt.rough ? optimization_precision::rough : optimization_precision::fine});
        AD_PRINT("{}", results);
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
