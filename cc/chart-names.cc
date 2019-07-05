#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> fields{*this, "fields", desc{"report names with fields"}};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool> verbose{*this, 'v', "verbose"};
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        for (const auto& chart_filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(chart_filename, acmacs::chart::Verify::None, do_report_time(opt.report_time));
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
            for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
                std::cout << "AG " << std::setw(num_digits) << ag_no << ' ';
                if (opt.fields)
                    std::cout << antigen->full_name_with_fields() << '\n';
                else
                    std::cout << string::join(" ", {antigen->full_name_with_passage(), "[" + static_cast<std::string>(antigen->date()) + "]", string::join(" ", antigen->lab_ids())})
                              << (antigen->reference() ? " Ref" : "") << '\n';
            }
            for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
                std::cout << "SR " << std::setw(num_digits) << sr_no << ' ';
                if (opt.fields)
                    std::cout << serum->full_name_with_fields() << '\n';
                else
                    std::cout << string::join(" ", {serum->full_name_with_passage(), serum->serum_species()}) << '\n';
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
