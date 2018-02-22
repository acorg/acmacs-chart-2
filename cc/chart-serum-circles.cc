#include <iostream>
#include <fstream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static void report(const argc_argv& args);

int main(int argc, char* const argv[])
{
      //using namespace std::string_literals;

    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--json", "", "write json data into file suitable for ssm report"},
                {"--projection", 0UL},
                {"--verbose", false},
                {"--time", false, "report time of loading chart"},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            report(args);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

struct AntigenData
{
    AntigenData(size_t ag_no, acmacs::chart::AntigenP ag, const acmacs::chart::Titer& a_titer)
        : antigen_no{ag_no}, antigen{ag}, titer{a_titer} {}

    size_t antigen_no;
    acmacs::chart::AntigenP antigen;
    acmacs::chart::Titer titer;
    double theoretical = -1;
    double empirical = -1;
};

struct SerumData
{
    SerumData(size_t sr_no, acmacs::chart::SerumP sr)
        : serum_no{sr_no}, serum{sr} {}

    size_t serum_no;
    acmacs::chart::SerumP serum;
    std::vector<AntigenData> antigens;
};

static std::vector<SerumData> collect(const acmacs::chart::Chart& chart, size_t projection_no);
static void report_text(const acmacs::chart::Chart& chart, const std::vector<SerumData>& sera_data);
static void report_json(std::ostream& output, const acmacs::chart::Chart& chart, const std::vector<SerumData>& sera_data);

void report(const argc_argv& args)
{
    const size_t projection_no = args["--projection"];
    auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, do_report_time(args["--time"]));
    if (chart->number_of_projections() == 0)
        throw std::runtime_error("chart has no projections");
    if (chart->number_of_projections() <= projection_no)
        throw std::runtime_error("invalid projection number");
    chart->set_homologous(acmacs::chart::Chart::find_homologous_for_big_chart::yes);
    auto result = collect(*chart, projection_no);

    if (std::string json_filename = args["--json"]; json_filename.empty())
        report_text(*chart, result);
    else if (json_filename == "-" || json_filename == "=")
        report_json(std::cout, *chart, result);
    else {
        acmacs::file::backup(json_filename);
        std::ofstream output(json_filename);
        report_json(output, *chart, result);
    }

} // report

// ----------------------------------------------------------------------

std::vector<SerumData> collect(const acmacs::chart::Chart& chart, size_t projection_no)
{
    std::vector<SerumData> result;
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto titers = chart.titers();
    for (auto [sr_no, serum]: acmacs::enumerate(*sera)) {
        auto& serum_data = result.emplace_back(sr_no, serum);
        for (auto ag_no : serum->homologous_antigens()) {
            auto& antigen_data = serum_data.antigens.emplace_back(ag_no, (*antigens)[ag_no], titers->titer(ag_no, sr_no));
            if (antigen_data.titer.is_regular()) {
                antigen_data.theoretical = chart.serum_circle_radius_theoretical(ag_no, sr_no, projection_no, false);
                antigen_data.empirical = chart.serum_circle_radius_empirical(ag_no, sr_no, projection_no, false);
            }
        }
    }
    return result;

} // collect

// ----------------------------------------------------------------------

void report_text(const acmacs::chart::Chart& chart, const std::vector<SerumData>& sera_data)
{
    const auto antigen_no_num_digits = static_cast<int>(std::log10(chart.number_of_antigens())) + 1;
    const auto serum_no_num_digits = static_cast<int>(std::log10(chart.number_of_sera())) + 1;
    for (const auto& serum_data : sera_data) {
        std::cout << "SR " << std::setw(serum_no_num_digits) << serum_data.serum_no << ' ' << serum_data.serum->full_name_with_fields() << '\n';
        if (!serum_data.antigens.empty()) {
            std::cout << "   titer theor empir\n";
            for (const auto& antigen_data : serum_data.antigens) {
                std::cout << "  " << std::setw(6) << std::right << antigen_data.titer;
                if (antigen_data.theoretical > 0)
                    std::cout << ' ' << std::setw(5) << std::fixed << std::setprecision(2) << antigen_data.theoretical;
                else
                    std::cout << std::setw(6) << ' ';
                if (antigen_data.empirical > 0)
                    std::cout << ' ' << std::setw(5) << std::fixed << std::setprecision(2) << antigen_data.empirical;
                else
                    std::cout << std::setw(6) << ' ';
                std::cout << "   " << std::setw(antigen_no_num_digits) << antigen_data.antigen_no << ' ' << antigen_data.antigen->full_name_with_passage() << '\n';
            }
        }
        else {
            std::cout << "    no antigens\n";
        }
    }

} // report_text

