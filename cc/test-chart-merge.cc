#include <iostream>
#include <cassert>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> ...\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            std::vector<acmacs::chart::ChartP> charts(args.number_of_arguments());
            std::transform(acmacs::index_iterator(0UL), acmacs::index_iterator(args.number_of_arguments()), charts.begin(),
                           [&args](size_t index) { return acmacs::chart::import_from_file(args[index], acmacs::chart::Verify::None, do_report_time(false)); });
            acmacs::chart::MergeSettings settings(acmacs::chart::CommonAntigensSera::match_level_t::strict);
            auto [result, merge_report] = acmacs::chart::merge(*charts[0], *charts[1], settings);
            for (size_t c_no = 2; c_no < charts.size(); ++c_no) {
                std::tie(result, merge_report) = acmacs::chart::merge(*result, *charts[c_no], settings);
            }

            acmacs::chart::PointIndexList antigens_visited, sera_visited;
            for (auto [layer_no, chart] : acmacs::enumerate(charts)) {
                const auto [antigens, sera] = result->titers()->antigens_sera_of_layer(layer_no);
                antigens_visited.extend(antigens);
                sera_visited.extend(sera);
                // std::cerr << "\nDEBUG: layer " << layer_no << '\n';
                for (auto ag_no : antigens) {
                    const auto full_name = result->antigens()->at(ag_no)->full_name();
                    // std::cerr << "DEBUG: AG " << ag_no << ' ' << full_name << '\n';
                    if (auto index = charts[layer_no]->antigens()->find_by_full_name(full_name); !index) {
                        std::cerr << "ERROR: antigen " << ag_no << " from layer " << layer_no << ": [" << full_name << "] not found in the original chart\n";
                        assert(false);
                    }
                }
                for (auto sr_no : sera) {
                    const auto full_name = result->sera()->at(sr_no)->full_name();
                    // std::cerr << "DEBUG: SR " << sr_no << ' ' << full_name << '\n';
                    if (auto index = charts[layer_no]->sera()->find_by_full_name(full_name); !index) {
                        std::cerr << "ERROR: serum " << sr_no << " from layer " << layer_no << ": [" << full_name << "] not found in the original chart\n";
                        assert(false);
                    }
                }
            }
            assert(antigens_visited.size() == result->number_of_antigens());
            assert(sera_visited.size() == result->number_of_sera());
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
