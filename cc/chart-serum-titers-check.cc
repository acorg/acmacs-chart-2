// CDC H3 FRA tables in 2016-2018 have over-reacting sera with titers like 400, 800, etc.
// There are (many) mistakes in excel sheets with missing or wrongly adding ending 0 in titers.

#include <vector>
#include <array>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> verbose{*this, 'v', "verbose"};

    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    constexpr std::array<size_t, 13> overreacting{100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600};
    auto is_overreacting = [&overreacting](size_t val) -> bool { return std::find(std::begin(overreacting), std::end(overreacting), val) != std::end(overreacting); };

    int exit_code = 0;
    try {
        fmt::print("CDC H3 FRA tables in 2016-2018 have over-reacting sera with titers like 400, 800, etc.\n"
                   "There are (many) mistakes in excel sheets with missing or wrongly adding ending 0 in titers.\n\n");
        Options opt(argc, argv);
        for (size_t file_no = 0; file_no < opt.charts->size(); ++file_no) {
            const auto filename = (*opt.charts)[file_no];
            if (opt.verbose)
                fmt::print(stderr, "DEBUG: {:3d} {}\n", file_no + 1, filename);
            auto chart = acmacs::chart::import_from_file(filename);
            auto sera = chart->sera();
            auto titers = chart->titers();
            std::vector<std::pair<size_t, size_t>> titer_per_serum(sera->size()); // pair: number of regular titers, number of over-reacting titers
            for (const auto& titer : titers->titers_existing()) {
                if (is_overreacting(titer.titer.value()))
                    ++titer_per_serum[titer.serum].second;
                else
                    ++titer_per_serum[titer.serum].first;
            }
            std::vector<size_t> invalid_sera;
            for (auto [serum_no, data] : acmacs::enumerate(titer_per_serum)) {
                if (data.first && data.second)
                    invalid_sera.push_back(serum_no);
            }
            if (!invalid_sera.empty()) {
                fmt::print(stderr, "WARNING: {} has invalid sera\n", filename);
                for (auto serum_no : invalid_sera)
                    fmt::print(stderr, "  {:3d} {}\n", serum_no + 1, sera->at(serum_no)->full_name());
            }
        }
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
