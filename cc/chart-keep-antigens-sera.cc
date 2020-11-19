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

    option<str>  antigens_to_keep{*this, 'a', "antigens", desc{"comma or space separated list of antigen indexes to keep (0-based), keep all if empty"}};
    option<str>  sera_to_keep{*this, 's', "sera", desc{"comma or space separated list of serum indexes to keep (0-based), keep all if empty"}};
    option<size_t> keep_recent{*this, "reference-and-recent", desc{"keep all reference and few recent antigens to have specified number of antigens in the output"}};
    option<bool> remove_projections{*this, "remove-projections"};

    argument<str> source{*this, arg_name{"source-chart"}, mandatory};
    argument<str> output{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(*opt.source)};

        acmacs::chart::PointIndexList antigens_to_keep{opt.antigens_to_keep->empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(*opt.antigens_to_keep)};
        acmacs::chart::PointIndexList sera_to_keep{opt.sera_to_keep->empty() ? acmacs::Indexes{} : acmacs::string::split_into_size_t(*opt.sera_to_keep)};

        if (opt.keep_recent.has_value()) {
            std::vector<std::pair<size_t, acmacs::chart::Date>> test_antigens;
            auto antigens = chart.antigens();
            for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
                if (antigen->reference())
                    antigens_to_keep.push_back(ag_no);
                else
                    test_antigens.emplace_back(ag_no, antigen->date());
            }
            std::sort(std::begin(test_antigens), std::end(test_antigens), [](const auto& ag1, const auto& ag2) { return ag1.second > ag2.second; }); // most recent first
            for (auto ta = test_antigens.begin(); ta != test_antigens.end() && antigens_to_keep.size() < *opt.keep_recent; ++ta)
                antigens_to_keep.push_back(ta->first);

            fmt::print(stderr, "DEBUG: antigens_to_keep {} test_antigens {}\n", antigens_to_keep.size(), test_antigens.size());
        }

        if (opt.remove_projections)
            chart.projections_modify().remove_all();

        if (!antigens_to_keep->empty()) {
            acmacs::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
            antigens_to_remove.remove(*antigens_to_keep);
            AD_INFO("antigens_to_remove:  {} {}", antigens_to_remove.size(), antigens_to_remove);
            chart.remove_antigens(antigens_to_remove);
        }

        if (!sera_to_keep->empty()) {
            acmacs::ReverseSortedIndexes sera_to_remove(chart.number_of_sera());
            sera_to_remove.remove(*sera_to_keep);
            AD_INFO("sera_to_remove: {} {}", sera_to_remove.size(), sera_to_remove);
            chart.remove_sera(sera_to_remove);
        }

        acmacs::chart::export_factory(chart, *opt.output, opt.program_name());
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
