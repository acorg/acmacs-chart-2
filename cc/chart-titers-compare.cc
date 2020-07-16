#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

class Titers
{
  public:
    Titers() = default;

    void add_table(std::string_view table) { all_tables_.push_back(table); }

    void add(std::string_view antigen, std::string_view serum, std::string_view serum_abbreviated, const acmacs::chart::Titer& titer)
    {
        data_.emplace_back(all_tables_.size() - 1, antigen, serum, serum_abbreviated, titer);
    }

    void collect()
    {
        for (const auto& en : data_) {
            all_antigens_.emplace_back(en.antigen);
            all_sera_.emplace_back(en.serum, en.serum_abbreviated);
            num_tables_ = std::max(num_tables_, en.table_no);
            max_antigen_name_ = std::max(max_antigen_name_, en.antigen.size());
        }
        ++num_tables_;
        std::sort(std::begin(all_antigens_), std::end(all_antigens_));
        all_antigens_.erase(std::unique(std::begin(all_antigens_), std::end(all_antigens_)), std::end(all_antigens_));
        std::sort(std::begin(all_sera_), std::end(all_sera_), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
        all_sera_.erase(std::unique(std::begin(all_sera_), std::end(all_sera_), [](const auto& e1, const auto& e2) { return e1.first == e2.first; }), std::end(all_sera_));
    }

    size_t serum_index(std::string_view serum) const
    {
            return static_cast<size_t>(std::find_if(std::begin(all_sera_), std::end(all_sera_), [serum](const auto& en) { return en.first == serum; }) - std::begin(all_sera_));
    }

    void report(bool omit_if_not_in_first) const
    {
        fmt::memory_buffer result;
        const auto column_width = 8;
        const auto table_prefix = 5;
        const auto table_no_width = 4;

        for (const auto [table_no, table] : acmacs::enumerate(all_tables_))
            fmt::print("{:3d} {}\n", table_no, table);

        fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name_ + table_prefix + table_no_width + 5);
        for (auto serum_no : acmacs::range(all_sera_.size()))
            fmt::format_to(result, "{: ^{}d}", serum_no + 1, column_width);
        fmt::format_to(result, "\n");
        fmt::format_to(result, "{: >{}s}  ", "", max_antigen_name_ + table_prefix + table_no_width + 5);
        for (auto serum : all_sera_)
            fmt::format_to(result, "{: ^8s}", serum.second, column_width);
        fmt::format_to(result, "\n\n");


        for (const auto [ag_no, antigen] : acmacs::enumerate(all_antigens_)) {
            std::vector<std::vector<const acmacs::chart::Titer*>> per_table_per_serum(num_tables_, std::vector<const acmacs::chart::Titer*>(all_sera_.size(), nullptr));
            for (size_t table_no{0}; table_no < num_tables_; ++table_no) {
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
            size_t present_in_tables{0};
            for (size_t table_no{0}; table_no < num_tables_; ++table_no) {

                if (present_in_table(table_no)) {
                    ++present_in_tables;
                    if (first_table) {
                        fmt::format_to(result, "{:3d}  {: <{}s} ", ag_no + 1, antigen, max_antigen_name_);
                        fmt::format_to(result, "{:{}d}{:<2s} ", table_no + 1, table_no_width, "");
                    }
                    else {
                        bool matches{true};
                        for (const auto [sr_no, titer] : acmacs::enumerate(per_table_per_serum[table_no]))
                            matches &= !titer || !per_table_per_serum[0][sr_no] || *titer == *per_table_per_serum[0][sr_no];
                        fmt::format_to(result, "{: >{}s} ", "", max_antigen_name_ + table_prefix);
                        fmt::format_to(result, "{:{}d}{:<2s} ", table_no + 1, table_no_width, matches ? "^" : "**");
                    }

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
            if (present_in_tables < 2 && present_in_table(0))
                fmt::format_to(result, "!!!!\n");
            fmt::format_to(result, "\n\n");
        }

        fmt::format_to(result, "\n");
        for (auto [sr_no, serum] : acmacs::enumerate(all_sera_, 1ul))
            fmt::format_to(result, "{: >{}s} {:3d} {}\n", "", max_antigen_name_ + table_prefix + table_no_width, sr_no, serum.first);

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

    std::vector<std::string_view> all_tables_;
    std::vector<std::string_view> all_antigens_;
    std::vector<std::pair<std::string_view, std::string_view>> all_sera_;
    size_t num_tables_{0};
    size_t max_antigen_name_{0};
    std::vector<std::vector<const acmacs::chart::Titer*>> all_titers; // [antigen_no][table_no][serum_no]
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
        for (const auto& chart_filename : *opt.charts) {
            titer_data.add_table(chart_filename);
            auto chart = acmacs::chart::import_from_file(chart_filename);
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            auto titers = chart->titers();
            for (size_t ag_no{0}; ag_no < antigens->size(); ++ag_no) {
                for (size_t sr_no{0}; sr_no < sera->size(); ++sr_no) {
                    titer_data.add(antigens->at(ag_no)->full_name(), sera->at(sr_no)->full_name(), sera->at(sr_no)->abbreviated_location_year(), titers->titer(ag_no, sr_no));
                }
            }
        }
        titer_data.collect();
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
