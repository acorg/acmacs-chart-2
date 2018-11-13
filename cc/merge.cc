#include <sstream>

#include "acmacs-base/stream.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/date.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/procrustes.hh"

// ----------------------------------------------------------------------

static void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2);
static void merge_titers(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void merge_plot_spec(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const acmacs::chart::MergeReport& report);
static void merge_projections_incremental(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void merge_projections_overlay(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void copy_layout(const acmacs::chart::Layout& source, acmacs::chart::Layout& target, size_t source_number_of_antigens, size_t target_number_of_antigens);
static acmacs::chart::PointIndexList map_disconnected(const acmacs::chart::PointIndexList& source, size_t source_number_of_antigens, size_t target_number_of_antigens, const acmacs::chart::MergeReport::index_mapping_t& antigen_mapping, const acmacs::chart::MergeReport::index_mapping_t& sera_mapping);

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

static auto merge_antigens_sera = [](auto& target, const auto& source, const acmacs::chart::MergeReport::index_mapping_t& to_target) {
    for (size_t no = 0; no < source.size(); ++no) {
        if (const auto entry = to_target.find(no); entry != to_target.end()) {
            if (entry->second.common)
                target.at(entry->second.index).update_with(source.at(no));
            else
                target.at(entry->second.index).replace_with(source.at(no));
        }
    }
};

// ----------------------------------------------------------------------

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const MergeSettings& settings)
{
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

    merge_titers(result, chart1, chart2, report);
    merge_plot_spec(result, chart1, chart2, report);

    if (chart1.number_of_projections()) {
        switch (settings.projection_merge) {
            case projection_merge_t::none:
                break; // no projections in the merge
            case projection_merge_t::incremental:
                merge_projections_incremental(result, chart1, chart2, report);
                break;
            case projection_merge_t::overlay:
                if (chart2.number_of_projections() == 0)
                    throw merge_error{"cannot perform overlay merge: secondary chart has no projections"};
                merge_projections_overlay(result, chart1, chart2, report);
                break;
        }
    }

    return {std::move(result), std::move(report)};

} // acmacs::chart::merge

// ----------------------------------------------------------------------

void merge_titers(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report)
{
    auto titers = result->titers_modify();
    auto titers1 = chart1.titers(), titers2 = chart2.titers();
    auto layers1 = titers1->number_of_layers(), layers2 = titers2->number_of_layers();
    titers->create_layers((layers1 ? layers1 : 1) + (layers2 ? layers2 : 1), result->antigens()->size());

    size_t target_layer_no = 0;
    auto copy_layers = [&target_layer_no, &titers](size_t source_layers, const auto& source_titers, const acmacs::chart::MergeReport::index_mapping_t& antigen_target,
                                                   const acmacs::chart::MergeReport::index_mapping_t& serum_target) {
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
}

// ----------------------------------------------------------------------

void merge_plot_spec(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const acmacs::chart::MergeReport& report)
{
    auto plot_spec1 = chart1.plot_spec();
    auto plot_spec2 = chart2.plot_spec();
    auto result_plot_spec = result->plot_spec_modify();
    for (size_t ag_no = 0; ag_no < chart1.number_of_antigens(); ++ag_no)
        result_plot_spec->modify(ag_no, plot_spec1->style(ag_no));
    for (size_t sr_no = 0; sr_no < chart1.number_of_sera(); ++sr_no)
        result_plot_spec->modify_serum(sr_no, plot_spec1->style(sr_no + chart1.number_of_antigens()));
    for (size_t ag_no = 0; ag_no < chart2.number_of_antigens(); ++ag_no) {
        if (auto found = report.antigens_secondary_target.find(ag_no); found != report.antigens_secondary_target.end() && !found->second.common)
            result_plot_spec->modify(found->second.index, plot_spec2->style(ag_no));
    }
    for (size_t sr_no = 0; sr_no < chart2.number_of_sera(); ++sr_no) {
        if (auto found = report.sera_secondary_target.find(sr_no); found != report.sera_secondary_target.end() && !found->second.common)
            result_plot_spec->modify_serum(found->second.index, plot_spec2->style(sr_no + chart2.number_of_antigens()));
    }

      // drawing order
      // auto& drawing_order = result_plot_spec->drawing_order_modify();

}

// ----------------------------------------------------------------------

void merge_projections_incremental(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& /*chart2*/, acmacs::chart::MergeReport& report)
{
    // copy best projection of chart1, set coords of non-common points of chart2 to NaN
    // std::cout << "INFO: incremental merge\n";
    auto projection1 = chart1.projection(0);
    if (!projection1->avidity_adjusts().empty())
        throw acmacs::chart::merge_error{"chart1 projection has avidity_adjusts"};
    auto result_projection = result->projections_modify()->new_from_scratch(projection1->number_of_dimensions(), projection1->minimum_column_basis());
    auto layout1 = projection1->layout();
    auto result_layout = result_projection->layout_modified();
    copy_layout(*layout1, *result_layout, chart1.number_of_antigens(), result->number_of_antigens());

    result_projection->transformation(projection1->transformation());
    if (const auto result_disconnected =
            map_disconnected(projection1->disconnected(), chart1.number_of_antigens(), result->number_of_antigens(), report.antigens_primary_target, report.sera_primary_target);
        !result_disconnected.empty()) {
        result_projection->set_disconnected(result_disconnected);
    }
}

// ----------------------------------------------------------------------

void merge_projections_overlay(acmacs::chart::ChartModifyP result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report)
{
      // std::cout << "INFO: overlay merge\n";
    auto projection1 = chart1.projection(0);
    if (!projection1->avidity_adjusts().empty())
        throw acmacs::chart::merge_error{"chart1 projection has avidity_adjusts"};
    auto projection2 = chart2.projection(0);
    if (!projection2->avidity_adjusts().empty())
        throw acmacs::chart::merge_error{"chart2 projection has avidity_adjusts"};
    if (projection1->number_of_dimensions() != projection2->number_of_dimensions())
        throw acmacs::chart::merge_error{"projections have different number of dimensions"};
    if (projection1->minimum_column_basis() != projection2->minimum_column_basis())
        throw acmacs::chart::merge_error{"projections have different minimum column bases"};

    // re-orinet layout2 to layout1 using procrustes
    const auto procrustes_data = procrustes(*projection1, *projection2, report.common.points(acmacs::chart::CommonAntigensSera::subset::all), acmacs::chart::procrustes_scaling_t::no);
    auto layout1 = projection1->transformed_layout();
    const auto transformation2 = procrustes_data.transformation.transformation();
      // std::cout << "INFO: transformation for the secondary layout: " << transformation2 << '\n';
    auto layout2 = projection2->layout()->transform(transformation2);

    auto result_projection = result->projections_modify()->new_from_scratch(projection1->number_of_dimensions(), projection1->minimum_column_basis());
    auto result_layout = result_projection->layout_modified();
    copy_layout(*layout1, *result_layout, chart1.number_of_antigens(), result->number_of_antigens());

    for (size_t ag_no = 0; ag_no < chart2.number_of_antigens(); ++ag_no) {
        if (auto found = report.antigens_secondary_target.find(ag_no); found != report.antigens_secondary_target.end()) {
            auto coord2 = layout2->get(ag_no);
            if (found->second.common)
                result_layout->set(found->second.index, coord2.mean_with(result_layout->get(found->second.index)));
            else
                result_layout->set(found->second.index, coord2);
        }
    }
    for (size_t sr_no = 0; sr_no < chart2.number_of_sera(); ++sr_no) {
        if (auto found = report.sera_secondary_target.find(sr_no); found != report.sera_secondary_target.end()) {
            auto coord2 = layout2->get(sr_no + chart2.number_of_antigens());
            if (found->second.common)
                result_layout->set(found->second.index + result->number_of_antigens(), coord2.mean_with(result_layout->get(found->second.index + result->number_of_antigens())));
            else
                result_layout->set(found->second.index + result->number_of_antigens(), coord2);
        }
    }

    if (auto result_disconnected1 =
            map_disconnected(projection1->disconnected(), chart1.number_of_antigens(), result->number_of_antigens(), report.antigens_primary_target, report.sera_primary_target),
        result_disconnected2 = map_disconnected(projection2->disconnected(), chart2.number_of_antigens(), result->number_of_antigens(), report.antigens_secondary_target, report.sera_secondary_target);
        !result_disconnected1.empty() || !result_disconnected2.empty()) {
        result_disconnected1.extend(result_disconnected2);
        result_projection->set_disconnected(result_disconnected1);
    }
}

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList map_disconnected(const acmacs::chart::PointIndexList& source, size_t source_number_of_antigens, size_t target_number_of_antigens, const acmacs::chart::MergeReport::index_mapping_t& antigen_mapping, const acmacs::chart::MergeReport::index_mapping_t& sera_mapping)
{
    if (source.empty())
        return source;

    acmacs::chart::PointIndexList result_disconnected;
    for (const auto p_no : source) {
        if (p_no < source_number_of_antigens) {
            if (const auto found = antigen_mapping.find(p_no); found != antigen_mapping.end())
                result_disconnected.insert(found->second.index);
        }
        else {
            if (const auto found = sera_mapping.find(p_no - source_number_of_antigens); found != sera_mapping.end())
                result_disconnected.insert(found->second.index + target_number_of_antigens);
        }
    }
    return result_disconnected;

} // map_disconnected

// ----------------------------------------------------------------------

void copy_layout(const acmacs::chart::Layout& source, acmacs::chart::Layout& target, size_t source_number_of_antigens, size_t target_number_of_antigens)
{
    for (size_t ag_no = 0; ag_no < source_number_of_antigens; ++ag_no)
        target.set(ag_no, source.get(ag_no));
    for (size_t sr_no = 0; sr_no < (source.number_of_points() - source_number_of_antigens); ++sr_no)
        target.set(sr_no + target_number_of_antigens, source.get(sr_no + source_number_of_antigens));
}

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
    const auto max_field = std::max(static_cast<int>(std::max(chart.antigens()->max_full_name(), chart.info()->max_source_name())), 20);
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

std::string acmacs::chart::MergeReport::titer_merge_report(const ChartModify& chart) const
{
    std::ostringstream output;
    titer_merge_report(output, chart);
    return output.str();

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
