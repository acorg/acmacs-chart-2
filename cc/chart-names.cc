#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
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
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> ...\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            for (size_t file_no = 0; file_no < args.number_of_arguments(); ++file_no) {
                auto chart = acmacs::chart::import_factory(args[file_no], acmacs::chart::Verify::None, report);
                auto antigens = chart->antigens();
                auto sera = chart->sera();
                const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
                for (auto [ag_no, antigen]: acmacs::enumerate(*antigens)) {
                    std::cout << "AG " << std::setw(num_digits) << ag_no << " " << string::join(" ", {antigen->name(), string::join(" ", antigen->annotations()), antigen->reassortant(), antigen->passage(), antigen->date(), string::join(" ", antigen->lab_ids())}) << (antigen->reference() ? " Ref" : "") << '\n';
                }
                for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
                    std::cout << "SR " << std::setw(num_digits) << sr_no << " " << string::join(" ", {serum->name(), string::join(" ", serum->annotations()), serum->reassortant(), serum->passage(), serum->serum_id(), serum->serum_species()}) << '\n';
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
