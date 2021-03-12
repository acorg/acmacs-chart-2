#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--mode", "strict", "homologous mode: strict, relaxed_strict, relaxed, all (see chart.hh)"},
                           {"--time", false, "report time of loading chart"},
                           {"--verbose", false},
                           {"-h", false},
                           {"--help", false},
                           {"-v", false},
                       });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            fmt::print(stderr, "Usage: {} [options] <chart-file>\n{}\n", args.program(), args.usage_options());
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);

            acmacs::chart::find_homologous options{acmacs::chart::find_homologous::strict};
            if (args["--mode"] == "relaxed_strict")
                options = acmacs::chart::find_homologous::relaxed_strict;
            else if (args["--mode"] == "relaxed")
                options = acmacs::chart::find_homologous::relaxed;
            else if (args["--mode"] == "all")
                options = acmacs::chart::find_homologous::all;
            else if (args["--mode"] != "strict")
                AD_WARNING("unecognized --mode argument, strict assumed");

            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto sera = chart->sera();
            chart->set_homologous(options, sera);

            for (const auto sr_no : range_from_0_to(chart->number_of_sera())) {
                fmt::print("{}\n", acmacs::chart::format_serum("{no0:{num_digits}d} {name_full_passage}", *chart, sr_no, acmacs::chart::collapse_spaces_t::no));
                for (const auto ag_no : sera->at(sr_no)->homologous_antigens())
                    fmt::print("      {}\n", acmacs::chart::format_antigen("{no0:{num_digits}d} {name_full}", *chart, ag_no, acmacs::chart::collapse_spaces_t::no));
                fmt::print("\n");
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
