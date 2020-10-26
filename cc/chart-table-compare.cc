#include "acmacs-base/argv.hh"
// #include "acmacs-base/fmt.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

constexpr const size_t None{static_cast<size_t>(-1)};

class AgSrRefs : public std::vector<size_t>
{
public:
    using std::vector<size_t>::vector;

    auto num_tables() const
    {
        return std::count_if(begin(), end(), [](size_t no) { return no != None; });
    }
};

struct TiterPerTable
{
    std::string antigen;
    std::string serum;
    std::vector<acmacs::chart::Titer> titer_per_table;

    auto num_tables() const
    {
        return ranges::count_if(titer_per_table, [](const auto& titer) { return !titer.is_dont_care(); });
    }

    auto mean_logged_titer() const
    {
        return ranges::accumulate(
            titer_per_table
            | ranges::views::filter([](const auto& titer) { return !titer.is_dont_care(); }),
            0.0, [](double sum, const auto& titer) { return sum + titer.logged_with_thresholded(); })
            / static_cast<double>(num_tables());

        // size_t nt { 0 };
        // double sum { 0 };
        // for (const auto& titer : titer_per_table) {

        // }
    }
};

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

        // remove antigens found in just one table
        for (auto it = antigens.begin(); it != antigens.end();) {
            if (it->second.num_tables() < 2)
                it = antigens.erase(it);
            else
                ++it;
        }

        constexpr const auto sqr = [](double value) { return value * value; };

        for (auto [serum_name, serum_tables] : sera) {
            if (serum_tables.num_tables() > 1) {
                for (auto [antigen_name, antigen_tables] : antigens) {
                    TiterPerTable titers{antigen_name, serum_name, std::vector<acmacs::chart::Titer>(num_tables)};
                    for (size_t t_no = 0; t_no < num_tables; ++t_no) {
                        if (serum_tables[t_no] != None && antigen_tables[t_no] != None)
                            titers.titer_per_table[t_no] = charts[t_no]->titers()->titer(antigen_tables[t_no], serum_tables[t_no]);
                    }
                    if (titers.num_tables() > 1) {
                        const auto mean = titers.mean_logged_titer();
                        fmt::print("{}\n{}\n", antigen_name, serum_name);
                        for (size_t t_no = 0; t_no < num_tables; ++t_no) {
                            if (const auto& titer = titers.titer_per_table[t_no]; !titer.is_dont_care())
                                fmt::print(" {} {:>7s}  {:.4f}\n", charts[t_no]->info()->date(), titer, sqr(titer.logged_with_thresholded() - mean));
                            else
                                fmt::print(" {}\n", charts[t_no]->info()->date());
                        }
                        fmt::print("          {}\n\n", mean);
                    }
                }
            }
        }

        // AD_DEBUG("Charts: {}", num_tables);
        // AD_DEBUG("Antigens: {}", antigens.size());
        // AD_DEBUG("Sera: {}", sera.size());
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
