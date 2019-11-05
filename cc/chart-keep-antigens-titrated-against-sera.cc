#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"-s", "", "comma separated list of serum indexes (zero based)"},
                           {"--time", false, "report time of loading chart"},
                           {"--dry-run", false, "report antigens, do not produce output"},
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2 || args["-s"].str().empty()) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const std::string sera(args["-s"]);
            acmacs::chart::PointIndexList sera_titrated_against{sera.empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(sera, ",")};
            // acmacs::ReverseSortedIndexes antigens_to_remove;
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            auto titers = chart.titers();
            acmacs::ReverseSortedIndexes antigens_to_keep;
            for (auto serum_index : sera_titrated_against) {
                antigens_to_keep.add(*titers->having_titers_with(serum_index + chart.number_of_antigens()));
            }
            std::cout << "INFO: antigens_to_keep: " << antigens_to_keep.size() << ' ' << antigens_to_keep << '\n';
            acmacs::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
            antigens_to_remove.remove(antigens_to_keep);
            std::cout << "INFO: antigens_to_remove: " << antigens_to_remove.size() << ' ' << antigens_to_remove << '\n';
            if (!args["--dry-run"]) {
                if (!antigens_to_remove.empty())
                    chart.remove_antigens(antigens_to_remove);
                acmacs::chart::export_factory(chart, args[1], args.program(), report);
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
