#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/file-stream.hh"
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
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 3) {
            std::cerr << "Usage: " << args.program() << " [options] <file-with-antigen-names> <chart-file> <output-chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report_time::no)};
            auto antigens = chart.antigens();
            acmacs::ReverseSortedIndexes antigens_to_remove{acmacs::Indexes{}};
            acmacs::file::ifstream input(args[0]);
            auto& input_stream = input.stream();
            while (input_stream) {
                std::string line;
                std::getline(input_stream, line);
                if (!line.empty()) {
                    if (const auto found = antigens->find_by_full_name(line); found) {
                        antigens_to_remove.add(std::vector<size_t>{*found});
                    }
                    else {
                        std::cerr << "ERROR: " << line << " not found\n";
                        exit_code = 1;
                    }
                }
            }
            if (!antigens_to_remove.empty()) {
                AD_INFO("antigens_to_remove: {} {}", antigens_to_remove.size(), antigens_to_remove);
                chart.remove_antigens(antigens_to_remove);
                acmacs::chart::export_factory(chart, args[2], args.program(), report_time::no);
            }
            else {
                AD_ERROR("nothing to remove!\n");
                exit_code = 1;
            }
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
