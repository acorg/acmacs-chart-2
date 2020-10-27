// #include <compare>
// #include <concepts>

#include "acmacs-base/argv.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

// template <typename T> requires requires (const T& a, const T& b)
// {
//     a.compare(b);
//     // {
//     //     a.compare(b)
//     //         } -> concepts::convertible_to<int>;
// }
// inline std::strong_ordering operator<=>(const T& lhs, const T& rhs)
// {
//     if (const auto res = lhs.compare(rhs); res == 0)
//         return std::strong_ordering::equal;
//     else if (res < 0)
//         return std::strong_ordering::less;
//     else
//         return std::strong_ordering::greater;
// }


struct TiterRef
{
    std::string serum;
    std::string antigen;
    size_t table_no;
    acmacs::chart::Titer titer;

    bool operator<(const TiterRef& rhs) const
    {
        if (const auto r1 = serum.compare(rhs.serum); r1 != 0)
            return r1 < 0;
        if (const auto r1 = antigen.compare(rhs.antigen); r1 != 0)
            return r1 < 0;
        return table_no < rhs.table_no;
    }

    // std::strong_ordering operator<=>(const TiterRef&) const = default;
};

struct TiterRefCollapsed
{
    std::string serum;
    std::string antigen;
    std::vector<acmacs::chart::Titer> titers;

    TiterRefCollapsed(const std::string& a_serum, const std::string& a_antigen, size_t num_tables) : serum{a_serum}, antigen{a_antigen}, titers(num_tables) {}

    static inline bool valid(const acmacs::chart::Titer& titer) { return !titer.is_dont_care(); }

    auto num_tables() const
    {
        return ranges::count_if(titers, valid);
    }

    auto mean_logged_titer() const
    {
        return ranges::accumulate(titers | ranges::views::filter(valid), 0.0, [](double sum, const auto& titer) { return sum + titer.logged_with_thresholded(); }) / static_cast<double>(num_tables());
    }

    bool eq(const TiterRef& raw) const { return serum == raw.serum && antigen == raw.antigen; }
};

class ChartData
{
  public:
    void scan(const acmacs::chart::Chart& chart)
    {
        const auto table_no = tables_.size();
        tables_.push_back(chart.info()->date());
        auto chart_antigens = chart.antigens();
        auto chart_sera = chart.sera();
        auto chart_titers = chart.titers();
        for (auto [ag_no, ag] : acmacs::enumerate(*chart_antigens)) {
            for (auto [sr_no, sr] : acmacs::enumerate(*chart_sera)) {
                if (const auto& titer = chart_titers->titer(ag_no, sr_no); !titer.is_dont_care())
                    raw_.push_back(TiterRef{.serum = sr->full_name(), .antigen = ag->full_name(), .table_no = table_no, .titer = titer});
            }
        }
    }

    void collapse()
    {
        ranges::sort(raw_, &TiterRef::operator<);
        for (const auto& raw : raw_) {
            if (collapsed_.empty())
                collapsed_.emplace_back(raw.serum, raw.antigen, tables_.size());
            else if (!collapsed_.back().eq(raw)) {
                if (collapsed_.back().num_tables() < 2)
                    collapsed_.pop_back();
                collapsed_.emplace_back(raw.serum, raw.antigen, tables_.size());
            }
            collapsed_.back().titers[raw.table_no] = raw.titer;
        }
    }

    void report_deviation_from_mean() const
        {
            for (const auto& en : collapsed_) {
                fmt::print("{}\n{}\n", en.serum, en.antigen);
                const auto mean = en.mean_logged_titer();
                for (size_t t_no = 0; t_no < tables_.size(); ++t_no) {
                    if (!en.titers[t_no].is_dont_care())
                        fmt::print(" {}  {:>7s}  {:.2f}\n", tables_[t_no], en.titers[t_no], std::abs(en.titers[t_no].logged_with_thresholded() - mean));
                    else
                        fmt::print(" {}\n", tables_[t_no]);
                }
                fmt::print("            {:.2f}\n\n", mean);
            }
        }

        void report_average_deviation_from_mean_per_table() const
        {
            std::vector<std::pair<double, size_t>> deviations(tables_.size(), {0.0, 0});
            for (const auto& en : collapsed_) {
                const auto mean = en.mean_logged_titer();
                ranges::for_each(ranges::views::iota(0ul, deviations.size()) //
                                     | ranges::views::filter([&en](size_t t_no) { return !en.titers[t_no].is_dont_care(); }),
                                 [&en, &deviations, mean](size_t t_no) {
                                     deviations[t_no].first += std::abs(en.titers[t_no].logged_with_thresholded() - mean);
                                     ++deviations[t_no].second;
                                 });
            }
            for (size_t t_no = 0; t_no < tables_.size(); ++t_no)
                fmt::print("{}  {:.2f}  {:4d}\n", tables_[t_no], deviations[t_no].first / static_cast<double>(deviations[t_no].second), deviations[t_no].second);
        }

      private:
        std::vector<acmacs::chart::TableDate> tables_;
        std::vector<TiterRef> raw_;
        std::vector<TiterRefCollapsed> collapsed_;
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

        ChartData data;
        for (const auto& fn : opt.charts)
            data.scan(*acmacs::chart::import_from_file(fn));
        data.collapse();
        // data.report_deviation_from_mean();
        data.report_average_deviation_from_mean_per_table();

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
