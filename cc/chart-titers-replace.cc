#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/text-export.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "replaces all titers matching regex (std::regex_search) with replacement ($` etc. substs supported)"; }
    option<bool> verbose{*this, 'v', "verbose"};
    argument<str> chart{*this, arg_name{"chart"}, mandatory};
    argument<str> look_for{*this, arg_name{"look_for"}, mandatory};
    argument<str> replacement{*this, arg_name{"replacement"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.chart)};
        chart.titers_modify().replace_all(std::regex{std::begin(*opt.look_for), std::end(*opt.look_for), acmacs::regex::icase}, opt.replacement);
        acmacs::chart::export_factory(chart, opt.chart, opt.program_name());
        if (opt.verbose)
            fmt::print("{}", acmacs::chart::export_table_to_text(chart));
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
