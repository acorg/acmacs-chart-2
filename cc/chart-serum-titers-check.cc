// CDC H3 FRA tables in 2016-2018 have over-reacting sera with titers like 400, 800, etc.
// There are (many) mistakes in excel sheets with missing or wrongly adding ending 0 in titers.

#include <iostream>
#include <vector>
#include <array>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    using namespace std::string_literals;

    constexpr std::array<size_t, 13> overreacting{100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200, 102400, 204800, 409600};
    auto is_overreacting = [&overreacting](size_t val) -> bool { return std::find(std::begin(overreacting), std::end(overreacting), val) != std::end(overreacting); };

    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--verbose", false},
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "CDC H3 FRA tables in 2016-2018 have over-reacting sera with titers like 400, 800, etc.\n"
                      << "There are (many) mistakes in excel sheets with missing or wrongly adding ending 0 in titers.\n\n"
                      << "Usage: " << args.program() << " [options] <chart-file> ...\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            for (size_t file_no = 0; file_no < args.number_of_arguments(); ++file_no) {
                std::cerr << "DEBUG: " << args[file_no] << '\n';
                auto chart = acmacs::chart::import_from_file(args[file_no], acmacs::chart::Verify::None, report);
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
                    std::cerr << "WARNING: " << args[file_no] << " has invalid sera\n";
                    for (auto serum_no : invalid_sera)
                        std::cerr << "  " << std::setw(3) << std::right << (serum_no + 1) << ' ' << sera->at(serum_no)->full_name() << '\n';
                }
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
