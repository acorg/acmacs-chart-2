#include <iostream>

#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        if (argc < 2)
            throw std::runtime_error(std::string("usage: ") + argv[0] + " <chart> ...");

        for (int arg = 1; arg < argc; ++arg) {
            acmacs::chart::ChartModify master_chart{acmacs::chart::import_from_file(argv[arg], acmacs::chart::Verify::None)};
            for (const auto proportion : {0.1, 0.2, 0.3, 0.4, 0.5}) {
                acmacs::chart::ChartClone chart(master_chart, acmacs::chart::ChartClone::clone_data::titers);
                auto titers = chart.titers_modify();
                const auto initial_titers = titers->number_of_non_dont_cares();
                titers->remove_layers();
                titers->set_proportion_of_titers_to_dont_care(proportion);
                const auto now_titers = titers->number_of_non_dont_cares();
                const auto resulting_proportion = double(initial_titers - now_titers) / initial_titers;
                if (std::abs(resulting_proportion - proportion) > 0.01) {
                    std::cerr << argv[arg] << "  initial: " << initial_titers << "  after dontcaring: " << now_titers << "  resulting proportion: " << resulting_proportion << '\n';
                    throw std::runtime_error("set_proportion_of_titers_to_dont_care failed");
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
