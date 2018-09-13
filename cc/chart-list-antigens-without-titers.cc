// --> cxx
#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--name-only", false},
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
            const bool name_only = args["--name-only"];
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report_time::No);
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            auto titers = chart->titers();
            auto without_titers = titers->having_too_few_numeric_titers(1);
            std::cerr << "DEBUG: " << without_titers << '\n';
            for (auto index : without_titers) {
                if (index < antigens->size()) {
                    if (!name_only)
                        std::cout << "AG " << std::setw(4) << std::right << index << ' ';
                    std::cout << antigens->at(index)->full_name() << '\n';
                }
                else {
                    std::cout << "SR " << std::setw(3) << std::right << (index - antigens->size()) << ' ' << sera->at(index - antigens->size())->full_name() << '\n';
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
