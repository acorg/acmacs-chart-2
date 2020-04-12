#include "acmacs-base/fmt.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        if (argc != 2)
            throw std::runtime_error(std::string("usage: ") + argv[0] + " <chart-file>");

        using namespace acmacs::chart;

        ChartModify chart{acmacs::chart::import_from_file(argv[1])};
        chart.relax(number_of_optimizations_t{1}, "none", acmacs::number_of_dimensions_t{2}, use_dimension_annealing::yes, optimization_options{}, report_stresses::no);
        auto projections = chart.projections_modify();
        fmt::print(stderr, "no: {}\n", projections->size());
        fmt::print(stderr, "tr {}: {}\n", 0, (*projections)[0]->transformation());
        for (size_t no = 0; no < projections->size(); ++no)
            fmt::print(stderr, "tr {}: {}\n", no, projections->at(no)->transformation());
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
