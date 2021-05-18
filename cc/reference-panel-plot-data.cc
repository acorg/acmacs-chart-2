#include <limits>
#include "acmacs-chart-2/reference-panel-plot-data.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

void acmacs::chart::ReferencePanelPlotData::add(const Chart& chart)
{
    const auto t_no = add_table(chart);
    for (size_t antigen_no = 0; antigen_no < chart.antigens()->size(); ++antigen_no) {
        for (size_t serum_no = 0; serum_no < chart.sera()->size(); ++serum_no) {
            auto [it, added] = titers_.try_emplace(ASName{chart.antigens()->at(antigen_no)->name_full(), chart.sera()->at(serum_no)->name_full()}, Titers(t_no + 1));
            it->second.resize(t_no + 1);
            it->second[t_no] = chart.titers()->titer(antigen_no, serum_no);
        }
    }

} // acmacs::chart::ReferencePanelPlotData::add

// ----------------------------------------------------------------------

size_t acmacs::chart::ReferencePanelPlotData::add_table(const Chart& chart)
{
    table_names_.push_back(chart.info()->make_name());
    return table_names_.size() - 1;

} // acmacs::chart::ReferencePanelPlotData::add_table

// ----------------------------------------------------------------------

void acmacs::chart::ReferencePanelPlotData::collect_titer_set()
{
    for (const auto& [as_name, titers] : titers_) {
        for (const auto& titer : titers) {
            if (!titer.is_dont_care())
                all_titers_.insert(titer);
        }
    }

} // acmacs::chart::ReferencePanelPlotData::collect_titer_set

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

std::vector<std::string> acmacs::chart::ReferencePanelPlotData::antigens(size_t min_tables) const
{
    return antigens_sera(titers_, min_tables, [](const auto& en) { return en.first.antigen; });

} // acmacs::chart::ReferencePanelPlotData::all_antigens

// ----------------------------------------------------------------------

std::vector<std::string> acmacs::chart::ReferencePanelPlotData::sera(size_t min_tables) const
{
    return antigens_sera(titers_, min_tables, [](const auto& en) { return en.first.serum; });

} // acmacs::chart::ReferencePanelPlotData::all_sera

// ----------------------------------------------------------------------

acmacs::chart::ReferencePanelPlotData::ASTable acmacs::chart::ReferencePanelPlotData::make_antigen_serum_table(const std::vector<std::string>& antigens, const std::vector<std::string>& sera) const
{
    const auto cell_no = [&antigens, &sera](const ASName& as_pair) {
        std::pair<ssize_t, ssize_t> cn { -1, -1 };
        if (const auto it = std::find(std::begin(antigens), std::end(antigens), as_pair.antigen); it != std::end(antigens))
            cn.first = it - std::begin(antigens);
        if (const auto it = std::find(std::begin(sera), std::end(sera), as_pair.serum); it != std::end(sera))
            cn.second = it - std::begin(sera);
        return cn;
    };

    ASTable table(antigens.size(), std::vector<AntigenSerumData>(sera.size()));
    size_t min_table_index{std::numeric_limits<size_t>::max()}, max_table_index{0};
    for (const auto& [as_name, titers] : titers_) {
        if (const auto cell = cell_no(as_name); cell.first >= 0 && cell.second >= 0) {
            table[static_cast<size_t>(cell.first)][static_cast<size_t>(cell.second)].titers = titers;
            min_table_index = std::min(min_table_index, titers.first_non_dontcare_index());
            max_table_index = std::max(max_table_index, titers.last_non_dontcare_index());
        }
    }

    // remove titers in each cell of the table that are not in the inclusive range [min_table_index, max_table_index]
    // calculate median
    for (auto& row : table) {
        for (auto& cell : row) {
            if (cell.titers.size() <= max_table_index)
                cell.titers.resize(max_table_index + 1);
            else
                cell.titers.erase(std::next(cell.titers.begin(), static_cast<ssize_t>(max_table_index) + 1), cell.titers.end());
            cell.titers.erase(cell.titers.begin(), std::next(cell.titers.begin(), static_cast<ssize_t>(min_table_index)));
            cell.find_median();
        }
    }

    return table;

} // acmacs::chart::ReferencePanelPlotData::make_antigen_serum_table

// ----------------------------------------------------------------------

void acmacs::chart::ReferencePanelPlotData::AntigenSerumData::find_median()
{
    auto good_titers = titers | ranges::views::filter(is_dont_care) | ranges::to_vector;
    ranges::actions::sort(good_titers);
    if (good_titers.size() % 2)
        median_titer = good_titers[good_titers.size() / 2];
    else
        median_titer = good_titers[good_titers.size() / 2 - 1];

} // acmacs::chart::ReferencePanelPlotData::AntigenSerumData::find_median

// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
