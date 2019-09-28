#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};
    argument<str> chart_file{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        auto chart = acmacs::chart::import_from_file(opt.chart_file, acmacs::chart::Verify::None, report);
        auto titers = chart->titers();
        auto antigens = chart->antigens();
        auto sera = chart->sera();
        const acmacs::chart::MinimumColumnBasis min_column_basis{chart->number_of_projections() ? chart->projection(0)->minimum_column_basis() : acmacs::chart::MinimumColumnBasis{}};
        auto column_bases = chart->column_bases(min_column_basis);

        for (auto serum_no : acmacs::range(sera->size())) {
            std::cout << "INFO: SR " << std::setw(4) << std::right << serum_no << ' ' << sera->at(serum_no)->full_name() << '\n';
            for (auto antigen_no : acmacs::range(antigens->size())) {
                if (const auto titer = titers->titer(antigen_no, serum_no); !titer.is_dont_care())
                    std::cout << std::setw(6) << std::right << *titer;
            }
            const auto cb = column_bases->column_basis(serum_no);
            std::cout << '\n' << "  const column basis: " << cb << " --> " << std::lround(std::exp2(cb) * 10) << '\n' << '\n';
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
