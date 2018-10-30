#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--match", "auto", "match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""},
                {"--scaling", false, "use scaling"},
                {"-p", 0, "primary projection no"},
                {"-r", 0, "secondary projection no"},
                {"--time", false, "test speed"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> [<chart-file>]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            const auto match_level = acmacs::chart::CommonAntigensSera::match_level(args["--match"]);
            auto chart1 = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto chart2 = (args.number_of_arguments() > 1 && std::string(args[0]) != args[1]) ? acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report) : chart1;
            acmacs::chart::CommonAntigensSera common(*chart1, *chart2, match_level);
            if (common) {
                auto procrustes_data = acmacs::chart::procrustes(*chart1->projection(args["-p"]), *chart2->projection(args["-r"]), common.points(), args["--scaling"] ? acmacs::chart::procrustes_scaling_t::yes : acmacs::chart::procrustes_scaling_t::no);
                std::cout << "common antigens: " << common.common_antigens() << " sera: " << common.common_sera() << '\n';
                  // std::cout << "common points:" << common.points() << '\n';
                std::cout << "transformation: " << acmacs::to_string(procrustes_data.transformation) << '\n';
                std::cout << "rms: " << acmacs::to_string(procrustes_data.rms) << '\n';
                  // common.report();
            }
            else {
                std::cerr << "ERROR: no common antigens/sera\n";
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
