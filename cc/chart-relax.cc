#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

static acmacs::chart::PointIndexList get_disconnected(std::string_view antigens, std::string_view sera, size_t number_of_antigens, size_t number_of_sera);

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> number_of_optimizations{*this, 'n', dflt{1UL}, desc{"number of optimizations"}};
    option<size_t> number_of_dimensions{*this, 'd', dflt{2UL}, desc{"number of dimensions"}};
    option<str>    minimum_column_basis{*this, 'm', dflt{"none"}, desc{"minimum column basis"}};
    option<bool>   rough{*this, "rough"};
    option<size_t> fine{*this, "fine", dflt{0UL}, desc{"relax roughly, then relax finely N best projections"}};
    option<bool>   no_dimension_annealing{*this, "no-dimension-annealing"};
    option<str>    method{*this, "method", dflt{"cg"}, desc{"method: lbfgs, cg"}};
    option<double> max_distance_multiplier{*this, "md", dflt{2.0}, desc{"randomization diameter multiplier"}};
    option<size_t> keep_projections{*this, "keep-projections", dflt{0UL}, desc{"number of projections to keep, 0 - keep all"}};
    option<bool>   no_disconnect_having_few_titers{*this, "no-disconnect-having-few-titers"};
    option<str>    disconnect_antigens{*this, "disconnect-antigens", dflt{""}, desc{"comma separated list of antigen/point indexes (0-based) to disconnect for the new projections"}};
    option<str>    disconnect_sera{*this, "disconnect-sera", dflt{""}, desc{"comma separated list of serum indexes (0-based) to disconnect for the new projections"}};
    option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for optimization (omp): 0 - autodetect, 1 - sequential"}};
    option<bool>   report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool>   verbose{*this, 'v', "verbose"};

    argument<str>  source_chart{*this, arg_name{"source-chart"}, mandatory};
    argument<str>  output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);

        const Timeit ti("performing " + std::to_string(opt.number_of_optimizations) + " optimizations: ", report);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source_chart, acmacs::chart::Verify::None, report)};
        const auto precision = (opt.rough || opt.fine > 0) ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
        const auto method{acmacs::chart::optimization_method_from_string(opt.method)};
        auto disconnected{get_disconnected(opt.disconnect_antigens, opt.disconnect_sera, chart.number_of_antigens(), chart.number_of_sera())};
        if (!opt.no_disconnect_having_few_titers)
            disconnected.extend(chart.titers()->having_too_few_numeric_titers());
        if (!disconnected->empty())
            std::cerr << "INFO: " << disconnected->size() << " points disconnected: " << disconnected << '\n';

        acmacs::chart::optimization_options options(method, precision, opt.max_distance_multiplier);
        options.num_threads = opt.threads;
        const auto dimension_annealing = acmacs::chart::use_dimension_annealing_from_bool(!opt.no_dimension_annealing);
        chart.relax(acmacs::chart::number_of_optimizations_t{*opt.number_of_optimizations}, *opt.minimum_column_basis, acmacs::number_of_dimensions_t{opt.number_of_dimensions}, dimension_annealing, options, opt.verbose ? acmacs::chart::report_stresses::yes : acmacs::chart::report_stresses::no, disconnected);
        auto projections = chart.projections_modify();
        projections->sort();
        for (size_t p_no = 0; p_no < opt.fine; ++p_no)
            chart.projection_modify(p_no)->relax(acmacs::chart::optimization_options(method, acmacs::chart::optimization_precision::fine));
        if (const size_t keep_projections = opt.keep_projections; keep_projections > 0 && projections->size() > keep_projections)
            projections->keep_just(keep_projections);
        std::cout << chart.make_info() << '\n';
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(chart, opt.output_chart, fs::path(opt.program_name()).filename(), report);
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

inline void extend(acmacs::chart::PointIndexList& target, std::vector<size_t>&& source, size_t aIncrementEach, size_t aMax)
{
    for (const auto no : source) {
        if (no >= aMax)
            throw std::runtime_error("invalid index " + acmacs::to_string(no) + ", expected in range 0.." + acmacs::to_string(aMax - 1) + " inclusive");
        target.insert(no + aIncrementEach);
    }
}

acmacs::chart::PointIndexList get_disconnected(std::string_view antigens, std::string_view sera, size_t number_of_antigens, size_t number_of_sera)
{
    acmacs::chart::PointIndexList points;
    if (!antigens.empty())
        extend(points, acmacs::string::split_into_size_t(antigens, ","), 0, number_of_antigens + number_of_sera);
    if (!sera.empty())
        extend(points, acmacs::string::split_into_size_t(sera, ","), number_of_antigens, number_of_sera);
    return points;

} // get_disconnected

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
