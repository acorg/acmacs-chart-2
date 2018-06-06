#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

static acmacs::chart::PointIndexList get_disconnected(std::string antigens, std::string sera, size_t number_of_antigens, size_t number_of_sera);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-n", 1U, "number of optimizations"},
                {"-d", 2U, "number of dimensions"},
                {"-m", "none", "minimum column basis"},
                {"--rough", false},
                {"--no-dimension-annealing", false},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 2.0, "randomization diameter multiplier"},
                {"--keep-projections", 0, "number of projections to keep, 0 - keep all"},
                {"--no-disconnect-having-few-titers", false, "do not disconnect points having too few numeric titers"},
                {"--disconnect-antigens", "", "comma separated list of antigen/point indexes (0-based) to disconnect for the new projections"},
                {"--disconnect-sera", "", "comma separated list of serum indexes (0-based) to disconnect for the new projections"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> [<output-chart-file>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            const auto method{acmacs::chart::optimization_method_from_string(args["--method"])};
            auto disconnected{get_disconnected(args["--disconnect-antigens"], args["--disconnect-sera"], chart.number_of_antigens(), chart.number_of_sera())};
            if (!args["--no-disconnect-having-few-titers"])
                disconnected.extend(chart.titers()->having_too_few_numeric_titers());

            const size_t number_of_attempts = args["-n"];
            chart.relax(number_of_attempts, args["-m"].str(), args["-d"], !args["--no-dimension-annealing"], acmacs::chart::optimization_options(method, precision, args["--md"]), args["--verbose"] || args["-v"], disconnected);
            // for (size_t attempt = 0; attempt < number_of_attempts; ++attempt) {
            //     auto [status, projection] = chart.relax(args["-m"].str(), args["-d"], !args["--no-dimension-annealing"], acmacs::chart::optimization_options(method, precision, args["--md"]), disconnected);
            //     std::cout << (attempt + 1) << ' ' << status << '\n';
            // }
            auto projections = chart.projections_modify();
            projections->sort();
            if (const size_t keep_projections = args["--keep-projections"]; keep_projections > 0 && projections->size() > keep_projections)
                projections->keep_just(keep_projections);
            std::cout << chart.make_info() << '\n';
            if (args.number_of_arguments() > 1)
                acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename(), report);
        }
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

acmacs::chart::PointIndexList get_disconnected(std::string antigens, std::string sera, size_t number_of_antigens, size_t number_of_sera)
{
    acmacs::chart::PointIndexList points;
    if (!antigens.empty())
        extend(points, acmacs::string::split_into_uint(antigens, ","), 0, number_of_antigens + number_of_sera);
    if (!sera.empty())
        extend(points, acmacs::string::split_into_uint(sera, ","), number_of_antigens, number_of_sera);
    return points;

} // get_disconnected

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
