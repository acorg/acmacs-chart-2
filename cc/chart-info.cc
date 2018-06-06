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
                {"--column-bases", false},
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
                std::cout << chart->make_info() << '\n';
                if (const auto having_too_few_numeric_titers = chart->titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers.empty())
                    std::cout << "Points having too few numeric titers: " << having_too_few_numeric_titers << '\n';
                if (args["--column-bases"]) {
                    // Timeit ti("column bases computing ");
                    auto cb = chart->computed_column_bases(acmacs::chart::MinimumColumnBasis{});
                    std::cout << "computed column bases: " << *cb << '\n';
                    if (chart->number_of_projections()) {
                        if (auto fcb = chart->projection(0)->forced_column_bases(); fcb)
                            std::cout << "forced column bases: " << *fcb << '\n';
                    }
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
