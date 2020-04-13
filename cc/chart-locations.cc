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
        std::set<std::string> locations;
        for (const auto& chart_filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(chart_filename);
            auto antigens = chart->antigens();
            for (auto antigen : *antigens) {
                if (!opt.test_only || !antigen->reference()) {
                    if (const auto loc = acmacs::locationdb::get().find(antigen->location(), acmacs::locationdb::include_continent::no); loc.has_value())
                        locations.emplace(loc->location_name);
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
