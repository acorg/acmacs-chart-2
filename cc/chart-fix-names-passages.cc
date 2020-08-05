#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-virus/virus-name-normalize.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> input_chart{*this, arg_name{"input-chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.input_chart)};
        // fmt::print("{}\n", chart->make_info());
        const auto subtype = chart.info()->virus_type();

        auto antigens = chart.antigens_modify();
        for (size_t ag_no{0}; ag_no < antigens->size(); ++ag_no) {
            auto& antigen = antigens->at(ag_no);
            auto parsed = acmacs::virus::name::parse(antigen.name());
            if (*parsed.subtype == "A")
                parsed.subtype = subtype;
            const auto new_name = parsed.name();
            if (antigen.name() != new_name) {
                // fmt::print("\"{}\" <- \"{}\"\n", new_name, antigen.name());
                antigen.name(*new_name);
            }
        }

        auto sera = chart.sera_modify();
        for (size_t sr_no{0}; sr_no < sera->size(); ++sr_no) {
            auto& serum = sera->at(sr_no);
            auto parsed = acmacs::virus::name::parse(serum.name());
            if (*parsed.subtype == "A")
                parsed.subtype = subtype;
            const auto new_name = parsed.name();
            if (serum.name() != new_name) {
                // fmt::print("\"{}\" <- \"{}\"\n", new_name, serum.name());
                serum.name(*new_name);
            }
        }

        acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name());
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
