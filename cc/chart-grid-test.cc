#include <algorithm>

#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/string-split.hh"
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
    option<double> step{*this, "step", dflt{0.1}, desc{"grid step"}};
    option<str>    points_to_test{*this, "points", dflt{"all"}, desc{"comma separated list of point numbers or names to test, \"all\" to test all"}};
    option<str>    json{*this, "json", desc{"export test results into json"}};
    option<str>    csv{*this, "csv", desc{"export layout and test results into csv"}};
    option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for test (omp): 0 - autodetect, 1 - sequential"}};
    option<bool>   report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool>   verbose{*this, "verbose"};

    argument<str> source{*this, arg_name{"chart-to-test"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source, acmacs::chart::Verify::None, report)};

        acmacs::chart::GridTest::Results results;
        if (opt.points_to_test == "all") {
            std::vector<size_t> projections_to_reorient;
            size_t projection_no_to_test = opt.projection;
            for (auto attempt = 1; attempt < 10; ++attempt) {
                acmacs::chart::GridTest test(chart, projection_no_to_test, opt.step);
                results = test.test_all(opt.threads);
                fmt::print("{}\n", results.report());
                if (opt.verbose || !opt.relax) {
                    for (const auto& entry : results) {
                        if (entry)
                            fmt::print("{}\n", entry.report(chart));
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
                acmacs::chart::export_factory(chart, opt.output, opt.program_name(), report);
            }
            fmt::print(stderr, "{}\n", chart.make_info());
        }
        else {
            acmacs::chart::GridTest test(chart, opt.projection, opt.step);
            auto antigens = chart.antigens();
            acmacs::chart::Indexes points;
            for (const auto& point_ref : acmacs::string::split(*opt.points_to_test, ",")) {
                if (const auto point_no = string::from_chars<size_t>(point_ref); point_no != std::numeric_limits<size_t>::max()) {
                    points.insert(point_no);
                }
                else {
                    if (auto found = antigens->find_by_full_name(point_ref); found)
                        points.insert(*found);
                    else
                        points = antigens->find_by_name(point_ref);
                    if (points->empty())
                        throw std::runtime_error(fmt::format("No points selected by {}", point_ref));
                }
            }
            results = test.test(*points);
            fmt::print("{}\n", results.report());
        }
        if (opt.json)
            acmacs::file::write(opt.json, results.export_to_json(chart));
        if (opt.csv)
            acmacs::file::write(opt.csv, results.export_to_layout_csv(chart, *chart.projection(opt.projection)));
        if (!opt.json && !opt.csv) {
            for (const auto& res : results) {
                if (res.diagnosis == acmacs::chart::GridTest::Result::trapped || res.diagnosis == acmacs::chart::GridTest::Result::hemisphering)
                    fmt::print("{}\n", res.report(chart));
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
