#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> ...\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            for (size_t file_no = 0; file_no < args.number_of_arguments(); ++file_no) {
                auto chart = acmacs::chart::import_from_file(args[file_no], acmacs::chart::Verify::None, report);
                auto titers = chart->titers();
                const auto num_antigens = chart->number_of_antigens(), num_sera = chart->number_of_sera();
                size_t sum = 0;
                for (size_t ag_no = 0; ag_no < num_antigens; ++ag_no)
                    for (size_t sr_no = 0; sr_no < num_sera; ++sr_no)
                        if (auto tt = titers->titer(ag_no, sr_no); !tt.is_dont_care())
                            sum += tt.value();
                std::cout << "INFO: sum " << sum << '\n';
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
