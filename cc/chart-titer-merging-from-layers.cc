#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--time", false, "report time of loading chart"},
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 3) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <antigen-no> <serum-no>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto titers = chart->titers();
            if (titers->number_of_layers() < 2)
                throw std::runtime_error{"chart without layers"};
            auto antigens = chart->antigens();
            const size_t antigen_no = std::stoul(args[1]);
            if (antigen_no >= antigens->size())
                throw std::runtime_error{"antigen number out of range"};
            auto sera = chart->sera();
            const size_t serum_no = std::stoul(args[2]);
            if (serum_no >= sera->size())
                throw std::runtime_error{"serum number out of range"};
            auto info = chart->info();

            std::cout << "INFO: AG " << std::setw(4) << std::right << antigen_no << ' ' << antigens->at(antigen_no)->full_name() << '\n';
            std::cout << "INFO: SR " << std::setw(4) << std::right << serum_no << ' ' << sera->at(serum_no)->full_name() << '\n';
            std::cout << "INFO: Layers: " << titers->number_of_layers() << '\n';
            std::cout << "INFO: Merged: " << titers->titer(antigen_no, serum_no) << '\n';
            std::cout << "INFO: In layers:\n"; // << titers->titers_for_layers(antigen_no, serum_no) << '\n';
            for (auto layer_no : acmacs::range(titers->number_of_layers())) {
                if (const auto titer = titers->titer_of_layer(layer_no, antigen_no, serum_no); !titer.is_dont_care()) {
                    std::cout << std::setw(4) << std::right << layer_no << ' ' << std::setw(10) << std::left << info->source(layer_no)->date() << titer << '\n';
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
