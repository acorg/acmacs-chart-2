#include <fstream>
#include <cctype>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/log.hh"

// ----------------------------------------------------------------------

static std::shared_ptr<acmacs::chart::ChartNew> import_from_file(std::string_view filename);
static std::vector<std::string> read_fields(std::istream& input);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> table{*this, arg_name{"table.txt"}, mandatory};
};

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(acmacs::log::relax);
        acmacs::log::enable(acmacs::log::report_stresses);

        auto chart = import_from_file(opt.table);
        fmt::print("{}\n", chart->make_info());
        chart->relax(acmacs::chart::number_of_optimizations_t{20}, acmacs::chart::MinimumColumnBasis("none"), acmacs::number_of_dimensions_t{2}, acmacs::chart::use_dimension_annealing::yes,
                     acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_cg_pca, acmacs::chart::optimization_precision::rough));
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::ChartNew> import_from_file(std::string_view filename)
{
    std::ifstream input(std::string{filename});
    const auto serum_names = read_fields(input);
    std::vector<std::vector<std::string>> antigens_titers;
    while (input)
        antigens_titers.push_back(read_fields(input));
    auto chart = std::make_shared<acmacs::chart::ChartNew>(antigens_titers.size(), serum_names.size());
    auto& sera = chart->sera_modify();
    for (auto [serum_no, serum_name] : acmacs::enumerate(serum_names))
        sera.at(serum_no).name(serum_name);
    auto& antigens = chart->antigens_modify();
    auto& titers = chart->titers_modify();
    for (auto [antigen_no, row] : acmacs::enumerate(antigens_titers)) {
        for (auto [f_no, field] : acmacs::enumerate(row)) {
            if (f_no == 0)
                antigens.at(antigen_no).name(field);
            else
                titers.titer(antigen_no, f_no - 1, acmacs::chart::Titer{field});
        }
    }
    return chart;

} // import_from_file

// ----------------------------------------------------------------------

std::vector<std::string> read_fields(std::istream& input)
{
    std::vector<std::string> result{""};
    for (auto c = input.get(); std::char_traits<char>::not_eof(c); c = input.get()) {
        if (c == '\n')
            break;
        if (std::isspace(c)) {
            if (!result.back().empty())
                result.emplace_back();
        }
        else {
            result.back().append(1, static_cast<char>(c));
        }
    }
    if (result.back().empty())
        result.erase(result.end() - 1);
    return result;

} // read_fields

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
