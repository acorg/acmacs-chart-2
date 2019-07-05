// --> cxx
#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> name_only{*this, "name-only"};
    argument<str> chart{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.chart);
        auto antigens = chart->antigens();
        auto sera = chart->sera();
        auto titers = chart->titers();
        auto without_titers = titers->having_too_few_numeric_titers(1);
        for (auto index : without_titers) {
            if (index < antigens->size()) {
                if (!opt.name_only)
                    std::cout << "AG " << std::setw(4) << std::right << index << ' ';
                std::cout << antigens->at(index)->full_name() << '\n';
            }
            else {
                std::cout << "SR " << std::setw(3) << std::right << (index - antigens->size()) << ' ' << sera->at(index - antigens->size())->full_name() << '\n';
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
