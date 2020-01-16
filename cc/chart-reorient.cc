#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  match{*this, "match", dflt{"auto"}, desc{"match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""}};
    option<int>  master_projection_no{*this, 'm', "master-projection", dflt{0}, desc{"master projection no"}};
    option<int>  chart_projection_no{*this, 'p', "chart-projection", dflt{0}, desc{"chart projection no, -1 to reorient all"}};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str> master_chart{*this, arg_name{"master-chart"}, mandatory};
    argument<str> source{*this, arg_name{"chart-to-reorient"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);

        const auto match_level = acmacs::chart::CommonAntigensSera::match_level(opt.match);
        auto master = acmacs::chart::import_from_file(opt.master_chart, acmacs::chart::Verify::None, report);
        acmacs::chart::ChartModify to_reorient{acmacs::chart::import_from_file(opt.source, acmacs::chart::Verify::None, report)};
        acmacs::chart::CommonAntigensSera common(*master, to_reorient, match_level);
        if (common) {
            fmt::print("common antigens: {} sera: {}\n", common.common_antigens(), common.common_sera());
            const auto projections = *opt.chart_projection_no < 0 ? acmacs::filled_with_indexes(to_reorient.number_of_projections()) : std::vector<size_t>{static_cast<size_t>(*opt.chart_projection_no)};
            for (auto projection_no : projections) {
                auto procrustes_data = acmacs::chart::procrustes(*master->projection(static_cast<size_t>(*opt.master_projection_no)), *to_reorient.projection(projection_no), common.points(), acmacs::chart::procrustes_scaling_t::no);
                to_reorient.projection_modify(projection_no)->transformation(procrustes_data.transformation);
                fmt::print("projection: {}\ntransformation: {}\nrms: {}\n", projection_no, procrustes_data.transformation, procrustes_data.rms);
            }
            acmacs::chart::export_factory(to_reorient, opt.output, opt.program_name(), report);
        }
        else {
            fmt::print(stderr, "ERROR: no common antigens/sera\n");
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR:  {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
