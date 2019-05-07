#include <iostream>

#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        if (argc != 3)
            throw std::runtime_error(std::string("usage: ") + argv[0] + " <chart-file> <output-chart-file>");

        auto source_chart = acmacs::chart::import_from_file(argv[1], acmacs::chart::Verify::None);
        acmacs::chart::ChartClone result(source_chart, acmacs::chart::ChartClone::clone_data::projections_plot_spec);
        acmacs::chart::export_factory(result, argv[2], argv[0]);
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
