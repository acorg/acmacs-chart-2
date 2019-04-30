#include <iostream>

#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        if (argc != 2)
            throw std::runtime_error(std::string("usage: ") + argv[0] + " <chart-file>");

        std::cout << argv[0] << ' ' << argv[1] << '\n';
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(argv[1], acmacs::chart::Verify::None, report_time::no)};
        auto p = chart.projection_modify(0)->clone(chart);
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
