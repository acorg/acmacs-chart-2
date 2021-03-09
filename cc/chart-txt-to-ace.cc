#include <cctype>

#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/file-stream.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

static std::shared_ptr<acmacs::chart::ChartNew> import_from_file(std::string_view filename);
static std::vector<std::string> read_fields(std::istream& input);

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    argument<str> input_table{*this, arg_name{"table.txt"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = import_from_file(opt.input_table);
        acmacs::chart::export_factory(*chart, opt.output_chart, opt.program_name());

        std::cout << chart->make_info() << '\n';
        auto antigens = chart->antigens();
        auto sera = chart->sera();
        const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
        for (auto [ag_no, antigen] : acmacs::enumerate(*antigens))
            std::cout << "AG " << std::setw(num_digits) << ag_no << ' ' << antigen->format("{name_full}") << '\n';
        for (auto [sr_no, serum] : acmacs::enumerate(*sera))
            std::cout << "SR " << std::setw(num_digits) << sr_no << ' ' << serum->format("{name_full}") << '\n';
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
    acmacs::file::ifstream input(filename);
    const auto serum_names = read_fields(input);
    const std::vector<std::string> serum_ids = (input && std::isspace(input->peek())) ? read_fields(input) : std::vector<std::string>{};
    std::vector<std::vector<std::string>> antigens_titers;
    while (input) {
        if (const auto fields = read_fields(input); !fields.empty())
            antigens_titers.push_back(fields);
    }
    auto chart = std::make_shared<acmacs::chart::ChartNew>(antigens_titers.size(), serum_names.size());
    auto& sera = chart->sera_modify();
    for (auto [serum_no, serum_name] : acmacs::enumerate(serum_names))
        sera.at(serum_no).name(serum_name);
    if (serum_ids.size() == serum_names.size()) {
        for (auto [serum_no, serum_id] : acmacs::enumerate(serum_ids))
            sera.at(serum_no).serum_id(acmacs::chart::SerumId{serum_id});
    }
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
            result.back().append(1, c);
        }
    }
    if (result.back().empty())
        result.erase(result.end() - 1);
    return result;

} // read_fields

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
