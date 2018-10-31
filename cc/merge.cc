#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2);

// ----------------------------------------------------------------------

acmacs::chart::MergeReport::MergeReport(const Chart& primary, const Chart& secondary, CommonAntigensSera::match_level_t a_match_level)
    : match_level{a_match_level}, common(primary, secondary, a_match_level)
{
    const bool remove_distinct = primary.info()->lab() == "CDC";
    auto src1 = primary.antigens();
    size_t target_no = 0;
    for (size_t no1 = 0; no1 < src1->size(); ++no1) {
        if (!remove_distinct || !src1->at(no1)->distinct())
            antigens_primary_target[no1] = target_no++;
    }
    auto src2 = secondary.antigens();
    for (size_t no2 = 0; no2 < src2->size(); ++no2) {
        if (!remove_distinct || !src2->at(no2)->distinct()) {
            if (const auto no1 = common.antigen_primary_by_secondary(no2); no1)
                antigens_secondary_target[no2] = antigens_primary_target.at(*no1);
            else
                antigens_secondary_target[no2] = target_no++;
        }
    }

} // acmacs::chart::MergeReport::MergeReport

// ----------------------------------------------------------------------

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2,
                                                                                        acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    auto merge_antigens_sera = [](auto& target, const auto& source1, const auto& source2, auto primary_by_secondary) {
        for (size_t ag_no = 0; ag_no < source1.size(); ++ag_no)
            target.at(ag_no).replace_with(source1.at(ag_no));
        for (size_t no = source1.size(), no2 = 0; no2 < source2.size(); ++no2) {
            if (auto no1 = primary_by_secondary(no2); no1)
                target.at(*no1).update_with(source2.at(no2));
            else
                target.at(no++).replace_with(source2.at(no2));
        }
    };

      // --------------------------------------------------

    MergeReport report(chart1, chart2, match_level);

    ChartModifyP result = std::make_shared<ChartNew>(chart1.number_of_antigens() + chart2.number_of_antigens() - report.common.common_antigens(),
                                                     chart1.number_of_sera() + chart2.number_of_sera() - report.common.common_sera());
    merge_info(*result, chart1, chart2);
    merge_antigens_sera(*result->antigens_modify(), *chart1.antigens(), *chart2.antigens(), [&report](size_t no2) { return report.common.antigen_primary_by_secondary(no2); });
    merge_antigens_sera(*result->sera_modify(), *chart1.sera(), *chart2.sera(), [&report](size_t no2) { return report.common.serum_primary_by_secondary(no2); });

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
