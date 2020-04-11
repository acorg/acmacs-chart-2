#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  output_chart{*this, 'o', "output", dflt{""}, desc{"output chart"}};
    option<str>  match{*this, "match", dflt{"auto"}, desc{"match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""}};
    option<str>  merge_type{*this, 'm', "merge-type", dflt{"simple"}, desc{"merge type: \"type1\"..\"type5\", \"incremental\" (type2), \"overlay\" (type3), \"simple\" (type1)"}};
    option<bool> duplicates_distinct{*this, "duplicates-distinct", desc{"make duplicates distinct"}};
    option<str>  report_titers{*this, "report", desc{"titer merge report"}};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str_array> source_charts{*this, arg_name{"source-chart"}, mandatory};
};

int main(int argc, const char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        acmacs::chart::MergeSettings settings;
        if (opt.merge_type == "incremental" || opt.merge_type == "type2")
            settings.projection_merge = acmacs::chart::projection_merge_t::type2;
        else if (opt.merge_type == "overlay" || opt.merge_type == "type3")
            settings.projection_merge = acmacs::chart::projection_merge_t::type3;
        else if (opt.merge_type == "type4")
            settings.projection_merge = acmacs::chart::projection_merge_t::type4;
        else if (opt.merge_type == "type5")
            settings.projection_merge = acmacs::chart::projection_merge_t::type5;
        else if (opt.merge_type != "simple" && opt.merge_type != "type1")
            throw std::runtime_error(string::concat("unrecognized --merge-type value: ", opt.merge_type.get()));
        if (opt.source_charts->size() < 2)
            throw std::runtime_error("too few source charts specified");
        settings.match_level = acmacs::chart::CommonAntigensSera::match_level(opt.match);
        const auto read = [report,duplicates_distinct=*opt.duplicates_distinct](std::string_view filename) {
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report)};
            if (duplicates_distinct) {
                chart.antigens_modify()->duplicates_distinct(chart.antigens()->find_duplicates());
                chart.sera_modify()->duplicates_distinct(chart.sera()->find_duplicates());
            }
            return chart;
        };
        auto chart1 = read((*opt.source_charts)[0]);
        auto chart2 = read((*opt.source_charts)[1]);
        auto [result, merge_report] = acmacs::chart::merge(chart1, chart2, settings);
        std::cout << chart1.description() << '\n' << chart2.description() << "\n\n";
        merge_report.common.report();
        std::cout << "----------\n\n";
        for (size_t c_no = 2; c_no < opt.source_charts->size(); ++c_no) {
            auto chart3 = read((*opt.source_charts)[c_no]);
            std::cout << result->description() << '\n' << chart3.description() << "\n\n";
            std::tie(result, merge_report) = acmacs::chart::merge(*result, chart3, settings);
            merge_report.common.report();
            std::cout << "----------\n\n";
        }
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(*result, opt.output_chart, opt.program_name(), report);
        if (opt.report_titers.has_value()) {
            merge_report.titer_merge_report(opt.report_titers, *result, opt.program_name());
        }
        else {
            std::cout << result->make_info() << '\n';
            if (const auto having_too_few_numeric_titers = result->titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers->empty())
                std::cout << fmt::format("Points having too few numeric titers: {} {}\n", having_too_few_numeric_titers->size(), having_too_few_numeric_titers);
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
