#include "acmacs-base/argv.hh"
// #include "acmacs-base/fmt.hh"
// #include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

constexpr const size_t None{static_cast<size_t>(-1)};

// struct AgSrRef
// {
//     AgSrRef(std::string&& a_name, size_t num_tables) : name{std::move(a_name)}, tables(num_tables, None) {}
//     bool operator<(const AgSrRef& rhs) const { return name < rhs.name; }

//     std::string name;
//     std::vector<size_t> tables;
// };

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "compare titers of mutiple tables"; }
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        std::vector<std::shared_ptr<acmacs::chart::Chart>> charts(opt.charts->size());
        std::transform(std::begin(opt.charts), std::end(opt.charts), std::begin(charts), [](const auto& fn) { return acmacs::chart::import_from_file(fn); });

        // std::for_each(std::begin(charts), std::end(charts), [](const auto& chart) { AD_DEBUG("{}", chart->info()->date()); });

        using AgSrRefs = std::vector<size_t>;
        std::map<std::string, AgSrRefs> antigens, sera;
        const auto num_tables = charts.size();
        for (auto [table_no, chart] : acmacs::enumerate(charts)) {
            auto chart_antigens = chart->antigens();
            for (auto [ag_no, ag] : acmacs::enumerate(*chart_antigens))
                antigens.emplace(ag->full_name(), AgSrRefs(num_tables, None)).first->second[table_no] = ag_no;
            auto chart_sera = chart->sera();
            for (auto [sr_no, sr] : acmacs::enumerate(*chart_sera))
                sera.emplace(sr->full_name(), AgSrRefs(num_tables, None)).first->second[table_no] = sr_no;
        }

        AD_DEBUG("Charts: {}", num_tables);
        AD_DEBUG("Antigens: {}", antigens.size());
        AD_DEBUG("Sera: {}", sera.size());
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
