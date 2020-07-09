#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<int>    projection{*this, "projection", dflt{0}, desc{"-1 to relax all"}};
    option<bool>   rough{*this, "rough"};
    option<bool>   sort{*this, "sort", desc{"sort projections"}};
    option<str>    method{*this, "method", dflt{"cg"}, desc{"method: lbfgs, cg"}};
    option<double> max_distance_multiplier{*this, "md", dflt{1.0}, desc{"max distance multiplier"}};
    option<bool>   report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str>  source_chart{*this, arg_name{"source-chart"}, mandatory};
    argument<str>  output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);

        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source_chart, acmacs::chart::Verify::None, report)};
        if (chart.number_of_projections() == 0)
            throw std::runtime_error("chart has no projections");
        const auto precision = opt.rough ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
        const acmacs::chart::optimization_method method{acmacs::chart::optimization_method_from_string(opt.method)};
        auto relax = [&chart, method, precision](size_t proj_no) {
            auto projection = chart.projection_modify(proj_no);
            const auto status = projection->relax(acmacs::chart::optimization_options(method, precision));
            fmt::print("{}\n", status);
        };
        if (opt.projection >= 0) {
            if (static_cast<size_t>(opt.projection) >= chart.number_of_projections())
                throw std::runtime_error("invalid projection number");
            relax(static_cast<size_t>(opt.projection));
        }
        else {
            for (size_t p_no = 0; p_no < chart.number_of_projections(); ++p_no)
                relax(p_no);
        }

        if (opt.sort)
            chart.projections_modify()->sort();
        std::cout << chart.make_info() << '\n';
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name(), report);
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
