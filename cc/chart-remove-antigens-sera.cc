#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  antigens_to_remove{*this, 'a', "--antigens", desc{"comma or space separated list of antigen indexes to remove (0-based)"}};
    option<str>  sera_to_remove{*this, 's', "--sera", desc{"comma or space separated list of serum indexes to remove (0-based)"}};
    option<bool> remove_egg{*this, "remove-egg", desc{"remove egg antigens and sera"}};
    option<bool> remove_projections{*this, "remove-projections"};

    argument<str> source{*this, arg_name{"source-chart"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::ReverseSortedIndexes antigens_to_remove{opt.antigens_to_remove->empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(*opt.antigens_to_remove)};
        acmacs::ReverseSortedIndexes sera_to_remove{opt.sera_to_remove->empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(*opt.sera_to_remove)};
        // fmt::print(stderr, "DEBUG: sera_to_remove: \"{}\" -> {}\n", opt.sera_to_remove, sera_to_remove);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(*opt.source, acmacs::chart::Verify::None)};
        if (opt.remove_egg) {
            auto ag_egg = chart.antigens()->all_indexes();
            chart.antigens()->filter_egg(ag_egg);
            antigens_to_remove.add(*ag_egg);
            auto sr_egg = chart.sera()->all_indexes();
            chart.sera()->filter_egg(sr_egg);
            sera_to_remove.add(*sr_egg);
        }
        if (opt.remove_projections)
            chart.projections_modify()->remove_all();
        if (!antigens_to_remove.empty())
            chart.remove_antigens(antigens_to_remove);
        if (!sera_to_remove.empty())
            chart.remove_sera(sera_to_remove);
        acmacs::chart::export_factory(chart, *opt.output, opt.program_name());
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
