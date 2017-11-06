#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart/factory-import.hh"
#include "acmacs-chart/factory-export.hh"
#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <input-chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            auto chart = acmacs::chart::import_factory(args[0], acmacs::chart::Verify::All);
            std::cout << chart->make_info() << '\n';
            acmacs::chart::export_factory(chart, args[1], fs::path(args.program()).filename());
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
