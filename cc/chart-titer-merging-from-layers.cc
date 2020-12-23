#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> chart{*this, arg_name{"chart"}, mandatory};
    argument<size_t> antigen_no{*this, arg_name{"antigen-no"}, mandatory};
    argument<size_t> serum_no{*this, arg_name{"serum-no"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        auto chart = acmacs::chart::import_from_file(opt.chart);
        auto titers = chart->titers();
        if (titers->number_of_layers() < 2)
            throw std::runtime_error{"chart without layers"};
        auto antigens = chart->antigens();
        if (*opt.antigen_no >= antigens->size())
            throw std::runtime_error{"antigen number out of range"};
        auto sera = chart->sera();
        if (*opt.serum_no >= sera->size())
            throw std::runtime_error{"serum number out of range"};
        auto info = chart->info();

        std::cout << "INFO: AG " << std::setw(4) << std::right << *opt.antigen_no << ' ' << antigens->at(opt.antigen_no)->full_name() << '\n';
        std::cout << "INFO: SR " << std::setw(4) << std::right << *opt.serum_no << ' ' << sera->at(opt.serum_no)->full_name() << '\n';
        std::cout << "INFO: Layers: " << titers->number_of_layers() << '\n';
        std::cout << "INFO: Merged: " << *titers->titer(opt.antigen_no, opt.serum_no) << '\n';
        std::cout << "INFO: In layers:\n"; // << titers->titers_for_layers(antigen_no, serum_no) << '\n';
        for (auto layer_no : acmacs::range(titers->number_of_layers())) {
            if (const auto titer = titers->titer_of_layer(layer_no, opt.antigen_no, opt.serum_no); !titer.is_dont_care()) {
                std::cout << std::setw(4) << std::right << layer_no << ' ' << std::setw(10) << std::left << static_cast<std::string_view>(info->source(layer_no)->date()) << *titer << '\n';
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
