#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

acmacs::chart::MergeReport::MergeReport(const Chart& primary, const Chart& secondary, CommonAntigensSera::match_level_t a_match_level)
    : match_level{a_match_level}, common(primary, secondary, a_match_level)
{
} // acmacs::chart::MergeReport::MergeReport

// ----------------------------------------------------------------------

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    MergeReport report(chart1, chart2, match_level);

    ChartModifyP result = std::make_shared<ChartNew>(chart1.number_of_antigens() + chart2.number_of_antigens() - report.common.common_antigens(), chart1.number_of_sera() + chart2.number_of_sera() - report.common.common_sera());

    result->info_modify()->virus(chart1.info()->virus());
    if (chart1.info()->number_of_sources() == 0) {
        result->info_modify()->add_source(chart1.info());
    }
    else {
        for (size_t s_no = 0; s_no < chart1.info()->number_of_sources(); ++s_no)
            result->info_modify()->add_source(chart1.info()->source(s_no));
    }
    if (chart2.info()->number_of_sources() == 0) {
        result->info_modify()->add_source(chart2.info());
    }
    else {
        for (size_t s_no = 0; s_no < chart2.info()->number_of_sources(); ++s_no)
            result->info_modify()->add_source(chart2.info()->source(s_no));
    }

    return {std::move(result), std::move(report)};

} // acmacs::chart::merge

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
