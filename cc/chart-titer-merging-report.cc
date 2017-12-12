#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--verify", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const bool verify = args["--verify"];
            auto chart = acmacs::chart::import_factory(args[0], verify ? acmacs::chart::Verify::All : acmacs::chart::Verify::None);

            auto titers = chart->titers();
            if (titers->number_of_layers() < 2)
                throw std::runtime_error{"chart without layers"};

            for (auto ag_no: acmacs::range(0UL, chart->number_of_antigens())) {
                for (auto sr_no: acmacs::range(0UL, chart->number_of_sera())) {
                    auto titer_layers = titers->titers_for_layers(ag_no, sr_no);
                }
            }

              // std::cout << chart->make_info() << '\n';
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
