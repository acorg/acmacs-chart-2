#include "acmacs-chart-2/titer-data.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

void acmacs::chart::TiterData::add(const Chart& chart)
{
    const auto t_no = add_table(chart);
    for (size_t antigen_no = 0; antigen_no < chart.antigens()->size(); ++antigen_no) {
        for (size_t serum_no = 0; serum_no < chart.sera()->size(); ++serum_no) {
            auto [it, added] = titers_.try_emplace(ASName{chart.antigens()->at(antigen_no)->name_full(), chart.sera()->at(serum_no)->name_full()}, std::vector<Titer>{});
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



// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
