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
                {"-a", "", "comma separated list of antigen indexes (zero based), keep all if empty"},
                {"-s", "", "comma separated list of serum indexes (zero based), keep all if empty"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2 || (args["-a"].str().empty() && args["-s"].str().empty())) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};

            const std::string antigens_to_keep_s(args["-a"]);
            acmacs::chart::PointIndexList antigens_to_keep{antigens_to_keep_s.empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(antigens_to_keep_s, ",")};
            if (!antigens_to_keep->empty()) {
                acmacs::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
                antigens_to_remove.remove(*antigens_to_keep);
                std::cout << "INFO: antigens_to_remove: " << antigens_to_remove.size() << ' ' << antigens_to_remove << '\n';
                chart.remove_antigens(antigens_to_remove);
            }

            const std::string sera_to_keep_s(args["-s"]);
            acmacs::chart::PointIndexList sera_to_keep{sera_to_keep_s.empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(sera_to_keep_s, ",")};
            if (!sera_to_keep->empty()) {
                acmacs::ReverseSortedIndexes sera_to_remove(chart.number_of_sera());
                sera_to_remove.remove(*sera_to_keep);
                std::cout << "INFO: sera_to_remove: " << sera_to_remove.size() << ' ' << sera_to_remove << '\n';
                chart.remove_sera(sera_to_remove);
            }
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
