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
    std::string_view help_pre() const override{ return "Re-orients all projections to the master projection of the chart\n"; }

    option<size_t> master_projection_no{*this, 'm', desc{"master projection no"}};
    argument<str> chart{*this, arg_name{"chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::chart::ChartModify to_reorient{acmacs::chart::import_from_file(opt.chart)};
        auto master_projection = to_reorient.projection(opt.master_projection_no);
        acmacs::chart::CommonAntigensSera common(to_reorient);
        for (auto projection_no : acmacs::filled_with_indexes(to_reorient.number_of_projections())) {
            if (projection_no != *opt.master_projection_no) {
                const auto procrustes_data = acmacs::chart::procrustes(*master_projection, *to_reorient.projection(projection_no), common.points(), acmacs::chart::procrustes_scaling_t::no);
                to_reorient.projection_modify(projection_no)->transformation(procrustes_data.transformation);
                fmt::print("projection:  {}\ntransformation: {}\nrms: {}\n\n", projection_no, procrustes_data.transformation, procrustes_data.rms);
            }
        }
        acmacs::chart::export_factory(to_reorient, opt.output_chart, opt.program_name());
    }
    catch (std::exception& err) {
        fmt::print(stderr, "> ERROR {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
