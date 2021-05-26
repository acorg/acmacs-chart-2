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

    option<size_t> number_of_attempts{*this, 'n', desc{"number of optimizations"}};
    option<bool>   no_disconnect_having_few_titers{*this, "no-disconnect-having-few-titers", desc{"do not disconnect points having too few numeric titers"}};
    option<bool>   unmovable_non_nan_points{*this, "unmovable-non-nan-points", desc{"keep ag/sr of primary chart frozen (unmovable)"}};
    option<bool>   remove_source_projection{*this, "remove-source-projection"};
    option<bool>   rough{*this, "rough"};
    option<bool>   grid{*this, "grid-test"};
    option<str>    grid_json{*this, "grid-json", desc{"export grid test results into json"}};
    option<double> grid_step{*this, "grid-step", dflt{0.1}};
    option<str>    method{*this, "method", dflt{"alglib-cg"}, desc{"method: alglib-lbfgs, alglib-cg, optim-bfgs, optim-differential-evolution"}};
    option<double> randomization_diameter_multiplier{*this, "md", dflt{2.0}, desc{"randomization diameter multiplier"}};
    option<size_t> keep_projections{*this, "keep-projections", dflt{0ul}, desc{"number of projections to keep, 0 - keep all"}};
    option<int>    threads{*this, "threads", dflt{0}};
    option<bool>   report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str> chart{*this, arg_name{"chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        const Timeit ti(fmt::format("performing {} optimizations: ", opt.number_of_attempts), report);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.chart, acmacs::chart::Verify::None, report)};
        const auto precision = opt.rough ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
        const auto method{acmacs::chart::optimization_method_from_string(opt.method)};

        const size_t source_projection_no = 0;
        acmacs::chart::optimization_options options(method, precision, opt.randomization_diameter_multiplier);
        options.num_threads = opt.threads;
        chart.relax_incremental(source_projection_no, acmacs::chart::number_of_optimizations_t{*opt.number_of_attempts}, options,
                                opt.remove_source_projection ? acmacs::chart::remove_source_projection::yes : acmacs::chart::remove_source_projection::no,
                                opt.unmovable_non_nan_points ? acmacs::chart::unmovable_non_nan_points::yes : acmacs::chart::unmovable_non_nan_points::no);
        auto& projections = chart.projections_modify();
        if (opt.keep_projections > 0 && projections.size() > opt.keep_projections)
            projections.keep_just(opt.keep_projections);
        fmt::print("{}\n", chart.make_info());
        if (!opt.output_chart.empty())
            acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name(), report);
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
