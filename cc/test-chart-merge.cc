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
            const acmacs::chart::MergeSettings settings(acmacs::chart::CommonAntigensSera::match_level_t::strict);
            auto [result, merge_report] = acmacs::chart::merge(*charts[0], *charts[1], settings);
            for (size_t c_no = 2; c_no < charts.size(); ++c_no) {
                std::tie(result, merge_report) = acmacs::chart::merge(*result, *charts[c_no], settings);
            }

            // const auto num_digits = static_cast<int>(std::log10(std::max(result->antigens()->size(), result->sera()->size()))) + 1;
            // for (auto [ag_no, antigen]: acmacs::enumerate(*result->antigens()))
            //     std::cout << "AG " << std::setw(num_digits) << ag_no << ' ' << antigen->full_name() << '\n';
            // for (auto [sr_no, serum]: acmacs::enumerate(*result->sera()))
            //     std::cout << "SR " << std::setw(num_digits) << sr_no << ' ' << serum->full_name() << '\n';

            acmacs::chart::PointIndexList antigens_visited, sera_visited;
            for (auto [layer_no, chart] : acmacs::enumerate(charts)) {
                const auto [antigens, sera] = result->titers()->antigens_sera_of_layer(layer_no);
                antigens_visited.extend(antigens);
                sera_visited.extend(sera);

                std::map<size_t, size_t> antigen_map, serum_map;
                  // ---- test antigens -----
                // std::cerr << "\nDEBUG: layer " << layer_no << '\n';
                for (auto ag_no : antigens) {
                    const auto full_name = result->antigens()->at(ag_no)->full_name();
                    // std::cerr << "DEBUG: AG " << ag_no << ' ' << full_name << '\n';
                    if (auto index = charts[layer_no]->antigens()->find_by_full_name(full_name); !index) {
                        std::cerr << "ERROR: antigen " << ag_no << " from layer " << layer_no << ": [" << full_name << "] not found in the original chart\n";
                        assert(false);
                    }
                    else
                        antigen_map[ag_no] = *index;
                }
                  // ---- test sera -----
                for (auto sr_no : sera) {
                    const auto full_name = result->sera()->at(sr_no)->full_name();
                    // std::cerr << "DEBUG: SR " << sr_no << ' ' << full_name << '\n';
                    if (auto index = charts[layer_no]->sera()->find_by_full_name(full_name); !index) {
                        std::cerr << "ERROR: serum " << sr_no << " from layer " << layer_no << ": [" << full_name << "] not found in the original chart\n";
                        assert(false);
                    }
                    else
                        serum_map[sr_no] = *index;
                }
                  // ---- test titers -----
                for (auto ag_no : antigens) {
                    for (auto sr_no : sera) {
                        const auto titer = result->titers()->titer_of_layer(layer_no, ag_no, sr_no);
                        const auto titer_orig = charts[layer_no]->titers()->titer(antigen_map[ag_no], serum_map[sr_no]);
                        if (titer != titer_orig) {
                            std::cerr << "ERROR: titer mismatch:\n"
                                      << titer << " result: " << ag_no << ' ' << result->antigens()->at(ag_no)->full_name() << ' ' << sr_no << ' ' << result->sera()->at(sr_no)->full_name()
                                      << '\n' << titer_orig << " orig chart " << layer_no << ' ' << antigen_map[ag_no] << ' ' << charts[layer_no]->antigens()->at(antigen_map[ag_no])->full_name() << ' ' << serum_map[sr_no] << ' ' << charts[layer_no]->sera()->at(serum_map[sr_no])->full_name() << "\n";
                            assert(titer == titer_orig);
                        }
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
