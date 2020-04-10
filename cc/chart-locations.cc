#include "acmacs-base/argv.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> test_only{*this, 't', "test-only", desc{"test antigens only"}};
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto& locdb = acmacs::locationdb::get();
        std::set<std::string> locations;
        for (const auto& chart_filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(chart_filename);
            auto antigens = chart->antigens();
            for (auto antigen : *antigens) {
                if (!opt.test_only || !antigen->reference()) {
                    try {
                        locations.emplace(locdb.find_or_throw(antigen->location()).location_name);
                    }
                    catch (std::exception&) {
                    }
                }
            }
        }
        for (const auto& loc : locations)
            fmt::print("{}\n", loc);
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
