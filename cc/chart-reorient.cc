#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/procrustes.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--match", "auto", "match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""},
                {"-m", 0, "master projection no"},
                {"-p", 0, "chart projection no, -1 to orient all"},
                {"--time", false, "test speed"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 3) {
            std::cerr << "Usage: " << args.program() << " [options] <master-chart> <chart-to-reorient> <output-chart>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            const auto match_level = acmacs::chart::CommonAntigensSera::match_level(args["--match"]);
            auto master = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            acmacs::chart::ChartModify to_reorient{acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report)};
            acmacs::chart::CommonAntigensSera common(*master, to_reorient, match_level);
            if (common) {
                std::cout << "common antigens: " << common.common_antigens() << " sera: " << common.common_sera() << '\n';
                  // std::cout << "common points:" << common.points() << '\n';
                  // common.report();
                const auto projections = static_cast<int>(args["-p"]) == -1 ? acmacs::filled_with_indexes(to_reorient.number_of_projections()) : std::vector<size_t>{static_cast<size_t>(args["-p"])};
                for (auto projection_no : projections) {
                    auto procrustes_data = acmacs::chart::procrustes(*master->projection(args["-m"]), *to_reorient.projection(projection_no), common.points(), acmacs::chart::procrustes_scaling_t::no);
                    to_reorient.projection_modify(projection_no)->transformation(procrustes_data.transformation);
                    std::cout << "projection: " << projection_no << '\n';
                    std::cout << "transformation: " << acmacs::to_string(procrustes_data.transformation) << '\n';
                    std::cout << "rms: " << acmacs::to_string(procrustes_data.rms) << "\n\n";
                }
                acmacs::chart::export_factory(to_reorient, args[2], fs::path(args.program()).filename(), report);
            }
            else {
                std::cerr << "ERROR: no common antigens/sera\n";
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
