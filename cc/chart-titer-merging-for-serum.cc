#include <iostream>
#include <set>

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

    argument<str> chart{*this, arg_name{"chart-file"}, mandatory};
    argument<size_t> serum_no{*this, arg_name{"serum-no"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.chart, acmacs::chart::Verify::None, report_time::no);
        auto titers = chart->titers();
        if (titers->number_of_layers() < 2)
            throw std::runtime_error("chart has no layers");
        const auto max_antigen_name = static_cast<int>(chart->antigens()->max_full_name());

        std::cout << "SR " << opt.serum_no << ' ' << chart->serum(opt.serum_no)->full_name() << ' ' << static_cast<std::string_view>(chart->serum(opt.serum_no)->passage()) << '\n';
        if (chart->number_of_projections() > 0) {
            if (auto fcb = chart->projection(0)->forced_column_bases(); fcb)
                std::cout << "forced column basis: " << fcb->column_basis(opt.serum_no) << '\n';
        }
        std::cout << std::setw(max_antigen_name + 6) << ' ' << "merge";
        for (size_t layer_no = 0; layer_no < titers->number_of_layers(); ++layer_no)
            std::cout << "    " << std::setw(2) << std::right << layer_no << ' ';
        std::cout << '\n';
        std::set<acmacs::chart::Titer> all_titers;
        for (size_t antigen_no = 0; antigen_no < chart->number_of_antigens(); ++antigen_no) {
            if (std::any_of(acmacs::index_iterator(0UL), acmacs::index_iterator(titers->number_of_layers()),
                            [&titers, antigen_no, serum_no = *opt.serum_no](size_t layer_no) { return !titers->titer_of_layer(layer_no, antigen_no, serum_no).is_dont_care(); })) {
                std::cout << std::setw(4) << std::right << antigen_no << ' ' << std::setw(max_antigen_name) << std::left << chart->antigen(antigen_no)->full_name()
                          << std::setw(6) << std::right << *titers->titer(antigen_no, opt.serum_no);
                for (size_t layer_no = 0; layer_no < titers->number_of_layers(); ++layer_no) {
                    const auto titer = titers->titer_of_layer(layer_no, antigen_no, opt.serum_no);
                    std::cout << std::setw(6) << std::right << *titer << ' ';
                    all_titers.insert(titer);
                }
                std::cout << '\n';
            }
        }
        std::cout << "\nall titers:";
        for (auto titer : all_titers)
            std::cout << ' ' << *titer;
        std::cout << "\n\n";

        auto info = chart->info();
        for (size_t layer_no = 0; layer_no < titers->number_of_layers(); ++layer_no)
            std::cout << std::setw(2) << std::right << layer_no << ' ' << static_cast<std::string_view>(info->source(layer_no)->date()) << '\n';
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
