#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-a", "", "comma separated list of antigen indexes to remove (0-based)"},
                {"-s", "", "comma separated list of serum indexes to remove (zero based)"},
                {"--remove-egg", false, "remove egg antigens and sera"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const std::string antigens = args["-a"], sera = args["-s"];
            acmacs::ReverseSortedIndexes antigens_to_remove{antigens.empty() ? acmacs::Indexes{} : acmacs::string::split_into_uint(antigens, ",")};
            acmacs::ReverseSortedIndexes sera_to_remove{sera.empty() ? acmacs::Indexes{} : acmacs::string::split_into_uint(sera, ",")};
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            if (args["--remove-egg"]) {
                auto ag_egg = chart.antigens()->all_indexes();
                chart.antigens()->filter_egg(ag_egg);
                antigens_to_remove.add(ag_egg.data());
                auto sr_egg = chart.sera()->all_indexes();
                chart.sera()->filter_egg(sr_egg);
                sera_to_remove.add(sr_egg.data());
            }
            if (!antigens_to_remove.empty())
                chart.remove_antigens(antigens_to_remove);
            if (!sera_to_remove.empty())
                chart.remove_sera(sera_to_remove);
            acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename(), report);
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
