#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--match", "auto", "match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""},
                           {"--time", false, "test speed"},
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto match_level{acmacs::chart::CommonAntigensSera::match_level_t::automatic};
            if (const std::string match_level_s = args["--match"].str(); !match_level_s.empty()) {
                switch (match_level_s[0]) {
                    case 's':
                        match_level = acmacs::chart::CommonAntigensSera::match_level_t::strict;
                        break;
                    case 'r':
                        match_level = acmacs::chart::CommonAntigensSera::match_level_t::relaxed;
                        break;
                    case 'i':
                        match_level = acmacs::chart::CommonAntigensSera::match_level_t::ignored;
                        break;
                    case 'a':
                        match_level = acmacs::chart::CommonAntigensSera::match_level_t::automatic;
                        break;
                    default:
                        std::cerr << "Unrecognized --match argument, automatic assumed" << '\n';
                        break;
                }
            }
            if (std::string(args[0]) == args[1]) {
                auto chart1 = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
                acmacs::chart::CommonAntigensSera common(*chart1);
                common.report();
            }
            else {
                auto chart1 = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
                auto chart2 = acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report);
                acmacs::chart::CommonAntigensSera common(*chart1, *chart2, match_level);
                common.report();
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
