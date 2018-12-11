#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <array>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-export.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {{"--time", false, "report time of loading chart"}, {"-h", false}, {"--help", false}, {"-v", false}, {"--verbose", false}});
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 0) {
            std::cerr << "Usage: " << args.program() << " [options]\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            constexpr size_t num_antigens = 27, num_sera = 8;
            acmacs::chart::ChartNew chart(num_antigens, num_sera);
            if (!chart.make_name().empty())
                throw std::runtime_error("invalid new chart name");
            const auto exported = acmacs::chart::export_factory(chart, acmacs::chart::export_format::ace, args.program(), report_time::no);
            auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report_time::no);
            // std::cout << exported << '\n';

            if (imported->number_of_antigens() != num_antigens)
                throw std::runtime_error("invalid number_of_antigens");
            if (imported->number_of_sera() != num_sera)
                throw std::runtime_error("invalid number_of_sera");

            auto titers = chart.titers();
            for (auto ti_source : *titers) {
                if (!ti_source.titer.is_dont_care())
                    throw std::runtime_error("unexpected titer: [" + acmacs::to_string(ti_source) + "], expected: *");
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
