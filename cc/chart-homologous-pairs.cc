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
        argc_argv args(argc, argv,
                       {
                           {"--mode", "strict", "homologous mode: strict, relaxed_strict, relaxed, all (see chart.hh)"},
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
            const auto report = do_report_time(args["--time"]);

            acmacs::chart::find_homologous options{acmacs::chart::find_homologous::strict};
            if (args["--mode"] == "relaxed_strict")
                options = acmacs::chart::find_homologous::relaxed_strict;
            else if (args["--mode"] == "relaxed")
                options = acmacs::chart::find_homologous::relaxed;
            else if (args["--mode"] == "all")
                options = acmacs::chart::find_homologous::all;
            else if (args["--mode"] != "strict")
                std::cerr << "WARNING: unecognized --mode argument, strict assumed\n";

            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            chart->set_homologous(options, sera);
            const auto ag_num_digits = static_cast<int>(std::log10(antigens->size())) + 1;
            const auto sr_num_digits = static_cast<int>(std::log10(antigens->size())) + 1;

            for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
                std::cout << std::setw(sr_num_digits) << std::right << sr_no << ' ' << serum->full_name_with_passage() << '\n';
                for (auto ag_no : serum->homologous_antigens())
                    std::cout << "      " << std::setw(ag_num_digits) << std::right << ag_no << ' ' << (*antigens)[ag_no]->full_name() << '\n';
                std::cout << '\n';
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
