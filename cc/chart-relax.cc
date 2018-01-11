#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

static void test_rough(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, size_t num_dims, double max_distance_multiplier);
static void test_randomization(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, size_t num_dims, double max_distance_multiplier);
static void optimize_n(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, size_t num_dims, double max_distance_multiplier, bool rough);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-n", 1U, "number of optimizations"},
                {"-d", 2U, "number of dimensions"},
                {"-m", "none", "minimum column basis"},
                {"--md", 1.0, "max distance multiplier"},
                {"--rough", false},
                {"--rough-test", false},
                {"--randomization-test", false},
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
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            if (args["--rough-test"]) {
                test_rough(chart, args["-n"], args["-m"].str(), args["-d"], args["--md"]);
            }
            else if (args["--randomization-test"]) {
                test_randomization(chart, args["-n"], args["-m"].str(), args["-d"], args["--md"]);
            }
            else {
                optimize_n(chart, args["-n"], args["-m"].str(), args["-d"], args["--md"], args["--rough"]);
                chart.projections_modify()->sort();
                std::cout << chart.make_info() << '\n';
                if (args.number_of_arguments() > 1)
                    acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename(), report);
            }
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void test_randomization(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, size_t num_dims, double max_distance_multiplier)
{
    const size_t randomizations = 100;
    auto projection = chart.projections_modify()->new_from_scratch(num_dims, min_col_basis);
    for (size_t no = 0; no < attempts; ++no) {
        Timeit ti("randomize and relax: ");
        projection->randomize_layout(max_distance_multiplier);
        double best_stress = projection->stress();
        acmacs::Layout best_layout{*projection->layout()};
        for (size_t rnd_no = 0; rnd_no < randomizations; ++rnd_no) {
            projection->randomize_layout(max_distance_multiplier);
            const double stress = projection->stress();
            // std::cout << "  s: " << stress << '\n';
            if (stress < best_stress) {
                best_stress = stress;
                best_layout = *projection->layout();
            }
        }
        const auto status = projection->relax(acmacs::chart::OptimizationMethod::alglib_lbfgs, false);
        std::cout << "final " << std::setprecision(12) << status.final_stress << " time: " << acmacs::format(status.time) << " iters: " << status.number_of_iterations << " nstress: " << status.number_of_stress_calculations << ' ' << status.termination_report << '\n';
    }

} // test_randomization

// ----------------------------------------------------------------------

void test_rough(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, size_t num_dims, double max_distance_multiplier)
{
    auto projection = chart.projections_modify()->new_from_scratch(num_dims, min_col_basis);

    std::vector<std::tuple<size_t, double, double>> stresses;

    double speed_ratio_sum = 0;
    for (size_t no = 0; no < attempts; ++no) {
        projection->randomize_layout(max_distance_multiplier);
        const auto status = projection->relax(acmacs::chart::OptimizationMethod::alglib_lbfgs, true);
        std::cout << "rough " << std::setprecision(12) << status.final_stress << " time: " << acmacs::format(status.time) << " iters: " << status.number_of_iterations << " nstress: " << status.number_of_stress_calculations << ' ' << status.termination_report << '\n';
        const auto status2 = projection->relax(acmacs::chart::OptimizationMethod::alglib_lbfgs, false);
        std::cout << "fin   " << std::setprecision(12) << status2.final_stress << " time: " << acmacs::format(status2.time) << " iters: " << status2.number_of_iterations << " nstress: " << status2.number_of_stress_calculations << ' ' << status.termination_report << '\n';
        const double speed_ratio = static_cast<double>(status.time.count()) / (status.time + status2.time).count();
          // std::cout << "  rough speed ratio: " << speed_ratio << '\n';
        speed_ratio_sum += speed_ratio;
        stresses.emplace_back(no, status.final_stress, status2.final_stress);
    }
    std::cout << "average speed ratio: " << (speed_ratio_sum / attempts) << '\n';

    auto report = [&stresses](std::string header) {
                      std::cout << header << '\n';
                      for (const auto& entry: stresses)
                          std::cout << "  " << std::setw(3) << std::get<0>(entry) << ' ' << std::get<1>(entry) << ' ' << std::get<2>(entry) << ' ' << (std::get<1>(entry) - std::get<2>(entry)) << '\n';
                  };

    std::sort(stresses.begin(), stresses.end(), [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });
    std::vector<size_t> order_rough(stresses.size());
    std::transform(stresses.begin(), stresses.end(), order_rough.begin(), [](const auto& entry) { return std::get<0>(entry); });
    report("by rough");
    std::sort(stresses.begin(), stresses.end(), [](const auto& a, const auto& b) { return std::get<2>(a) < std::get<2>(b); });
    std::vector<size_t> order_final(stresses.size());
    std::transform(stresses.begin(), stresses.end(), order_final.begin(), [](const auto& entry) { return std::get<0>(entry); });
    report("by final");
    std::cout << "order_rough: " << order_rough << '\n';
    if (order_rough == order_final)
        std::cout << "orders are the same" << '\n';
    else
        std::cout << "orders are DIFFERENT\norder_final: " << order_final << '\n';

      // acmacs::Layout starting{*projection->layout()};
      // projection->set_layout(starting);
      // const auto status3 = projection->relax(acmacs::chart::OptimizationMethod::alglib_lbfgs, false);
      // std::cout << "final " << std::setprecision(12) << status3.final_stress << " time: " << acmacs::format(status3.time) << " iters: " << status3.number_of_iterations << " nstress: " << status3.number_of_stress_calculations << '\n';
      // std::cout << "stress diff: " << (status.final_stress - status3.final_stress) << '\n';

} // test_rough

// ----------------------------------------------------------------------

void optimize_n(acmacs::chart::ChartModify& chart, size_t attempts, std::string min_col_basis, size_t num_dims, double max_distance_multiplier, bool rough)
{
    for (size_t no = 0; no < attempts; ++no) {
          // Timeit ti("randomize and relax: ");
        auto projection = chart.projections_modify()->new_from_scratch(num_dims, min_col_basis);
        projection->randomize_layout(max_distance_multiplier);
        const auto status = projection->relax(acmacs::chart::OptimizationMethod::alglib_lbfgs, rough);
        if (attempts == 1)
            std::cout << status << '\n';
        else
            std::cout << std::setprecision(12) << status.final_stress << " time: " << acmacs::format(status.time) << " iters: " << status.number_of_iterations << " nstress: " << status.number_of_stress_calculations << '\n';
    }

} // optimize_n

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
