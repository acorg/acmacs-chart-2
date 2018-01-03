#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--projection", 0L},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto stress = chart->make_stress<double>(args["--projection"]);
            std::cerr << "stress d: " << acmacs::to_string(chart->projection(args["--projection"])->calculate_stress(stress)) << '\n';
            auto stress_f = chart->make_stress<float>(args["--projection"]);
            std::cerr << "stress f: " << acmacs::to_string(chart->projection(args["--projection"])->calculate_stress(stress_f)) << '\n';
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
