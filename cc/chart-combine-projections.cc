#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// Makes sure table is the same, combines projections, sorts them, keep few best projections

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> keep_projections{*this, 'k', "keep-projections", dflt{10ul}, desc{"number of projections to keep, 0 - keep all"}};
    option<str>    output_chart{*this, 'o', "output", desc{"output-chart"}};
    option<bool>   info{*this, 'i', "info"};

    argument<str_array>  source_charts{*this, arg_name{"source-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        acmacs::chart::ChartModify master{acmacs::chart::import_from_file(opt.source_charts->at(0))};
        for (const auto& chart_filename : ranges::views::drop(*opt.source_charts, 1)) {
            acmacs::chart::ChartModify merge_in{acmacs::chart::import_from_file(chart_filename)};
            try {
                master.combine_projections(merge_in);
            }
            catch (std::exception& err) {
                throw std::runtime_error{AD_FORMAT("cannot combine projections of {} and {}: {}", opt.source_charts->at(0), chart_filename, err)};
            }
        }
        master.projections_modify().sort();
        if (opt.info)
            fmt::print("{}\n", master.make_info());
        master.projections_modify().keep_just(*opt.keep_projections);
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(master, opt.output_chart, opt.program_name());
        fmt::print("{}\n", master.make_name());
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
