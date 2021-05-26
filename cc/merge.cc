#include "acmacs-base/date.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/procrustes.hh"

// ----------------------------------------------------------------------

static void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2);
static void merge_titers(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void merge_plot_spec(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const acmacs::chart::MergeReport& report);
static void merge_projections_type2(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void merge_projections_type3(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void merge_projections_type5(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report);
static void copy_layout(const acmacs::Layout& source, acmacs::Layout& target, size_t source_number_of_antigens, size_t target_number_of_antigens);
static acmacs::chart::DisconnectedPoints map_disconnected(const acmacs::chart::DisconnectedPoints& source, size_t source_number_of_antigens, size_t target_number_of_antigens, const acmacs::chart::MergeReport::index_mapping_t& antigen_mapping, const acmacs::chart::MergeReport::index_mapping_t& sera_mapping);
static void check_projection_before_merging(const acmacs::chart::Projection& projection);
static void check_projections_before_merging(const acmacs::chart::Projection& projection1, const acmacs::chart::Projection& projection2);

// ----------------------------------------------------------------------

acmacs::chart::MergeReport::MergeReport(const Chart& primary, const Chart& secondary, const MergeSettings& settings)
    : match_level{settings.match_level}, common(primary, secondary, settings.match_level)
{
      // antigens
    {
        const bool remove_distinct = settings.remove_distinct_ == remove_distinct::yes && primary.info()->lab(Info::Compute::Yes) == Lab{"CDC"};
        auto src1 = primary.antigens();
        for (size_t no1 = 0; no1 < src1->size(); ++no1) {
            if (!remove_distinct || !src1->at(no1)->distinct())
                antigens_primary_target.emplace_or_replace(no1, target_index_common_t{target_antigens++, common.antigen_secondary_by_primary(no1).has_value()});
        }
        auto src2 = secondary.antigens();
        const auto secondary_antigen_indexes = secondary_antigens_to_merge(primary, secondary, settings);
        for (const auto no2 : secondary_antigen_indexes) {
            if (!remove_distinct || !src2->at(no2)->distinct()) {
                if (const auto no1 = common.antigen_primary_by_secondary(no2); no1)
                    antigens_secondary_target.emplace_or_replace(no2, antigens_primary_target.get(*no1));
                else
                    antigens_secondary_target.emplace_or_replace(no2, target_index_common_t{target_antigens++, false});
            }
        }
    }

      // sera
    {
        auto src1 = primary.sera();
        for (size_t no1 = 0; no1 < src1->size(); ++no1)
            sera_primary_target.emplace_or_replace(no1, target_index_common_t{target_sera++, common.serum_secondary_by_primary(no1).has_value()});
        auto src2 = secondary.sera();
        for (size_t no2 = 0; no2 < src2->size(); ++no2) {
            if (const auto no1 = common.serum_primary_by_secondary(no2); no1)
                sera_secondary_target.emplace_or_replace(no2, sera_primary_target.get(*no1));
            else
                sera_secondary_target.emplace_or_replace(no2, target_index_common_t{target_sera++, false});
        }
    }

    // std::cerr << "DEBUG: antigens_primary_target " << antigens_primary_target << '\n';
    // std::cerr << "DEBUG: antigens_secondary_target" << antigens_secondary_target << '\n';
    // AD_DEBUG("sera_primary_target {}", sera_primary_target);
    // AD_DEBUG("sera_secondary_target {}", sera_secondary_target);

} // acmacs::chart::MergeReport::MergeReport

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::MergeReport::secondary_antigens_to_merge(const Chart& primary, const Chart& secondary, const MergeSettings& settings) const
{
    Indexes indexes = secondary.antigens()->all_indexes();
    if (settings.combine_cheating_assays_ == combine_cheating_assays::yes) {
        // expected: primary chart is single or multi layered, secondary chart is single layered
        auto secondary_titers = secondary.titers();
        if (secondary_titers->number_of_layers() > 1)
            AD_WARNING("[chart merge and combine_cheating_assays]: secondary chart is multilayered, result can be unexpected");

        bool all_secondary_in_primary{true}; // otherwise no cheating assay
        bool titers_same{true};              // otherwise no cheating assay

        const auto primary_by_secondary = [&all_secondary_in_primary](const auto& indexes2, auto&& func) {
            return ranges::views::transform(indexes2,
                                            [&all_secondary_in_primary, &func](size_t no2) -> size_t {
                                                if (const auto no1 = func(no2); no1.has_value()) {
                                                    return *no1;
                                                }
                                                else {
                                                    all_secondary_in_primary = false;
                                                    return static_cast<size_t>(-1);
                                                }
                                            }) |
                   ranges::to_vector;
        };

        auto secondary_antigens = secondary.antigens();
        auto secondary_sera = secondary.sera();

        const auto antigens2_indexes = secondary_antigens->reference_indexes();
        const auto sera2_indexes = secondary_sera->all_indexes();
        const auto antigen_indexes1 = primary_by_secondary(antigens2_indexes, [this](size_t no2) { return common.antigen_primary_by_secondary(no2); });
        const auto serum_indexes1 = primary_by_secondary(sera2_indexes, [this](size_t no2) { return common.serum_primary_by_secondary(no2); });
        if (all_secondary_in_primary) {
            // check titers
            const auto are_titers_same = [&](auto&& primary_titer) {
                for (const auto antigen_index_no : range_from_0_to(antigens2_indexes.size())) {
                    for (const auto serum_index_no : range_from_0_to(sera2_indexes.size())) {
                        if (secondary_titers->titer(antigens2_indexes[antigen_index_no], sera2_indexes[serum_index_no]) !=
                            primary_titer(antigen_indexes1[antigen_index_no], serum_indexes1[serum_index_no]))
                            return false;
                    }
                }
                return true;
            };

            auto primary_titers = primary.titers();
            if (primary_titers->number_of_layers() == 0) {
                titers_same = are_titers_same([&primary_titers](size_t ag, size_t sr) { return primary_titers->titer(ag, sr); });
            }
            else {
                titers_same = false;
                // if in any of the layers titers are the same
                for (const auto layer_no : range_from_0_to(primary_titers->number_of_layers())) {
                    titers_same |= are_titers_same([&primary_titers, layer_no](size_t ag, size_t sr) { return primary_titers->titer_of_layer(layer_no, ag, sr); });
                    if (titers_same)
                        break;
                }
            }
        }

        if (all_secondary_in_primary && titers_same) {
            if (const auto test_indexes = secondary_antigens->test_indexes(); !test_indexes.empty()) {
                AD_INFO("cheating assay ({}) will be combined, no reference titers will be in the new layer, test antigens: {}", secondary.make_name(), test_indexes.size());
                return test_indexes;
            }
            else {
                AD_ERROR("cheating assay ({}) and chart has no test antigens, remove table or disable cheating assay handling", secondary.make_name());
                throw merge_error{"cheating assay and chart has no test antigens"};
            }
        }
        else {
            // if (!all_secondary_in_primary)
            //     AD_INFO("[chart merge and combine_cheating_assays]: not a cheating assay: not all secondary ref antigens/sera found in primary");
            // if (!titers_same)
            //     AD_INFO("[chart merge and combine_cheating_assays]: not a cheating assay: titers are different");
        }
    }

    return secondary.antigens()->all_indexes(); // no cheating assay or combining not requested

} // acmacs::chart::MergeReport::secondary_antigens_to_merge

// ----------------------------------------------------------------------

static const auto merge_antigens_sera = [](auto& target, const auto& source, const acmacs::chart::MergeReport::index_mapping_t& to_target, bool always_replace) {
    for (size_t no = 0; no < source.size(); ++no) {
        if (const auto entry = to_target.find(no); entry != to_target.end()) {
            if (!always_replace && entry->second.common)
                target.at(entry->second.index).update_with(*source.at(no));
            else
                target.at(entry->second.index).replace_with(*source.at(no));
            // AD_DEBUG("merge_antigens_sera {} {} <- {} {}", entry->second.index, target.at(entry->second.index).full_name(), no, source.at(no)->full_name());
        }
    }
};

// ----------------------------------------------------------------------

std::pair<acmacs::chart::ChartModifyP, acmacs::chart::MergeReport> acmacs::chart::merge(const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const MergeSettings& settings)
{
    const auto relax_type4_type5 = [](const MergeReport& report, ChartModify& chart) {
        UnmovablePoints unmovable_points;
        // set unmovable for all points of chart1 including common ones
        for (const auto& [index1, index_merge_common] : report.antigens_primary_target)
            unmovable_points.insert(index_merge_common.index);
        for (const auto& [index1, index_merge_common] : report.sera_primary_target)
            unmovable_points.insert(index_merge_common.index + chart.number_of_antigens());
        auto projection = chart.projection_modify(0);
        projection->set_unmovable(unmovable_points);
        projection->relax(optimization_options{});
    };

    // --------------------------------------------------

    if (const auto dup1a = chart1.antigens()->find_duplicates(), dup1s = chart1.sera()->find_duplicates(); !dup1a.empty() || !dup1s.empty()) {
        throw merge_error{acmacs::string::concat(chart1.description(), " has duplicates among antigens or sera: ", to_string(dup1a), ' ', to_string(dup1s))};
    }
    if (const auto dup2a = chart2.antigens()->find_duplicates(), dup2s = chart2.sera()->find_duplicates(); !dup2a.empty() || !dup2s.empty()) {
        throw merge_error{acmacs::string::concat(chart2.description(), " has duplicates among antigens or sera: ", to_string(dup2a), ' ', to_string(dup2s))};
    }

    MergeReport report(chart1, chart2, settings);

    ChartModifyP result = std::make_shared<ChartNew>(report.target_antigens, report.target_sera);
    merge_info(*result, chart1, chart2);
    auto& result_antigens = result->antigens_modify();
    auto& result_sera = result->sera_modify();
    merge_antigens_sera(result_antigens, *chart1.antigens(), report.antigens_primary_target, true);
    merge_antigens_sera(result_antigens, *chart2.antigens(), report.antigens_secondary_target, false);
    merge_antigens_sera(result_sera, *chart1.sera(), report.sera_primary_target, true);
    merge_antigens_sera(result_sera, *chart2.sera(), report.sera_secondary_target, false);
    if (const auto rda = result_antigens.find_duplicates(), rds = result_sera.find_duplicates(); !rda.empty() || !rds.empty()) {
        fmt::memory_buffer msg;
        fmt::format_to(msg, "Merge \"{}\" has duplicates: AG:{} SR:{}\n", result->description(), rda, rds);
        for (const auto& dups : rda) {
            for (const auto ag_no : dups)
                fmt::format_to(msg, "  AG {:5d} {}\n", ag_no, result_antigens.at(ag_no).name_full());
            fmt::format_to(msg, "\n");
        }
        for (const auto& dups : rds) {
            for (const auto sr_no : dups)
                fmt::format_to(msg, "  SR {:5d} {}\n", sr_no, result_sera.at(sr_no).name_full());
            fmt::format_to(msg, "\n");
        }
        const auto err_message = fmt::to_string(msg);
        // AD_ERROR("{}", err_message);
        throw merge_error{err_message};
    }

    merge_titers(*result, chart1, chart2, report);
    merge_plot_spec(*result, chart1, chart2, report);

    if (chart1.number_of_projections()) {
        switch (settings.projection_merge) {
            case projection_merge_t::type1:
                break; // no projections in the merge
            case projection_merge_t::type2:
                merge_projections_type2(*result, chart1, chart2, report);
                break;
            case projection_merge_t::type3:
                if (chart2.number_of_projections() == 0)
                    throw merge_error{"cannot perform type3 merge: secondary chart has no projections"};
                merge_projections_type3(*result, chart1, chart2, report);
                break;
            case projection_merge_t::type4:
                if (chart2.number_of_projections() == 0)
                    throw merge_error{"cannot perform type4 merge: secondary chart has no projections"};
                merge_projections_type3(*result, chart1, chart2, report);
                // fmt::print(stderr, "DEBUG: merge type4 before relax stress {}\n", result->projection(0)->stress());
                relax_type4_type5(report, *result);
                // fmt::print(stderr, "DEBUG: merge type4 after relax stress {}\n", result->projection(0)->stress());
                break;
            case projection_merge_t::type5:
                if (chart2.number_of_projections() == 0)
                    throw merge_error{"cannot perform type5 merge: secondary chart has no projections"};
                merge_projections_type5(*result, chart1, chart2, report);
                // fmt::print(stderr, "DEBUG: merge type5 before relax stress {}\n", result->projection(0)->stress());
                relax_type4_type5(report, *result);
                // fmt::print(stderr, "DEBUG: merge type5 after relax stress {}\n", result->projection(0)->stress());
                break;
        }
    }

    return {std::move(result), std::move(report)};

} // acmacs::chart::merge

// ----------------------------------------------------------------------

void merge_titers(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report)
{
    auto& titers = result.titers_modify();
    auto titers1 = chart1.titers(), titers2 = chart2.titers();
    auto layers1 = titers1->number_of_layers(), layers2 = titers2->number_of_layers();
    titers.create_layers((layers1 ? layers1 : 1) + (layers2 ? layers2 : 1), result.number_of_antigens());

    size_t target_layer_no = 0;
    auto copy_layers = [&target_layer_no, &titers](size_t source_layers, const auto& source_titers, const acmacs::chart::MergeReport::index_mapping_t& antigen_target,
                                                   const acmacs::chart::MergeReport::index_mapping_t& serum_target) {
        auto assign = [&titers, &target_layer_no](size_t ag_no, size_t sr_no, const acmacs::chart::Titer& titer) { titers.titer(ag_no, sr_no, target_layer_no, titer); };
        auto copy_titer = [&assign, &antigen_target, &serum_target](const auto& titer_iterator) {
            for (auto titer_ref : titer_iterator) {
                if (auto ag_no = antigen_target.find(titer_ref.antigen), sr_no = serum_target.find(titer_ref.serum); ag_no != antigen_target.end() && sr_no != serum_target.end())
                    assign(ag_no->second.index, sr_no->second.index, titer_ref.titer);
            }
        };

        if (source_layers) {
            for (size_t source_layer_no = 0; source_layer_no < source_layers; ++source_layer_no, ++target_layer_no) {
                copy_titer(source_titers.titers_existing_from_layer(source_layer_no));
            }
        }
        else {
            copy_titer(source_titers.titers_existing());
            ++target_layer_no;
        }
    };
    copy_layers(layers1, *titers1, report.antigens_primary_target, report.sera_primary_target);
    copy_layers(layers2, *titers2, report.antigens_secondary_target, report.sera_secondary_target);
    report.titer_report = titers.set_from_layers(result);
}

// ----------------------------------------------------------------------

void merge_plot_spec(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, const acmacs::chart::MergeReport& report)
{
    auto plot_spec1 = chart1.plot_spec();
    auto plot_spec2 = chart2.plot_spec();
    auto& result_plot_spec = result.plot_spec_modify();
    for (size_t ag_no = 0; ag_no < chart1.number_of_antigens(); ++ag_no)
        result_plot_spec.modify(ag_no, plot_spec1->style(ag_no));
    for (size_t sr_no = 0; sr_no < chart1.number_of_sera(); ++sr_no)
        result_plot_spec.modify_serum(sr_no, plot_spec1->style(sr_no + chart1.number_of_antigens()));
    for (size_t ag_no = 0; ag_no < chart2.number_of_antigens(); ++ag_no) {
        if (auto found = report.antigens_secondary_target.find(ag_no); found != report.antigens_secondary_target.end() && !found->second.common)
            result_plot_spec.modify(found->second.index, plot_spec2->style(ag_no));
    }
    for (size_t sr_no = 0; sr_no < chart2.number_of_sera(); ++sr_no) {
        if (auto found = report.sera_secondary_target.find(sr_no); found != report.sera_secondary_target.end() && !found->second.common)
            result_plot_spec.modify_serum(found->second.index, plot_spec2->style(sr_no + chart2.number_of_antigens()));
    }

      // drawing order
      // auto& drawing_order = result_plot_spec.drawing_order_modify();

}

// ----------------------------------------------------------------------

void check_projection_before_merging(const acmacs::chart::Projection& projection)
{
    if (!projection.avidity_adjusts().empty())
        throw acmacs::chart::merge_error{"projection has avidity_adjusts"};
}

// ----------------------------------------------------------------------

void check_projections_before_merging(const acmacs::chart::Projection& projection1, const acmacs::chart::Projection& projection2)
{
    check_projection_before_merging(projection1);
    check_projection_before_merging(projection2);
    if (projection1.number_of_dimensions() != projection2.number_of_dimensions())
        throw acmacs::chart::merge_error{"projections have different number of dimensions"};
    if (projection1.minimum_column_basis() != projection2.minimum_column_basis())
        throw acmacs::chart::merge_error{"projections have different minimum column bases"};
}

// ----------------------------------------------------------------------

void merge_projections_type2(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& /*chart2*/, acmacs::chart::MergeReport& report)
{
    // copy best projection of chart1, set coords of non-common points of chart2 to NaN
    // std::cout << "INFO: incremental merge\n";
    auto projection1 = chart1.projection(0);
    check_projection_before_merging(*projection1);
    auto result_projection = result.projections_modify().new_from_scratch(projection1->number_of_dimensions(), projection1->minimum_column_basis());
    auto layout1 = projection1->layout();
    auto result_layout = result_projection->layout_modified();
    copy_layout(*layout1, *result_layout, chart1.number_of_antigens(), result.number_of_antigens());

    result_projection->transformation(projection1->transformation());
    if (const auto result_disconnected =
            map_disconnected(projection1->disconnected(), chart1.number_of_antigens(), result.number_of_antigens(), report.antigens_primary_target, report.sera_primary_target);
        !result_disconnected->empty()) {
        result_projection->set_disconnected(result_disconnected);
    }
}

// ----------------------------------------------------------------------

// The best projection of the second chart orieneted to the best
// projection of the first chart using procrustes. Coordinates of the
// non-common points are copied to the resulting layout from their
// source layouts. Coordinates of each common point are set to the
// middle between coordinates of that point in the source projections.

void merge_projections_type3(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report)
{
    auto projection1 = chart1.projection(0);
    auto projection2 = chart2.projection(0);
    check_projections_before_merging(*projection1, *projection2);

    // re-orinet layout2 to layout1 using procrustes
    const auto procrustes_data = procrustes(*projection1, *projection2, report.common.points(acmacs::chart::CommonAntigensSera::subset::all), acmacs::chart::procrustes_scaling_t::no);
    auto layout1 = projection1->transformed_layout();
    const auto transformation2 = procrustes_data.transformation;
    // std::cout << "INFO: transformation for the secondary layout: " << transformation2 << '\n';
    auto layout2 = projection2->layout()->transform(transformation2);

    auto result_projection = result.projections_modify().new_from_scratch(projection1->number_of_dimensions(), projection1->minimum_column_basis());
    auto result_layout = result_projection->layout_modified();
    copy_layout(*layout1, *result_layout, chart1.number_of_antigens(), result.number_of_antigens());

    for (const auto& [index2, merge] : report.antigens_secondary_target) {
        if (merge.common)
            result_layout->update(merge.index, acmacs::middle(layout2->at(index2), result_layout->at(merge.index)));
        else
            result_layout->update(merge.index, layout2->at(index2));
    }
    for (const auto& [index2, merge] : report.sera_secondary_target) {
        if (merge.common)
            result_layout->update(merge.index + result.number_of_antigens(), acmacs::middle(layout2->at(index2 + chart2.number_of_antigens()), result_layout->at(merge.index + result.number_of_antigens())));
        else
            result_layout->update(merge.index + result.number_of_antigens(), layout2->at(index2 + chart2.number_of_antigens()));
    }

    if (auto result_disconnected1 = map_disconnected(projection1->disconnected(), chart1.number_of_antigens(), result.number_of_antigens(), report.antigens_primary_target, report.sera_primary_target),
        result_disconnected2 = map_disconnected(projection2->disconnected(), chart2.number_of_antigens(), result.number_of_antigens(), report.antigens_secondary_target, report.sera_secondary_target);
        !result_disconnected1->empty() || !result_disconnected2->empty()) {
        result_disconnected1.extend(result_disconnected2);
        result_projection->set_disconnected(result_disconnected1);
    }
}

// ----------------------------------------------------------------------

// The best projection of the second chart orieneted to the best
// projection of the first chart using procrustes. Coordinates of the
// all points of the first chart and coordinates of the non-common
// points of the second chart are copied to the resulting layout from
// their source layouts.

void merge_projections_type5(acmacs::chart::ChartModify& result, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2, acmacs::chart::MergeReport& report)
{
    auto projection1 = chart1.projection(0);
    auto projection2 = chart2.projection(0);
    check_projections_before_merging(*projection1, *projection2);

    // re-orinet layout2 to layout1 using procrustes
    const auto procrustes_data = procrustes(*projection1, *projection2, report.common.points(acmacs::chart::CommonAntigensSera::subset::all), acmacs::chart::procrustes_scaling_t::no);
    auto layout1 = projection1->transformed_layout();
    const auto transformation2 = procrustes_data.transformation;
      // std::cout << "INFO: transformation for the secondary layout: " << transformation2 << '\n';
    auto layout2 = projection2->layout()->transform(transformation2);

    auto result_projection = result.projections_modify().new_from_scratch(projection1->number_of_dimensions(), projection1->minimum_column_basis());
    auto result_layout = result_projection->layout_modified();
    copy_layout(*layout1, *result_layout, chart1.number_of_antigens(), result.number_of_antigens());

    for (const auto& [index2, merge] : report.antigens_secondary_target) {
        if (!merge.common)
            result_layout->update(merge.index, layout2->at(index2));
    }
    for (const auto& [index2, merge] : report.sera_secondary_target) {
        if (!merge.common)
            result_layout->update(merge.index + result.number_of_antigens(), layout2->at(index2 + chart2.number_of_antigens()));
    }

    if (auto result_disconnected1 = map_disconnected(projection1->disconnected(), chart1.number_of_antigens(), result.number_of_antigens(), report.antigens_primary_target, report.sera_primary_target),
        result_disconnected2 = map_disconnected(projection2->disconnected(), chart2.number_of_antigens(), result.number_of_antigens(), report.antigens_secondary_target, report.sera_secondary_target);
        !result_disconnected1->empty() || !result_disconnected2->empty()) {
        result_disconnected1.extend(result_disconnected2);
        result_projection->set_disconnected(result_disconnected1);
    }

} // merge_projections_type5

// ----------------------------------------------------------------------

acmacs::chart::DisconnectedPoints map_disconnected(const acmacs::chart::DisconnectedPoints& source, size_t source_number_of_antigens, size_t target_number_of_antigens, const acmacs::chart::MergeReport::index_mapping_t& antigen_mapping, const acmacs::chart::MergeReport::index_mapping_t& sera_mapping)
{
    if (source->empty())
        return source;

    acmacs::chart::DisconnectedPoints result_disconnected;
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

void copy_layout(const acmacs::Layout& source, acmacs::Layout& target, size_t source_number_of_antigens, size_t target_number_of_antigens)
{
    for (size_t ag_no = 0; ag_no < source_number_of_antigens; ++ag_no)
        target.update(ag_no, source[ag_no]);
    for (size_t sr_no = 0; sr_no < (source.number_of_points() - source_number_of_antigens); ++sr_no)
        target.update(sr_no + target_number_of_antigens, source[sr_no + source_number_of_antigens]);
}

// ----------------------------------------------------------------------

void merge_info(acmacs::chart::ChartModify& target, const acmacs::chart::Chart& chart1, const acmacs::chart::Chart& chart2)
{
    target.info_modify().virus(chart1.info()->virus());
    if (chart1.info()->number_of_sources() == 0) {
        target.info_modify().add_source(chart1.info());
    }
    else {
        for (size_t s_no = 0; s_no < chart1.info()->number_of_sources(); ++s_no)
            target.info_modify().add_source(chart1.info()->source(s_no));
    }
    if (chart2.info()->number_of_sources() == 0) {
        target.info_modify().add_source(chart2.info());
    }
    else {
        for (size_t s_no = 0; s_no < chart2.info()->number_of_sources(); ++s_no)
            target.info_modify().add_source(chart2.info()->source(s_no));
    }

} // merge_info

// ----------------------------------------------------------------------

std::string acmacs::chart::MergeReport::titer_merge_report(const ChartModify& chart) const
{
    const auto max_field = std::max(static_cast<int>(std::max(max_full_name(*chart.antigens()), chart.info()->max_source_name())), 20);

    fmt::memory_buffer output;
    fmt::format_to(output, "Acmacs merge table and diagnositics\n\n{}\n{}\n\n", chart.description(), chart.show_table());

    fmt::format_to(output, "                                   DIAGNOSTICS\n         (common titers, and how they merged, and the individual tables)\n\n");
    fmt::format_to(output, "{}\n", titer_merge_diagnostics(chart, PointIndexList{filled_with_indexes(chart.antigens()->size())}, PointIndexList{filled_with_indexes(chart.sera()->size())}, max_field));

    for (auto layer_no : acmacs::range(chart.titers()->number_of_layers())) {
        if (layer_no < chart.info()->number_of_sources())
            fmt::format_to(output, "{}\n", chart.info()->source(layer_no)->name_non_empty());
        else
            fmt::format_to(output, "layer {}\n", layer_no);
        fmt::format_to(output, "{}\n\n", chart.show_table(layer_no));
    }

    fmt::format_to(output, "    Table merge subset showing only rows and columns that have merged values\n        (same as first diagnostic output, but subsetted for changes only)\n");
    const auto [antigens, sera] = chart.titers()->antigens_sera_in_multiple_layers();
    fmt::format_to(output, "{}\n", titer_merge_diagnostics(chart, antigens, sera, max_field));

    return fmt::to_string(output);

} // acmacs::chart::MergeReport::titer_merge_report

// ----------------------------------------------------------------------

std::string acmacs::chart::MergeReport::titer_merge_report_common_only(const ChartModify& chart) const
{
    const auto max_field = std::max(static_cast<int>(std::max(max_full_name(*chart.antigens()), chart.info()->max_source_name())), 20);

    fmt::memory_buffer output;
    fmt::format_to(output, "Acmacs merge diagnositics for common antigens and sera\n\n{}\n\n", chart.description());

    if (common.common_antigens() && common.common_sera()) {
        fmt::format_to(output, "                                   DIAGNOSTICS\n         (common titers, and how they merged, and the individual tables)\n\n");
        fmt::format_to(output, "{}\n", titer_merge_diagnostics(chart, common.common_primary_antigens(), common.common_primary_sera(), max_field));

        // for (auto layer_no : acmacs::range(chart.titers()->number_of_layers())) {
        //     if (layer_no < chart.info()->number_of_sources())
        //         fmt::format_to(output, "{}\n", chart.info()->source(layer_no)->name_non_empty());
        //     else
        //         fmt::format_to(output, "layer {}\n", layer_no);
        //     fmt::format_to(output, "{}\n\n", chart.show_table(layer_no));
        // }

        // fmt::format_to(output, "    Table merge subset showing only rows and columns that have merged values\n        (same as first diagnostic output, but subsetted for changes only)\n");
        // const auto [antigens, sera] = chart.titers()->antigens_sera_in_multiple_layers();
        // fmt::format_to(output, "{}\n", titer_merge_diagnostics(chart, antigens, sera, max_field));
    }
    else
        fmt::format_to(output, ">> WARNING: common only report cannot be done: common antigens: {} common sera: {}", common.common_antigens(), common.common_sera());

    return fmt::to_string(output);

} // acmacs::chart::MergeReport::titer_merge_report_common_only

// ----------------------------------------------------------------------

std::string acmacs::chart::MergeReport::titer_merge_diagnostics(const ChartModify& chart, const PointIndexList& antigens, const PointIndexList& sera, int max_field_size) const
{
    // auto sr_label = [](size_t sr_no) -> char { return static_cast<char>('A' + sr_no); };
    auto sr_label = [](size_t sr_no) -> size_t { return sr_no + 1; };
    auto ags = chart.antigens();
    auto srs = chart.sera();
    auto tt = chart.titers();

    fmt::memory_buffer output;
    fmt::format_to(output, "{:{}s}", "", max_field_size);
    for (auto sr_no : sera)
        fmt::format_to(output, "{:>7d}", sr_label(sr_no));
    fmt::format_to(output, "\n{:{}s}", "", max_field_size + 2);
    for (auto sr_no : sera)
        fmt::format_to(output, "{:>7s}", srs->at(sr_no)->format("{location_abbreviated}/{year2}"));
    fmt::format_to(output, "\n");

    for (auto ag_no : antigens) {
        auto antigen = ags->at(ag_no);
        fmt::format_to(output, "{}\n", antigen->name_full());
        for (auto layer_no : acmacs::range(tt->number_of_layers())) {
            if (layer_no < chart.info()->number_of_sources())
                fmt::format_to(output, "{:<{}s}", chart.info()->source(layer_no)->name_non_empty(), max_field_size + 2);
            else
                fmt::format_to(output, "{:<{}s}", fmt::format("layer {}", layer_no), max_field_size + 2);
            for (auto sr_no : sera) {
                auto titer = tt->titer_of_layer(layer_no, ag_no, sr_no);
                fmt::format_to(output, "{:>7s}", (titer.is_dont_care() ? "" : *titer));
            }
            fmt::format_to(output, "\n");
        }
        fmt::format_to(output, "{:<{}s}", "Merge", max_field_size + 2);
        for (auto sr_no : sera)
            fmt::format_to(output, "{:>7s}", *tt->titer(ag_no, sr_no));
        fmt::format_to(output, "\n");

        fmt::format_to(output, "{:<{}s}", "Report (see below)", max_field_size + 2);
        for (auto sr_no : sera) {
            if (const auto found = std::find_if(titer_report->begin(), titer_report->end(), [ag_no=ag_no,sr_no](const auto& entry) { return entry.antigen == ag_no && entry.serum == sr_no; }); found != titer_report->end())
                fmt::format_to(output, "{:>7s}", TitersModify::titer_merge_report_brief(found->report));
            else
                fmt::format_to(output, "       ");
        }
        fmt::format_to(output, "\n\n");
    }

    fmt::format_to(output, "{}\n", TitersModify::titer_merge_report_description());

    return fmt::to_string(output);

} // acmacs::chart::MergeReport::titer_merge_diagnostics

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
