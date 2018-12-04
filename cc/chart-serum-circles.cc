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

static inline std::string make_infix(size_t no, std::string source)
{
    return std::to_string(no) + "_" + string::replace(source, "/", "_", " (", "_", " ", "_", "A(H1N1)", "H1", "A(H3N2)", "H3", ",", "_", "(", "", ")", "", "?", "X");
}

struct AntigenData
{
    size_t antigen_no;
    acmacs::chart::AntigenP antigen;
    std::string infix;
    acmacs::chart::Titer titer;
    double theoretical = -1;
    double empirical = -1;

    AntigenData(size_t ag_no, acmacs::chart::AntigenP ag, const acmacs::chart::Titer& a_titer)
        : antigen_no{ag_no}, antigen{ag}, infix(make_infix(ag_no, ag->full_name_with_passage())), titer{a_titer} {}
    constexpr bool valid_theoretical() const { return theoretical > 0; }
    constexpr bool valid_empirical() const { return empirical > 0; }
};

struct SerumData
{
    size_t serum_no;
    acmacs::chart::SerumP serum;
    std::string infix;
    std::vector<AntigenData> antigens;

    SerumData(size_t sr_no, acmacs::chart::SerumP sr)
        : serum_no{sr_no}, serum{sr}, infix(make_infix(sr_no, sr->full_name_without_passage())) {}
    bool valid() const { return !antigens.empty(); }
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
    chart->set_homologous(acmacs::chart::find_homologous::for_serum_circles);
    auto result = collect(*chart, projection_no);

