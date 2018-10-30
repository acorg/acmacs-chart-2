#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"-o", "", "output chart"},
                           {"--match", "auto", "match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""},
                           {"--time", false, "test speed"},
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> ...\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            const auto match_level = acmacs::chart::CommonAntigensSera::match_level(args["--match"]);
            auto read = [report](std::string_view filename) { return acmacs::chart::import_from_file(filename, acmacs::chart::Verify::None, report); };
            auto chart1 = read(args[0]);
            auto chart2 = read(args[1]);
            auto [result, merge_report] = acmacs::chart::merge(*chart1, *chart2);
            std::cout << chart1->description() << '\n' << chart2->description() << "\n\n";
            merge_report.common.report();
            std::cout << "----------\n\n";
            for (size_t c_no = 2; c_no < args.number_of_arguments(); ++c_no) {
                auto chart3 = read(args[c_no]);
                std::cout << result->description() << '\n' << chart3->description() << "\n\n";
                std::tie(result, merge_report) = acmacs::chart::merge(*result, *chart3);
                merge_report.common.report();
                std::cout << "----------\n\n";
            }
            if (args["-o"])
                acmacs::chart::export_factory(*result, args["-o"], fs::path(args.program()).filename(), report);
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
