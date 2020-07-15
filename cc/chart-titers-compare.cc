#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

class Titers
{
  public:
    Titers() = default;

    void add(size_t table_no, std::string_view antigen, std::string_view serum, std::string_view serum_abbreviated, const acmacs::chart::Titer& titer)
    {
        data_.emplace_back(table_no, antigen, serum, serum_abbreviated, titer);
    }

    void report(bool omit_if_not_in_first) const
    {
        std::vector<std::string_view> all_antigens;
        std::vector<std::pair<std::string_view, std::string_view>> all_sera;
        size_t max_table{0}, max_antigen_name{0};
        for (const auto& en : data_) {
            all_antigens.emplace_back(en.antigen);
            all_sera.emplace_back(en.serum, en.serum_abbreviated);
            max_table = std::max(max_table, en.table_no);
            max_antigen_name = std::max(max_antigen_name, en.antigen.size());
        }
        std::sort(std::begin(all_antigens), std::end(all_antigens));
        all_antigens.erase(std::unique(std::begin(all_antigens), std::end(all_antigens)), std::end(all_antigens));
        std::sort(std::begin(all_sera), std::end(all_sera), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
        all_sera.erase(std::unique(std::begin(all_sera), std::end(all_sera), [](const auto& e1, const auto& e2) { return e1.first == e2.first; }), std::end(all_sera));

        // const auto antigen_index = [&all_antigens](std::string_view antigen) { return std::find(std::begin(all_antigens), std::end(all_antigens), antigen) - std::begin(all_antigens); };
        const auto serum_index = [&all_sera](std::string_view serum) {
            return static_cast<size_t>(std::find_if(std::begin(all_sera), std::end(all_sera), [serum](const auto& en) { return en.first == serum; }) - std::begin(all_sera));
        };

        fmt::memory_buffer result;
        const auto column_width = 8;
        const auto table_prefix = 5;
        const auto table_no_width = 4;
        fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name + table_prefix + table_no_width + 3);
        for (auto serum_no : acmacs::range(all_sera.size()))
            fmt::format_to(result, "{: ^{}d}", serum_no + 1, column_width);
        fmt::format_to(result, "\n");
        fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name + table_prefix + table_no_width + 3);
        for (auto serum : all_sera)
            fmt::format_to(result, "{: ^8s}", serum.second, column_width);
        fmt::format_to(result, "\n\n");

        for (const auto [ag_no, antigen] : acmacs::enumerate(all_antigens)) {
            std::vector<std::vector<const acmacs::chart::Titer*>> per_table_per_serum(max_table + 1, std::vector<const acmacs::chart::Titer*>(all_sera.size(), nullptr));
            for (size_t table_no{0}; table_no <= max_table; ++table_no) {
                for (const auto& entry : data_) {
                    if (entry.table_no == table_no && entry.antigen == antigen)
                        per_table_per_serum[table_no][serum_index(entry.serum)] = &entry.titer;
                }
            }
            const auto present_in_table = [&per_table_per_serum](size_t table_no) {
                return std::any_of(std::begin(per_table_per_serum[table_no]), std::end(per_table_per_serum[table_no]), [](const auto* titer) { return bool{titer}; });
            };

            if (omit_if_not_in_first && !present_in_table(0))
                continue;

            bool first_table{true};
            for (size_t table_no{0}; table_no <= max_table; ++table_no) {

                if (present_in_table(table_no)) {
                    if (first_table)
                        fmt::format_to(result, "{:3d}  {: <{}s} ", ag_no + 1, antigen, max_antigen_name);
                    else
                        fmt::format_to(result, "{: >{}s} ", "", max_antigen_name + table_prefix);
                    fmt::format_to(result, "{:{}d} ", table_no + 1, table_no_width);

                    for (const auto [sr_no, titer] : acmacs::enumerate(per_table_per_serum[table_no])) {
                        if (titer) {
                            if (const auto* titer1 = per_table_per_serum[0][sr_no]; table_no == 0 || !titer1 || *titer1 != *titer)
                                fmt::format_to(result, "{: >{}s}", *titer, column_width);
                            else
                                fmt::format_to(result, "{: >{}s}", "^", column_width);
                        }
                        else
                            fmt::format_to(result, "{: >{}s}", "", column_width);
                    }
                    first_table = false;
                    fmt::format_to(result, "\n");
                }
            }
            fmt::format_to(result, "\n\n");
        }

        fmt::format_to(result, "\n");
        for (auto [sr_no, serum] : acmacs::enumerate(all_sera, 1ul))
            fmt::format_to(result, "{: >{}s} {:3d} {}\n", "", max_antigen_name + table_prefix + table_no_width, sr_no, serum.first);

        fmt::print("{}\n", fmt::to_string(result));
    }

  private:
    struct Entry
    {
        Entry(size_t a_table_no, std::string_view a_antigen, std::string_view a_serum, std::string_view a_serum_abbreviated, const acmacs::chart::Titer& a_titer)
            : table_no{a_table_no}, antigen{a_antigen}, serum{a_serum}, serum_abbreviated{a_serum_abbreviated}, titer{a_titer}
        {
        }
        size_t table_no;
        std::string antigen;
        std::string serum;
        std::string serum_abbreviated;
        acmacs::chart::Titer titer;
    };

    std::vector<Entry> data_;

};

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "compare titers of mutiple charts"; }
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};

    option<bool> omit_if_not_in_first{*this, "omit-not-first", desc{"omit antigen if it is not found in the first table"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        Titers titer_data;
        for (const auto [table_no, chart_filename] : acmacs::enumerate(*opt.charts)) {
            auto chart = acmacs::chart::import_from_file(chart_filename);
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            auto titers = chart->titers();
            for (size_t ag_no{0}; ag_no < antigens->size(); ++ag_no) {
                for (size_t sr_no{0}; sr_no < sera->size(); ++sr_no) {
                    titer_data.add(table_no, antigens->at(ag_no)->full_name(), sera->at(sr_no)->full_name(), sera->at(sr_no)->abbreviated_location_year(), titers->titer(ag_no, sr_no));
                }
            }
            fmt::print("{:3d} {}\n", table_no, chart_filename);
        }
        titer_data.report(opt.omit_if_not_in_first);
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