// ----------------------------------------------------------------------

void report_json(std::ostream& output, const acmacs::chart::Chart& chart, const std::vector<SerumData>& sera_data)
{
    const std::string tag_prefix = chart.info()->lab(acmacs::chart::Info::Compute::Yes) + "_" + string::replace(chart.info()->assay(), " ", "_") + '_';
    const char* mod_pre = "        {\"N\": \"clades_light\", \"size\": 8},\n";

    output << "{ \"_\":\"-*- js-indent-level: 2 -*-\",\n  \"mods\": {\n";
    output << "    \"?? " << tag_prefix << "\": false,\n\n";

    for (const auto& serum_data : sera_data) {
        const std::string serum_name_infix = string::replace(serum_data.serum->full_name_without_passage(), "/", "_", " ", "_", "A(H1N1)", "H1", "A(H3N2)", "H3", ",", "_");
        if (!serum_data.antigens.empty()) {
            for (const auto& antigen_data : serum_data.antigens) {
                output << "    \"?? SR " << serum_data.serum_no << ' ' << serum_data.serum->full_name_with_passage() << "\": false,\n"
                       << "    \"?? AG " << antigen_data.antigen_no << ' ' << antigen_data.antigen->full_name_with_passage() << "\": false,\n"
                       << "    \"?? titer:" << antigen_data.titer << "  theoretical:" << antigen_data.theoretical << "  empirical:" << antigen_data.empirical << "\": false,\n";
                output << "    \"" << tag_prefix << "serum_circle_" << serum_name_infix << ".theoretical.all\": [\n";
                output << mod_pre;
                output << "        {\"N\": \"serum_circle\", \"type\": \"theoretical\", \"serum\": {\"index\": " << serum_data.serum_no
                       << "}, \"?antigen\": {\"index\": 0}, \"report\": true, \"circle\": {\"fill\": \"#C0FF8080\", \"outline\": \"red\", \"outline_width\": 2}}\n";
                output << "    ],\n";

                // output << "    \"" << tag_prefix << "serum_circle_" << serum_name_infix << ".theoretical.12m\": [\n";
                // output << "    ],\n";
                // output << "    \"" << tag_prefix << "serum_circle_" << serum_name_infix << ".empirical.all\": [\n";
                // output << "    ],\n";
                // output << "    \"" << tag_prefix << "serum_circle_" << serum_name_infix << ".empirical.12m\": [\n";
                // output << "    ],\n";
                // output << "    \"" << tag_prefix << "serum_coverage_circle_" << serum_name_infix << ".theoretical.all\": [\n";
                // output << "    ],\n";
                // output << "    \"" << tag_prefix << "serum_coverage_circle_" << serum_name_infix << ".theoretical.12m\": [\n";
                // output << "    ],\n";
                // output << "    \"" << tag_prefix << "serum_coverage_circle_" << serum_name_infix << ".empirical.all\": [\n";
                // output << "    ],\n";
                // output << "    \"" << tag_prefix << "serum_coverage_circle_" << serum_name_infix << ".empirical.12m\": [\n";
                // output << "    ],\n";

                output << '\n';
            }
        }
        else {
            output << "    \"?? ** no antigens ** no serum circle ** " << tag_prefix << " " << serum_data.serum->full_name_with_passage() << "\": false,\n";
        }
        output << "\n    \"? ======================================================================\": false,\n";

    }
    output << "    \"? avoid last comma\": []\n";
    output << "  }\n}\n";

} // report_json

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
