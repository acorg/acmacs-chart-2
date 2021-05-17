#include "acmacs-chart-2/titer-data.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

void acmacs::chart::TiterData::add(const Chart& chart)
{
    const auto t_no = add_table(chart);

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