    if (std::string json_filename(args["--json"]); json_filename.empty())
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

enum class mod_type { circle, coverage_circle };

template <mod_type> std::string mod_type_name();
template <> inline std::string mod_type_name<mod_type::circle>() { return "serum_circle"; }
template <> inline std::string mod_type_name<mod_type::coverage_circle>() { return "serum_coverage_circle"; }

enum class radius_type { theoretical, empirical };

template <radius_type> bool validate(const AntigenData& antigen_data);
template <> inline bool validate<radius_type::theoretical>(const AntigenData& antigen_data) { return antigen_data.valid_theoretical(); }
template <> inline bool validate<radius_type::empirical>(const AntigenData& antigen_data) { return antigen_data.valid_empirical(); }

template <radius_type> const char* radius_infix();
template <> inline const char* radius_infix<radius_type::theoretical>() { return "theoretical"; }
template <> inline const char* radius_infix<radius_type::empirical>() { return "empirical"; }

enum class time_type { all, m12 };
template <time_type> const char* time_infix();
template <> inline const char* time_infix<time_type::all>() { return "all"; }
template <> inline const char* time_infix<time_type::m12>() { return "12m"; }

template <mod_type mt, radius_type rt, time_type tt> inline void make_tag(std::ostream& output, std::string /*lab_assay_tag*/, std::string serum_tag, std::string antigen_tag)
{
      // output << lab_assay_tag << mod_type_name<mt>() << '.' << serum_tag << '.' << antigen_tag << '.' << radius_infix<rt>() << '.' << time_infix<tt>();
    output << mod_type_name<mt>() << '.' << serum_tag << '.' << antigen_tag << '.' << radius_infix<rt>() << '.' << time_infix<tt>();
}

// ----------------------------------------------------------------------

template <mod_type mt, radius_type rt, time_type tt> void make_list_2(std::ostream& output, const std::vector<SerumData>& sera_data, std::string lab_assay_tag)
{
      // output << "    \"" << lab_assay_tag << mod_type_name<mt>() << '.' << radius_infix<rt>() << '.' << time_infix<tt>() << "\": [\n";
    output << "    \"" << radius_infix<rt>() << '.' << time_infix<tt>() << "\": [\n";
    for (const auto& serum_data : sera_data) {
        bool commented = false;
        for (const auto& antigen_data : serum_data.antigens) {
            if (validate<rt>(antigen_data)) {
                output << "      ";
                if (commented)
                    output << "  \"? ";
                else
                    output << '"';
                make_tag<mt, rt, tt>(output, lab_assay_tag, serum_data.infix, antigen_data.infix);
                output << "\",\n";
                  // commented = true;
            }
        }
    }
    output << "        \"?? no comma\"\n";
    output << "    ],\n\n";
}

template <typename ... Args> void make_list(mod_type mt, radius_type rt, time_type tt, Args&& ... args)
{
    switch (mt) {
      case mod_type::circle:
          switch (rt) {
            case radius_type::theoretical:
                switch (tt) {
                  case time_type::all:
                      make_list_2<mod_type::circle, radius_type::theoretical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_list_2<mod_type::circle, radius_type::theoretical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
            case radius_type::empirical:
                switch (tt) {
                  case time_type::all:
                      make_list_2<mod_type::circle, radius_type::empirical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_list_2<mod_type::circle, radius_type::empirical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
          }
          break;
      case mod_type::coverage_circle:
          switch (rt) {
            case radius_type::theoretical:
                switch (tt) {
                  case time_type::all:
                      make_list_2<mod_type::coverage_circle, radius_type::theoretical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_list_2<mod_type::coverage_circle, radius_type::theoretical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
            case radius_type::empirical:
                switch (tt) {
                  case time_type::all:
                      make_list_2<mod_type::coverage_circle, radius_type::empirical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_list_2<mod_type::coverage_circle, radius_type::empirical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
          }
          break;
    }
}

// ----------------------------------------------------------------------

inline void make_serum_info(std::ostream& output, const SerumData& serum_data)
{
    output << "    \"?? ==== " << serum_data.infix << " ======================================================================\": false,\n"
           << "    \"?? SR " << serum_data.serum_no << ' ' << serum_data.serum->full_name_with_passage() << "\": false,\n";
    if (!serum_data.antigens.empty()) {
        for (const auto& antigen_data : serum_data.antigens) {
            output << "    \"??   titer:" << std::setw(5) << std::right << antigen_data.titer << " theor:";
            if (antigen_data.valid_theoretical())
                output << std::setw(4) << std::fixed << std::setprecision(2) << std::left << antigen_data.theoretical;
            else
                output << "?   ";
            output << " empir:";
            if (antigen_data.valid_empirical())
                output << std::setw(4) << std::fixed << std::setprecision(2) << std::left << antigen_data.empirical;
            else
                output << "?   ";
            output << " AG " << std::setw(4) << std::right << antigen_data.antigen_no << ' ' << antigen_data.antigen->full_name_with_passage() << "\": false,\n";
        }
    }
    else {
        output << "    \"?? ************ NO ANTIGENS \": false,\n";
    }
    output << '\n';
}

// ----------------------------------------------------------------------

template <mod_type mt, radius_type rt, std::enable_if_t<mt==mod_type::circle, int> = 0> void make_radius_type(std::ostream& output, std::string prefix)
{
    output << prefix << "\"type\": \"" << radius_infix<rt>() << "\",\n";
}

template <mod_type mt, radius_type rt, std::enable_if_t<mt==mod_type::coverage_circle, int> = 0> void make_radius_type(std::ostream& output, std::string prefix)
{
    output << prefix << '"' << radius_infix<rt>() << R"(": {"show": true, "fill": "#C0FF8080", "outline": "red", "outline_width": 2, "?angle_degrees": [0, 30], "radius_line_dash": "dash2", "?radius_line_color": "red", "?radius_line_width": 1},)" << '\n';
}

// ----------------------------------------------------------------------

template <mod_type mt, std::enable_if_t<mt==mod_type::circle, int> = 0> void make_within_outside(std::ostream& /*output*/, std::string /*prefix*/)
{
}

template <mod_type mt, std::enable_if_t<mt==mod_type::coverage_circle, int> = 0> void make_within_outside(std::ostream& output, std::string prefix)
{
    output << prefix << R"("within_4fold": {"fill_saturation": 10.0, "outline": "pink", "outline_width": 3, "order": "raise"},)" << '\n'
           << prefix << R"("outside_4fold": {"fill_saturation": 10.0, "outline": "black", "order": "raise"},)" << '\n';
}

// ----------------------------------------------------------------------

template <mod_type mt> void mark_serum(std::ostream& output, std::string prefix)
{
    output << prefix << R"("mark_serum": {"fill": "black", "outline": "black", "size": 20, "order": "raise", "?label": {"name_type": "full", "offset": [0, 1.2], "color": "black", "size": 12, "weight": "bold"}})" << '\n';
}

template <mod_type mt, std::enable_if_t<mt==mod_type::circle, int> = 0> void mark_antigen(std::ostream& output, std::string prefix)
{
    output << prefix << R"("mark_antigen": {"fill": "lightblue", "outline": "black", "order": "raise", "label": {"name_type": "full", "offset": [0, 1.2], "color": "black", "size": 12}},)" << '\n';
}

template <mod_type mt, std::enable_if_t<mt==mod_type::coverage_circle, int> = 0> void mark_antigen(std::ostream& /*output*/, std::string /*prefix*/)
{
}

// ----------------------------------------------------------------------

// time_type::all
template <mod_type mt, radius_type rt, time_type tt, std::enable_if_t<tt==time_type::all, int> = 0>
    void make_antigen_mod_3(std::ostream& output, const SerumData& serum_data, const AntigenData& antigen_data, std::string /*lab_assay_tag*/)
{
    output << "        {\"serum_name\": \"" << serum_data.serum->full_name_with_passage() << "\", \"serum_no\": " << serum_data.serum_no
           << ", \"antigen_name\": \"" << antigen_data.antigen->full_name_with_passage() << "\", \"antigen_no\": " << antigen_data.antigen_no
           << ", \"titer\": \"" << antigen_data.titer
           << "\", \"theoretical\": " << std::setprecision(2) << std::fixed << antigen_data.theoretical
           << ", \"empirical\": " << std::setprecision(2) << std::fixed << antigen_data.empirical
           << ", \"N\": \"comment\", \"type\": \"data\"},\n";
    output << "        {\"N\": \"" << mod_type_name<mt>()
           << "\", \"serum\": {\"index\": " << serum_data.serum_no
           << "}, \"antigen\": {\"index\": " << antigen_data.antigen_no << "},\n";
    make_radius_type<mt, rt>(output, "          ");
    make_within_outside<mt>(output, "          ");
    mark_antigen<mt>(output, "          ");
    mark_serum<mt>(output, "          ");
    output << "        }\n";
}

// time_type::m12
template <mod_type mt, radius_type rt, time_type tt, std::enable_if_t<tt==time_type::m12, int> = 0>
    void make_antigen_mod_3(std::ostream& output, const SerumData& serum_data, const AntigenData& antigen_data, std::string lab_assay_tag)
{
    output << "        \"";
    make_tag<mt, rt, time_type::all>(output, lab_assay_tag, serum_data.infix, antigen_data.infix);
    output << "\",\n";
    output << "        {\"N\": \"antigens\", \"select\": {\"older_than_days\": 365}, \"order\": \"lower\", \"fill\": \"grey80\", \"outline\": \"grey80\"}\n";
}

// ----------------------------------------------------------------------

template <mod_type mt, radius_type rt, time_type tt>
    void make_antigen_mod_2(std::ostream& output, const SerumData& serum_data, const AntigenData& antigen_data, std::string lab_assay_tag)
{
    if (validate<rt>(antigen_data)) {
        output << "      \"";
        make_tag<mt, rt, tt>(output, lab_assay_tag, serum_data.infix, antigen_data.infix);
        output << "\": [\n";
        make_antigen_mod_3<mt, rt, tt>(output, serum_data, antigen_data, lab_assay_tag);
        output << "      ],\n";
    }
}

// ----------------------------------------------------------------------

template <typename ... Args> void make_antigen_mod(mod_type mt, radius_type rt, time_type tt, Args&& ... args)
{
    switch (mt) {
      case mod_type::circle:
          switch (rt) {
            case radius_type::theoretical:
                switch (tt) {
                  case time_type::all:
                      make_antigen_mod_2<mod_type::circle, radius_type::theoretical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_antigen_mod_2<mod_type::circle, radius_type::theoretical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
            case radius_type::empirical:
                switch (tt) {
                  case time_type::all:
                      make_antigen_mod_2<mod_type::circle, radius_type::empirical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_antigen_mod_2<mod_type::circle, radius_type::empirical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
          }
          break;
      case mod_type::coverage_circle:
          switch (rt) {
            case radius_type::theoretical:
                switch (tt) {
                  case time_type::all:
                      make_antigen_mod_2<mod_type::coverage_circle, radius_type::theoretical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_antigen_mod_2<mod_type::coverage_circle, radius_type::theoretical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
            case radius_type::empirical:
                switch (tt) {
                  case time_type::all:
                      make_antigen_mod_2<mod_type::coverage_circle, radius_type::empirical, time_type::all>(std::forward<Args>(args) ...);
                      break;
                  case time_type::m12:
                      make_antigen_mod_2<mod_type::coverage_circle, radius_type::empirical, time_type::m12>(std::forward<Args>(args) ...);
                      break;
                }
                break;
          }
          break;
    }
}

// ----------------------------------------------------------------------

void report_json(std::ostream& output, const acmacs::chart::Chart& chart, const std::vector<SerumData>& sera_data)
{
      // const auto assay_tag = string::replace(chart.info()->assay(), "FOCUS REDUCTION", "FRA", " ", "_");
    const std::string assay_tag = chart.info()->assay() == "HI" ? "HI" : "NEUT";
    const auto lab = chart.info()->lab(acmacs::chart::Info::Compute::Yes);
    const auto lab_assay_tag = lab + '_' + assay_tag + '.';

    // const char* mod_pre = "        {\"N\": \"clades_light\", \"size\": 8},\n";

    output << "{ \"_\":\"-*- js-indent-level: 2 -*-\",\n\n";
    output << "  \"?? " << lab << ' ' << assay_tag << "\": false,\n\n";

    output << "  \"?? ==== COVERAGE LIST ======================================================================\": false,\n";
    output << "  \"serum_coverage_mods\": {\n\n";
    for (auto rt : {radius_type::theoretical, radius_type::empirical})
        for (auto tt : {time_type::all, time_type::m12})
            make_list(mod_type::coverage_circle, rt, tt, output, sera_data, lab_assay_tag);
    output << "    \"?? no comma\": false\n";
    output << "  },\n\n";

    output << "  \"?? ==== CIRCLE LIST ======================================================================\": false,\n";
    output << "  \"serum_circle_mods\": {\n\n";
    for (auto rt : {radius_type::theoretical, radius_type::empirical})
        for (auto tt : {time_type::all, time_type::m12})
            make_list(mod_type::circle, rt, tt, output, sera_data, lab_assay_tag);
    output << "    \"?? no comma\": false\n";
    output << "  },\n\n";

    output << "  \"mods\": {\n\n";
    for (const auto& serum_data : sera_data) {
        make_serum_info(output, serum_data);
        for (const auto& antigen_data : serum_data.antigens) {
            for (auto rt : {radius_type::theoretical, radius_type::empirical})
                for (auto mt : {mod_type::coverage_circle, mod_type::circle})
                    for (auto tt : {time_type::all, time_type::m12})
                        make_antigen_mod(mt, rt, tt, output, serum_data, antigen_data, lab_assay_tag);
            output << '\n';
        }
    }

    output << "    \"?? no comma\": false\n";
    output << "  }\n}\n";

} // report_json

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
