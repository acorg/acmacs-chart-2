#include "acmacs-chart-2/titer-data.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

void acmacs::chart::TiterData::add(const Chart& chart)
{
    const auto t_no = add_table(chart);
    for (size_t antigen_no = 0; antigen_no < chart.antigens()->size(); ++antigen_no) {
        for (size_t serum_no = 0; serum_no < chart.sera()->size(); ++serum_no) {
            auto [it, added] = titers_.try_emplace(ASName{chart.antigens()->at(antigen_no)->name_full(), chart.sera()->at(serum_no)->name_full()}, Titers(t_no + 1));
            it->second.resize(t_no + 1);
            it->second[t_no] = chart.titers()->titer(antigen_no, serum_no);
        }
    }

} // acmacs::chart::TiterData::add

// ----------------------------------------------------------------------

size_t acmacs::chart::TiterData::add_table(const Chart& chart)
{
    table_names_.push_back(chart.info()->make_name());
    return table_names_.size() - 1;

} // acmacs::chart::TiterData::add_table

// ----------------------------------------------------------------------

void acmacs::chart::TiterData::collect_titer_set()
{
    for (const auto& [as_name, titers] : titers_) {
        for (const auto& titer : titers) {
            if (!titer.is_dont_care())
                all_titers_.insert(titer);
        }
    }

} // acmacs::chart::TiterData::collect_titer_set

// ----------------------------------------------------------------------

template <typename Extractor> static inline std::vector<std::string> antigens_sera(const auto& titers, size_t min_tables, Extractor extractor)
{
    auto list = titers                                                                                                         //
                | ranges::views::filter([min_tables](const auto& en) { return en.second.count_non_dontcare() >= min_tables; }) //
                | ranges::views::transform(extractor)                                                                          //
                | ranges::to<std::vector>;
    return std::move(list) | ranges::actions::sort // action requires std::move()
           | ranges::actions::unique;              //
}

std::vector<std::string> acmacs::chart::TiterData::antigens(size_t min_tables) const
{
    return antigens_sera(titers_, min_tables, [](const auto& en) { return en.first.antigen; });

} // acmacs::chart::TiterData::all_antigens

// ----------------------------------------------------------------------

std::vector<std::string> acmacs::chart::TiterData::sera(size_t min_tables) const
{
    return antigens_sera(titers_, min_tables, [](const auto& en) { return en.first.serum; });

} // acmacs::chart::TiterData::all_sera

// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
