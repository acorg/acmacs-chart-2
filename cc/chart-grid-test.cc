#include <iostream>
#include <algorithm>

#include "acmacs-base/argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/grid-test.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/procrustes.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool>   relax{*this, "relax", desc{"move trapped points and relax, test again, repeat while there are trapped points"}};
    option<size_t> projection{*this, "projection", dflt{0UL}, desc{"projection number to test"}};
    option<double> step{*this, "stem", dflt{0.1}, desc{"grid step"}};
    option<str>    point_to_test{*this, "point", dflt{"all"}, desc{"point number of name to test, \"all\" to test all"}};
    option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for test (omp): 0 - autodetect, 1 - sequential"}};
    option<bool>   report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool>   verbose{*this, "verbose"};

    argument<str> source{*this, arg_name{"chart-to-text"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source, acmacs::chart::Verify::None, report)};

        // std::cout << test.initial_report() << '\n';
        if (opt.point_to_test == "all") {
            std::vector<size_t> projections_to_reorient;
            size_t projection_no_to_test = opt.projection;
            for (auto attempt = 1; attempt < 10; ++attempt) {
                acmacs::chart::GridTest test(chart, projection_no_to_test, opt.step);
                const auto results = test.test_all_parallel(opt.threads);
                std::cout << results.report() << '\n';
                if (opt.verbose || !opt.relax) {
                    for (const auto& entry : results) {
                        if (entry)
                            std::cout << entry.report(chart) << '\n';
                    }
                }
                if (!opt.relax)
                    break;
                auto projection = test.make_new_projection_and_relax(results, true);
                projection->comment("grid-test-" + acmacs::to_string(attempt));
                projection_no_to_test = projection->projection_no();
                projections_to_reorient.push_back(projection_no_to_test);
                if (std::all_of(results.begin(), results.end(), [](const auto& result) { return result.diagnosis != acmacs::chart::GridTest::Result::trapped; }))
                    break;
            }
            if (opt.output.has_value()) {
                if (!projections_to_reorient.empty()) {
                    acmacs::chart::CommonAntigensSera common(chart);
                    auto master_projection = chart.projection(opt.projection);
                    for (auto projection_to_reorient : projections_to_reorient) {
                        const auto procrustes_data = acmacs::chart::procrustes(*master_projection, *chart.projection(projection_to_reorient), common.points(), acmacs::chart::procrustes_scaling_t::no);
                        chart.projection_modify(projection_to_reorient)->transformation(procrustes_data.transformation);
                    }
                }
                chart.projections_modify()->sort();
                acmacs::chart::export_factory(chart, opt.output, fs::path(opt.program_name()).filename(), report);
            }
            std::cerr << chart.make_info() << '\n';
        }
        else {
            acmacs::chart::GridTest test(chart, opt.projection, opt.step);
            auto antigens = chart.antigens();
            acmacs::chart::Indexes points;
            try {
                points.insert(std::stoul(opt.point_to_test));
            }
            catch (std::invalid_argument&) {
                if (auto found = antigens->find_by_full_name(opt.point_to_test); found)
                    points.insert(*found);
                else
                    points = antigens->find_by_name(opt.point_to_test);
                if (points.empty())
                    throw std::runtime_error(string::concat("No points selected by ", *opt.point_to_test));
            }
            for (auto point : points) {
                if (const auto result = test.test_point(point); result)
                    std::cout << result.report(chart) << '\n';
                else if (opt.verbose)
                    std::cout << result.report(chart) << '\n';
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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
