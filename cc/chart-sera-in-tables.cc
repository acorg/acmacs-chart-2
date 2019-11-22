// #include <vector>
// #include <map>

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

    option<bool> verbose{*this, 'v', "verbose"};

    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{

    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::vector<std::string> table_dates;
        std::map<std::string, std::vector<std::string>> serum_to_tables;
        for (const auto& filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(filename);
            const auto table_date = chart->info()->date();
            table_dates.emplace_back(table_date);
            auto sera = chart->sera();
            for (auto serum : *sera)
                serum_to_tables.try_emplace(serum->full_name(), std::vector<std::string>{}).first->second.emplace_back(table_date);
        }

        fmt::print("Total sera: {}\nTables: {}\n\n", serum_to_tables.size(), table_dates.size());
        for (const auto [no, en] : acmacs::enumerate(serum_to_tables)) {
            fmt::print("{:3d} {}\n     ({})", no + 1, en.first, en.second.size());
            for (const auto& tab : en.second)
                fmt::print(" {}", tab);
            fmt::print("\n");
        }
        fmt::print("\n");

        fmt::print("{:12c}", ' ');
        for (size_t sr_no = 1; sr_no <= serum_to_tables.size(); ++sr_no) {
            if (sr_no < 10)
                fmt::print("  {}  ", sr_no);
            else
                fmt::print(" {:3d} ", sr_no);
        }
        fmt::print("\n");
        for (const auto& table_date : table_dates) {
            fmt::print("{:12s}", table_date);
            for (const auto& [serum, tables] : serum_to_tables) {
                if (const auto found = std::find(std::begin(tables), std::end(tables), table_date); found != std::end(tables))
                    fmt::print("  X  ");
                else
                    fmt::print("     ");
            }
            fmt::print("\n");
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "CDC H3 FRA tables have different sets of sera.\nProgram reports all sera and tables they are in\n\n");
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
