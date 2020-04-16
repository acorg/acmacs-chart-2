#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-v1.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> chart{*this, arg_name{"chart-file"}, mandatory};
    argument<str> country{*this, arg_name{"country"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto country = string::upper(*opt.country);
        auto chart = acmacs::chart::import_from_file(opt.chart);
        auto antigens = chart->antigens();
        std::map<std::string, std::vector<std::string>> antigens_per_month;
        for (const auto antigen : *antigens) {
            try {
                if (acmacs::locationdb::get().country(::virus_name::location(antigen->name())) == country) {
                    std::string key;
                    if (const auto date = antigen->date(); !date.empty())
                        key = date->substr(0, 7);
                    antigens_per_month[key].push_back(antigen->full_name_with_fields());
                }
            }
            catch (std::exception&) {
            }
        }
        for (const auto& [month, names] : antigens_per_month) {
            fmt::print("{:7s} {:4d}\n", month, names.size());
            for (const auto& name : names)
                fmt::print("    {}\n", name);
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
