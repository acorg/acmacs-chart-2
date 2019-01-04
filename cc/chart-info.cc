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

    option<bool> column_bases{*this, "column-bases"};
    option<bool> list_tables{*this, "list-tables"};
    option<bool> verify{*this, "verify"};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str_array> charts{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        for (size_t file_no = 0; file_no < opt.charts->size(); ++file_no) {
            auto chart = acmacs::chart::import_from_file((*opt.charts)[file_no], opt.verify ? acmacs::chart::Verify::All : acmacs::chart::Verify::None, report);
            std::cout << chart->make_info() << '\n';
            if (const auto having_too_few_numeric_titers = chart->titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers.empty())
                std::cout << "Points having too few numeric titers: " << having_too_few_numeric_titers.size() << ' ' << having_too_few_numeric_titers << '\n';
            if (opt.column_bases) {
                // Timeit ti("column bases computing ");
                auto cb = chart->computed_column_bases(acmacs::chart::MinimumColumnBasis{});
                std::cout << "computed column bases: " << *cb << '\n';
                for (auto projection_no : acmacs::range(chart->number_of_projections())) {
                    if (auto fcb = chart->projection(projection_no)->forced_column_bases(); fcb)
                        std::cout << "forced column bases for projection " << projection_no << ": " << *fcb << '\n';
                }
            }
            if (auto info = chart->info(); opt.list_tables && info->number_of_sources() > 0) {
                std::cout << "\nTables:\n";
                for (auto src_no : acmacs::range(info->number_of_sources()))
                    std::cout << std::setw(3) << src_no << ' ' << info->source(src_no)->make_name() << '\n';
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
