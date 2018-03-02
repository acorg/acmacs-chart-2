#include <iostream>
#include <unistd.h>
#include <cstdlib>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-export.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--full", false, "full output"},
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);

            acmacs::chart::ChartModify chart_modify{chart};
            chart_modify.info_modify();
            chart_modify.antigens_modify();
            chart_modify.sera_modify();
            chart_modify.titers_modify();
            chart_modify.forced_column_bases_modify();

            const auto plain = acmacs::chart::export_factory(*chart, acmacs::chart::export_format::ace, args.program(), report);
            const auto modified = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
            if (plain != modified) {
                if (args["--full"]) {
                    std::cout << "======== PLAIN ============" << plain << '\n';
                    std::cout << "======== MODIFIED ============" << modified << '\n';
                }
                else {
                    acmacs::file::temp plain_file{".ace"}, modified_file{".ace"};
                    write(plain_file, plain.data(), plain.size());
                    write(modified_file, modified.data(), modified.size());
                    std::system(("/usr/bin/diff -B -b " + static_cast<std::string>(plain_file) + " " + static_cast<std::string>(modified_file)).data());
                }
                throw std::runtime_error("different!");
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
