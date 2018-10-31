#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2);

// ----------------------------------------------------------------------

acmacs::chart::MergeReport::MergeReport(const Chart& primary, const Chart& secondary, CommonAntigensSera::match_level_t a_match_level)
    : match_level{a_match_level}, common(primary, secondary, a_match_level)
{
} // acmacs::chart::MergeReport::MergeReport

// ----------------------------------------------------------------------

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    MergeReport report(chart1, chart2, match_level);

      // std::map<size_t, size_t> result_chart1, result_chart2;

    ChartModifyP result = std::make_shared<ChartNew>(chart1.number_of_antigens() + chart2.number_of_antigens() - report.common.common_antigens(), chart1.number_of_sera() + chart2.number_of_sera() - report.common.common_sera());
    merge_info(*result, chart1, chart2);

    auto antigens = result->antigens_modify();
    auto antigens1 = chart1.antigens();
    for (size_t ag_no = 0; ag_no < antigens1->size(); ++ag_no)
        antigens->at(ag_no).replace_with(antigens1->at(ag_no));

    auto sera = result->sera_modify();
    auto sera1 = chart1.sera();
    for (size_t sr_no = 0; sr_no < sera1->size(); ++sr_no)
        sera->at(sr_no).replace_with(sera1->at(sr_no));

    return {std::move(result), std::move(report)};

} // acmacs::chart::merge

// ----------------------------------------------------------------------

void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2)
{
    target.info_modify()->virus(chart1.info()->virus());
    if (chart1.info()->number_of_sources() == 0) {
        target.info_modify()->add_source(chart1.info());
    }
    else {
        for (size_t s_no = 0; s_no < chart1.info()->number_of_sources(); ++s_no)
            target.info_modify()->add_source(chart1.info()->source(s_no));
    }
    if (chart2.info()->number_of_sources() == 0) {
        target.info_modify()->add_source(chart2.info());
    }
    else {
        for (size_t s_no = 0; s_no < chart2.info()->number_of_sources(); ++s_no)
            target.info_modify()->add_source(chart2.info()->source(s_no));
    }

} // merge_info

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
