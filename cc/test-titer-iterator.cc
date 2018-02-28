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
                {"--verify", false},
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
            const bool verify = args["--verify"];
            const auto report = do_report_time(args["--time"]);
            for (size_t file_no = 0; file_no < args.number_of_arguments(); ++file_no) {
                auto chart = acmacs::chart::import_from_file(args[file_no], verify ? acmacs::chart::Verify::All : acmacs::chart::Verify::None, report);
                auto titers = chart->titers();
                size_t non_dont_care_titers = 0;
                for (const auto& titer_data : *titers) {
                    if (titer_data.titer != titers->titer(titer_data.antigen, titer_data.serum))
                        throw std::runtime_error("titer mistmatch for " + std::to_string(titer_data.antigen) + ':' + std::to_string(titer_data.serum) + ": " + titer_data.titer + " vs. " + titers->titer(titer_data.antigen, titer_data.serum));
                    ++non_dont_care_titers;
                }
                if (non_dont_care_titers != titers->number_of_non_dont_cares())
                    throw std::runtime_error("number_of_non_dont_cares mistmatch: " + std::to_string(non_dont_care_titers) + " vs. " + std::to_string(titers->number_of_non_dont_cares()));
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
