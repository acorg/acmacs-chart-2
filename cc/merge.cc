#include "acmacs-base/stream.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/date.hh"
#include "acmacs-chart-2/merge.hh"

// ----------------------------------------------------------------------

static void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2);

// ----------------------------------------------------------------------

acmacs::chart::MergeReport::MergeReport(const Chart& primary, const Chart& secondary, const MergeSettings& settings)
    : match_level{settings.match_level}, common(primary, secondary, settings.match_level)
{
      // antigens
    {
        const bool remove_distinct = settings.remove_distinct && primary.info()->lab(Info::Compute::Yes) == "CDC";
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

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const MergeSettings& settings)
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

    if (!chart1.antigens()->find_duplicates().empty() || !chart1.sera()->find_duplicates().empty() || !chart2.antigens()->find_duplicates().empty() || !chart2.sera()->find_duplicates().empty())
        throw merge_error{"charts to merge have duplicates among antigens or sera"};

    MergeReport report(chart1, chart2, settings);

    ChartModifyP result = std::make_shared<ChartNew>(report.target_antigens, report.target_sera);
    merge_info(*result, chart1, chart2);
    auto result_antigens = result->antigens_modify();
    auto result_sera = result->sera_modify();
    merge_antigens_sera(*result_antigens, *chart1.antigens(), report.antigens_primary_target);
    merge_antigens_sera(*result_antigens, *chart2.antigens(), report.antigens_secondary_target);
    merge_antigens_sera(*result_sera, *chart1.sera(), report.sera_primary_target);
    merge_antigens_sera(*result_sera, *chart2.sera(), report.sera_secondary_target);
    if (!result_antigens->find_duplicates().empty() || !result_sera->find_duplicates().empty())
        throw merge_error{"merge has duplicates among antigens or sera"};

    // titers

    auto titers = result->titers_modify();
    auto titers1 = chart1.titers(), titers2 = chart2.titers();
    auto layers1 = titers1->number_of_layers(), layers2 = titers2->number_of_layers();
    titers->create_layers((layers1 ? layers1 : 1) + (layers2 ? layers2 : 1), result_antigens->size());

    size_t target_layer_no = 0;
    auto copy_layers = [&target_layer_no, &titers](size_t source_layers, const auto& source_titers, const MergeReport::index_mapping_t& antigen_target,
                                                   const MergeReport::index_mapping_t& serum_target) {
        auto assign = [&titers, &target_layer_no](size_t ag_no, size_t sr_no, std::string titer) { titers->titer(ag_no, sr_no, target_layer_no, titer); };
        auto copy_titer = [&assign, &antigen_target, &serum_target](auto first, auto last) {
            for (; first != last; ++first) {
                if (auto ag_no = antigen_target.find(first->antigen), sr_no = serum_target.find(first->serum); ag_no != antigen_target.end() && sr_no != serum_target.end())
                    assign(ag_no->second.index, sr_no->second.index, first->titer);
            }
        };
        if (source_layers) {
            for (size_t source_layer_no = 0; source_layer_no < source_layers; ++source_layer_no, ++target_layer_no)
                copy_titer(source_titers.begin(source_layer_no), source_titers.end(source_layer_no));
        }
        else {
            copy_titer(source_titers.begin(), source_titers.end());
            ++target_layer_no;
        }
    };
    copy_layers(layers1, *titers1, report.antigens_primary_target, report.sera_primary_target);
    copy_layers(layers2, *titers2, report.antigens_secondary_target, report.sera_secondary_target);
    report.titer_report = titers->set_from_layers(*result);

    // projections
    // plot spec

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

void acmacs::chart::MergeReport::titer_merge_report(std::string_view filename, const ChartModify& chart, const char* progname) const
{
    acmacs::file::ofstream output(filename);
    *output << "Acmacs merge table and diagnositics (in Derek's style).\nCreated by " << progname << " on " << current_date_time() << "\n\n";
    titer_merge_report(output, chart);

} // acmacs::chart::MergeReport::titer_merge_report

// ----------------------------------------------------------------------

void acmacs::chart::MergeReport::titer_merge_report(std::ostream& output, const ChartModify& chart) const
{
    const auto max_field = static_cast<int>(std::max(chart.antigens()->max_full_name(), chart.info()->max_source_name()));
    const auto hr = std::string(100, '-') + '\n';

    output << hr << chart.description() << '\n';
    chart.show_table(output);
    output << "\n\n";

    output << hr << "                                   DIAGNOSTICS\n         (common titers, and how they merged, and the individual tables)\n" << hr;
    titer_merge_diagnostics(output, chart, filled_with_indexes(chart.antigens()->size()), filled_with_indexes(chart.sera()->size()), max_field);

    for (auto layer_no : acmacs::range(chart.titers()->number_of_layers())) {
        output << hr << chart.info()->source(layer_no)->name_non_empty() << '\n';
        chart.show_table(output, layer_no);
        output << "\n\n";
    }

    output << hr << "    Table merge subset showing only rows and columns that have merged values\n        (same as first diagnostic output, but subsetted for changes only)\n" << hr;
    const auto [antigens, sera] = chart.titers()->antigens_sera_in_multiple_layers();
    titer_merge_diagnostics(output, chart, antigens, sera, max_field);

} // acmacs::chart::MergeReport::titer_merge_report

// ----------------------------------------------------------------------

void acmacs::chart::MergeReport::titer_merge_diagnostics(std::ostream& output, const ChartModify& chart, const PointIndexList& antigens, const PointIndexList& sera, int max_field_size) const
{
    auto sr_label = [](size_t sr_no) -> char { return static_cast<char>('A' + sr_no); };
    auto ags = chart.antigens();
    auto srs = chart.sera();
    auto tt = chart.titers();

    output << std::setw(max_field_size) << ' ';
    for (auto sr_no : sera)
        output << std::setw(7) << std::right << sr_label(sr_no);
    output << '\n' << std::setw(max_field_size + 2) << ' ';
    for (auto sr_no : sera)
        output << std::setw(7) << std::right << srs->at(sr_no)->abbreviated_location_year();
    output << '\n';

    for (auto ag_no : antigens) {
        auto antigen = ags->at(ag_no);
        output << antigen->full_name() << '\n';
        for (auto layer_no : acmacs::range(tt->number_of_layers())) {
            output << std::setw(max_field_size + 2) << std::left << chart.info()->source(layer_no)->name_non_empty();
            for (auto sr_no : sera) {
                auto titer = tt->titer_of_layer(layer_no, ag_no, sr_no);
                if (titer == "*")
                    titer.clear();
                output << std::setw(7) << std::right << titer;
            }
            output << '\n';
        }
        output << std::setw(max_field_size + 2) << std::left << "Merge";
        for (auto sr_no : sera)
            output << std::setw(7) << std::right << tt->titer(ag_no, sr_no);
        output << '\n';

        output << std::setw(max_field_size + 2) << std::left << "Report (see below)";
        for (auto sr_no : sera) {
            output << std::setw(7) << std::right;
            if (const auto found = std::find_if(titer_report->begin(), titer_report->end(), [ag_no=ag_no,sr_no](const auto& entry) { return entry.antigen == ag_no && entry.serum == sr_no; }); found != titer_report->end())
                output << TitersModify::titer_merge_report_brief(found->report);
            else
                output << ' ';
        }
        output << "\n\n";
    }
    output << TitersModify::titer_merge_report_description() << '\n';

} // acmacs::chart::MergeReport::titer_merge_diagnostics

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
