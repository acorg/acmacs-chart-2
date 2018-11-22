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
        argc_argv args(argc, argv,
                       {
                           {"-m", 0, "master projection no"},
                           {"--time", false, "test speed"},
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Re-orients all projections to the master projection of the chart\n"
                      << "Usage: " << args.program() << " [options] <chart> <output-chart>\n"
                      << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify to_reorient{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const size_t master_projection_no{args["-m"]};
            auto master_projection = to_reorient.projection(master_projection_no);
            acmacs::chart::CommonAntigensSera common(to_reorient);
            for (auto projection_no : acmacs::filled_with_indexes(to_reorient.number_of_projections())) {
                if (projection_no != master_projection_no) {
                    auto procrustes_data = acmacs::chart::procrustes(*master_projection, *to_reorient.projection(projection_no), common.points(), acmacs::chart::procrustes_scaling_t::no);
                    to_reorient.projection_modify(projection_no)->transformation(procrustes_data.transformation);
                    std::cout << "projection: " << projection_no << '\n';
                    std::cout << "transformation: " << acmacs::to_string(procrustes_data.transformation) << '\n';
                    std::cout << "rms: " << acmacs::to_string(procrustes_data.rms) << "\n\n";
                }
            }
            acmacs::chart::export_factory(to_reorient, args[1], fs::path(args.program()).filename(), report);
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
