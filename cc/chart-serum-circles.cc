#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
      //using namespace std::string_literals;

    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--projection", 0UL},
                {"--verbose", false},
                {"--time", false, "report time of loading chart"},
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
            const size_t projection_no = args["--projection"];
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            if (chart->number_of_projections() == 0)
                throw std::runtime_error("chart has no projections");
            if (chart->number_of_projections() <= projection_no)
                throw std::runtime_error("invalid projection number");
            chart->set_homologous(acmacs::chart::Chart::find_homologous_for_big_chart::yes);
            auto antigens = chart->antigens();
            const auto antigen_no_num_digits = static_cast<int>(std::log10(antigens->size())) + 1;
            auto sera = chart->sera();
            const auto serum_no_num_digits = static_cast<int>(std::log10(sera->size())) + 1;
            auto titers = chart->titers();
            for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
                const auto antigen_indexes = serum->homologous_antigens();

                std::cout << "SR " << std::setw(serum_no_num_digits) << sr_no << ' ' << serum->full_name_with_fields() << '\n';
                if (!antigen_indexes.empty()) {
                    std::cout << "   titer theor empir\n";
                    for (auto ag_no : antigen_indexes) {
                        std::cout << "  ";
                        if (const auto homologous_titer = titers->titer(ag_no, sr_no); homologous_titer.is_regular()) {
                            const auto theoretical = chart->serum_circle_radius_theoretical(ag_no, sr_no, projection_no, false);
                            const auto empirical = chart->serum_circle_radius_empirical(ag_no, sr_no, projection_no, false);
                            std::cout << std::setw(6) << std::right << homologous_titer
                                      << ' ' << std::setw(5) << std::fixed << std::setprecision(2) << theoretical
                                      << ' ' << std::setw(5) << std::fixed << std::setprecision(2) << empirical;
                        }
                        else {
                            std::cout << homologous_titer << " - -";
                        }
                        std::cout << "   " << std::setw(antigen_no_num_digits) << ag_no << ' ' << (*antigens)[ag_no]->full_name_with_passage() << '\n';
                    }
                }
                else {
                    std::cout << "    no antigens\n";
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
