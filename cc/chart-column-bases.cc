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
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto titers = chart->titers();
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            const acmacs::chart::MinimumColumnBasis min_column_basis{chart->number_of_projections() ? chart->projection(0)->minimum_column_basis() : acmacs::chart::MinimumColumnBasis{}};
            auto column_bases = chart->column_bases(min_column_basis);

            for (auto serum_no : acmacs::range(sera->size())) {
                std::cout << "INFO: SR " << std::setw(4) << std::right << serum_no << ' ' << sera->at(serum_no)->full_name() << '\n';
                for (auto antigen_no : acmacs::range(antigens->size())) {
                    if (const auto titer = titers->titer(antigen_no, serum_no); !titer.is_dont_care())
                        std::cout << std::setw(6) << std::right << titer;
                }
                const auto cb = column_bases->column_basis(serum_no);
                std::cout << '\n'
                        << "  const olumn basis: " << cb << " --> " << std::lround(std::exp2(cb) * 10) << '\n'
                        << '\n';
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
