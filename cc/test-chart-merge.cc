#include <iostream>
#include <cassert>

#include "acmacs-base/fmt.hh"
#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> ignore_passages{*this, "ignore-passages"};

    argument<str_array> charts{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::vector<acmacs::chart::ChartP> charts(opt.charts->size());
        std::transform(acmacs::index_iterator(0UL), acmacs::index_iterator(opt.charts->size()), charts.begin(), [&opt](size_t index) { return acmacs::chart::import_from_file(opt.charts->at(index)); });
        const acmacs::chart::MergeSettings settings(opt.ignore_passages ? acmacs::chart::CommonAntigensSera::match_level_t::ignored : acmacs::chart::CommonAntigensSera::match_level_t::strict);
        auto [result, merge_report] = acmacs::chart::merge(*charts[0], *charts[1], settings);
        for (size_t c_no = 2; c_no < charts.size(); ++c_no) {
            std::tie(result, merge_report) = acmacs::chart::merge(*result, *charts[c_no], settings);
        }

        acmacs::chart::PointIndexList antigens_visited, sera_visited;
        for (auto [layer_no, chart] : acmacs::enumerate(charts)) {
            const auto [antigens, sera] = result->titers()->antigens_sera_of_layer(layer_no);
            antigens_visited.extend(antigens);
            sera_visited.extend(sera);

            std::map<size_t, size_t> antigen_map, serum_map;
            // ---- test antigens -----
            // std::cerr << "\nDEBUG: layer " << layer_no << '\n';
            for (auto ag_no : antigens) {
                const auto full_name = result->antigens()->at(ag_no)->name_full();
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
                const auto full_name = result->sera()->at(sr_no)->name_full();
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
                                  << *titer << " result: " << ag_no << ' ' << result->antigens()->at(ag_no)->name_full() << ' ' << sr_no << ' ' << result->sera()->at(sr_no)->name_full() << '\n'
                                  << *titer_orig << " orig chart " << layer_no << ' ' << antigen_map[ag_no] << ' ' << charts[layer_no]->antigens()->at(antigen_map[ag_no])->name_full() << ' '
                                  << serum_map[sr_no] << ' ' << charts[layer_no]->sera()->at(serum_map[sr_no])->name_full() << "\n";
                        assert(titer == titer_orig);
                    }
                }
            }
        }
        assert(antigens_visited->size() == result->number_of_antigens());
        assert(sera_visited->size() == result->number_of_sera());
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
