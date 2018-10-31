#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2);

// ----------------------------------------------------------------------

acmacs::chart::MergeReport::MergeReport(const Chart& primary, const Chart& secondary, CommonAntigensSera::match_level_t a_match_level)
    : match_level{a_match_level}, common(primary, secondary, a_match_level)
{
      // antigens
    {
        const bool remove_distinct = primary.info()->lab() == "CDC";
        auto src1 = primary.antigens();
        for (size_t no1 = 0; no1 < src1->size(); ++no1) {
            if (!remove_distinct || !src1->at(no1)->distinct())
                antigens_primary_target[no1] = target_antigens++;
        }
        auto src2 = secondary.antigens();
        for (size_t no2 = 0; no2 < src2->size(); ++no2) {
            if (!remove_distinct || !src2->at(no2)->distinct()) {
                if (const auto no1 = common.antigen_primary_by_secondary(no2); no1)
                    antigens_secondary_target[no2] = antigens_primary_target.at(*no1);
                else
                    antigens_secondary_target[no2] = target_antigens++;
            }
        }
    }

      // sera
    {
        auto src1 = primary.sera();
        for (size_t no1 = 0; no1 < src1->size(); ++no1)
            sera_primary_target[no1] = target_sera++;
        auto src2 = secondary.sera();
        for (size_t no2 = 0; no2 < src2->size(); ++no2) {
            if (const auto no1 = common.serum_primary_by_secondary(no2); no1)
                sera_secondary_target[no2] = sera_primary_target.at(*no1);
            else
                sera_secondary_target[no2] = target_sera++;
        }
    }

    // std::cerr << "DEBUG: antigens_primary_target " << antigens_primary_target << DEBUG_LINE_FUNC << '\n';
    // std::cerr << "DEBUG: antigens_secondary_target" << antigens_secondary_target << DEBUG_LINE_FUNC << '\n';
    // std::cerr << "DEBUG: sera_primary_target " << sera_primary_target << DEBUG_LINE_FUNC << '\n';
    // std::cerr << "DEBUG: sera_secondary_target" << sera_secondary_target << DEBUG_LINE_FUNC << '\n';

} // acmacs::chart::MergeReport::MergeReport

// ----------------------------------------------------------------------

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2,
                                                                                        acmacs::chart::CommonAntigensSera::match_level_t match_level)
{
    auto merge_antigens_sera = [](auto& target, const auto& source, const MergeReport::index_mapping_t& to_target) {
        for (size_t no = 0; no < source.size(); ++no) {
            if (const auto entry = to_target.find(no); entry != to_target.end()) {
                if (entry->second.common)
                    target.at(entry->second.index).update_with(source.at(no));
                else
                    target.at(entry->second.index).replace_with(source.at(no));
            }
        }
    };

    // --------------------------------------------------

    MergeReport report(chart1, chart2, match_level);

    ChartModifyP result = std::make_shared<ChartNew>(report.target_antigens, report.target_sera);
    merge_info(*result, chart1, chart2);
    merge_antigens_sera(*result->antigens_modify(), *chart1.antigens(), report.antigens_primary_target);
    merge_antigens_sera(*result->antigens_modify(), *chart2.antigens(), report.antigens_secondary_target);
    merge_antigens_sera(*result->sera_modify(), *chart1.sera(), report.sera_primary_target);
    merge_antigens_sera(*result->sera_modify(), *chart2.sera(), report.sera_secondary_target);

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
