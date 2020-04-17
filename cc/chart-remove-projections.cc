#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t> keep{*this, 'k', "keep", dflt{0ul}, desc{"number of the (first) projections to keep"}};
    argument<str> input{*this, arg_name{"input-chart-file"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::chart::ChartModify chart(acmacs::chart::import_from_file(opt.input));
        auto projections = chart.projections_modify();
        if (opt.keep == 0ul)
            projections->remove_all();
        else if (projections->size() > 0)
            projections->remove_except(opt.keep, projections->at(0));
        acmacs::chart::export_factory(chart, opt.output, opt.program_name());
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
