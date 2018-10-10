#include <fstream>
#include <cctype>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

static std::shared_ptr<acmacs::chart::ChartNew> import_from_file(std::string_view filename);
static std::vector<std::string> read_fields(std::istream& input);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {{"-h", false}, {"--help", false}, {"-v", false}, {"--verbose", false}});
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <table.txt>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            auto chart = import_from_file(std::string(args[0]));
            std::cout << chart->make_info() << '\n';
            size_t number_of_attempts = 20;
            chart->relax(number_of_attempts, acmacs::chart::MinimumColumnBasis("none"), 2, true, acmacs::chart::optimization_options(acmacs::chart::optimization_method::alglib_cg_pca, acmacs::chart::optimization_precision::rough), true);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
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
    auto sera = chart->sera_modify();
    for (auto [serum_no, serum_name] : acmacs::enumerate(serum_names))
        sera->at(serum_no).name(serum_name);
    auto antigens = chart->antigens_modify();
    auto titers = chart->titers_modify();
    for (auto [antigen_no, row] : acmacs::enumerate(antigens_titers)) {
        for (auto [f_no, field] : acmacs::enumerate(row)) {
            if (f_no == 0)
                antigens->at(antigen_no).name(field);
            else
                titers->titer(antigen_no, f_no - 1, field);
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
            result.back().append(1, c);
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
