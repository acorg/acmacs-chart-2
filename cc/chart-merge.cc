#include "acmacs-base/argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/merge.hh"
#include "acmacs-chart-2/common.hh"

// ----------------------------------------------------------------------

using Charts = std::vector<std::unique_ptr<acmacs::chart::ChartModify>>;

static void set_merge_type(acmacs::chart::MergeSettings& settings, std::string_view merge_type);
static void combine_cheating_assays(Charts& charts, bool combine_requested);
[[nodiscard]] static std::vector<std::vector<size_t>> get_cheating_assays(const Charts& charts);
static void combine_tables(Charts& charts, const std::vector<size_t>& to_combine);

// ----------------------------------------------------------------------

using namespace acmacs::argv;

struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>  output_chart{*this, 'o', "output", dflt{""}, desc{"output chart"}};
    option<str>  match{*this, "match", dflt{"auto"}, desc{"match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""}};
    option<str>  merge_type{*this, 'm', "merge-type", dflt{"simple"}, desc{"merge type: \"type1\"..\"type5\", \"incremental\" (type2), \"overlay\" (type3), \"simple\" (type1)"}};
    option<bool> combine_cheating_assays{*this, "combine-cheating-assays", desc{"combine tables if they have the same reference titers"}};
    option<bool> duplicates_distinct{*this, "duplicates-distinct", desc{"make duplicates distinct"}};
    option<str>  report_titers{*this, "report", desc{"titer merge report"}};
    option<bool> report_common_only{*this, "common-only", desc{"titer merge report for common antigens and sera only"}};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers: all, common"}};

    argument<str_array> source_charts{*this, arg_name{"source-chart"}, mandatory};
};

int main(int argc, const char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

        acmacs::chart::MergeSettings settings;
        set_merge_type(settings, opt.merge_type);
        settings.match_level = acmacs::chart::CommonAntigensSera::match_level(opt.match);
        if (opt.source_charts->size() < 2)
            throw std::runtime_error("too few source charts specified");

        Charts charts;
        for (auto chart_no : range_from_0_to(opt.source_charts->size())) {
            auto chart = std::make_unique<acmacs::chart::ChartModify>(acmacs::chart::import_from_file((*opt.source_charts)[chart_no]));
            if (*opt.duplicates_distinct) {
                chart->antigens_modify().duplicates_distinct(chart->antigens()->find_duplicates());
                chart->sera_modify().duplicates_distinct(chart->sera()->find_duplicates());
            }
            charts.push_back(std::move(chart));
        }

        combine_cheating_assays(charts, opt.combine_cheating_assays);

        auto [result, merge_report] = acmacs::chart::merge(*charts[0], *charts[1], settings);

        fmt::print("{}\n{}\n\n{}\n----------\n\n", charts[0]->description(), charts[1]->description(), merge_report.common.report());
        for (const size_t c_no : range_from_to(2ul, charts.size())) {
            auto& chart3 = *charts[c_no];
            fmt::print("{}\n{}\n\n", result->description(), chart3.description());
            std::tie(result, merge_report) = acmacs::chart::merge(*result, chart3, settings);
            fmt::print("{}\n----------\n\n", merge_report.common.report());
        }
        if (opt.output_chart.has_value())
            acmacs::chart::export_factory(*result, opt.output_chart, opt.program_name());
        if (opt.report_titers.has_value()) {
            if (opt.report_common_only)
                acmacs::file::write(opt.report_titers, merge_report.titer_merge_report_common_only(*result));
            else
                acmacs::file::write(opt.report_titers, merge_report.titer_merge_report(*result));
        }
        else {
            fmt::print("{}\n", result->make_info());
            if (const auto having_too_few_numeric_titers = result->titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers->empty())
                fmt::print("Points having too few numeric titers: {} {}\n", having_too_few_numeric_titers->size(), having_too_few_numeric_titers);
            fmt::print("\nTables:\n");
            auto info = result->info();
            for (auto src_no : range_from_0_to(info->number_of_sources()))
                fmt::print("{:3d} {}\n", src_no, info->source(src_no)->make_name());
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void set_merge_type(acmacs::chart::MergeSettings& settings, std::string_view merge_type)
{
    using namespace std::string_view_literals;
    if (merge_type == "incremental"sv || merge_type == "type2"sv)
        settings.projection_merge = acmacs::chart::projection_merge_t::type2;
    else if (merge_type == "overlay"sv || merge_type == "type3"sv)
        settings.projection_merge = acmacs::chart::projection_merge_t::type3;
    else if (merge_type == "type4"sv)
        settings.projection_merge = acmacs::chart::projection_merge_t::type4;
    else if (merge_type == "type5"sv)
        settings.projection_merge = acmacs::chart::projection_merge_t::type5;
    else if (merge_type != "simple"sv && merge_type != "type1"sv)
        throw std::runtime_error{fmt::format("unrecognized --merge-type value: \"{}\"", merge_type)};

} // set_merge_type

// ----------------------------------------------------------------------

void combine_cheating_assays(Charts& charts, bool combine_requested)
{
    const auto cheating_assays = get_cheating_assays(charts);
    if (cheating_assays.empty()) {
        if (combine_requested)
            AD_INFO("No cheating assays found");
        return;
    }
    if (!combine_requested) {
        AD_WARNING("Cheating assays present, consider using --combine-cheating-assays");
        for (const auto& group : cheating_assays) {
            fmt::print(stderr, "    cheating assay group\n");
            for (const auto chart_no : group)
                fmt::print(stderr, "        {}\n", charts[chart_no]->make_name());
        }
        fmt::print(stderr, "\n\n");
        return;
    }

    for (const auto& group : cheating_assays)
        combine_tables(charts, group);

    // remove appended charts
    for (const auto& group : cheating_assays)
        ranges::for_each(group | ranges::views::drop(1), [&charts](size_t chart_no) { charts[chart_no].reset(); });
    charts.erase(ranges::remove(charts, std::unique_ptr<acmacs::chart::ChartModify>{}), std::end(charts));

} // combine_cheating_assays

// ----------------------------------------------------------------------

void combine_tables(Charts& charts, const std::vector<size_t>& to_combine)
{
    auto& master = *charts[to_combine[0]];
    auto& master_antigens = master.antigens_modify();
    auto& master_titers = master.titers_modify();
    auto& master_plot_spec = master.plot_spec_modify();
    master_titers.remove_layers();
    master.projections_modify().remove_all();
    for (const auto chart_no : to_combine | ranges::views::drop(1)) {
        auto& to_append = *charts[chart_no];
        auto to_append_antigens = to_append.antigens();
        auto to_append_titers = to_append.titers();
        for (const auto to_append_antigen_no : to_append_antigens->test_indexes()) {
            master_antigens.append()->replace_with(*to_append_antigens->at(to_append_antigen_no));
            master_titers.append_antigen();
            master_plot_spec.append_antigen();
            for (const auto sr_no : range_from_0_to(master.number_of_sera()))
                master_titers.titer(master_titers.number_of_antigens() - 1, sr_no, to_append_titers->titer(to_append_antigen_no, sr_no));
        }
        master.info_modify().date(fmt::format("{}+{}", master.info_modify().date(), to_append.info()->date()));
    }

} // combine_tables

// ----------------------------------------------------------------------

std::vector<std::vector<size_t>> get_cheating_assays(const Charts& charts)
{
    const auto pair_order = [](const auto& e1, const auto& e2) { return e1.second < e2.second; };
    const auto get_reference_names = [pair_order](const auto& ag_sr) {
        return *ag_sr.reference_indexes()                                                                             //
               | ranges::views::transform([&ag_sr](size_t no) { return std::make_pair(no, ag_sr[no]->full_name()); }) //
               | ranges::to_vector                                                                                    //
               | ranges::actions::sort(pair_order);
    };
    const auto get_all_names = [pair_order](const auto& ag_sr) {
        return *ag_sr.all_indexes()                                                                                   //
               | ranges::views::transform([&ag_sr](size_t no) { return std::make_pair(no, ag_sr[no]->full_name()); }) //
               | ranges::to_vector                                                                                    //
               | ranges::actions::sort(pair_order);
    };

    const auto titers_same = []<typename T, typename N>(const T& titers1, const N& antigens1, const N& sera1, const T& titers2, const N& antigens2, const N& sera2) {
        for (const auto& [ag1_no, ag1] : antigens1) {
            const auto ag2_no = ranges::find_if(antigens2, [ag1 = ag1](const auto& en) { return en.second == ag1; })->first;
            for (const auto& [sr1_no, sr1] : sera1) {
                const auto sr2_no = ranges::find_if(sera2, [sr1 = sr1](const auto& en) { return en.second == sr1; })->first;
                if (titers1.titer(ag1_no, sr1_no) != titers2.titer(ag2_no, sr2_no))
                    return false;
            }
        }
        return true;
    };

    std::vector<std::vector<size_t>> result;
    std::vector<size_t> processed;
    for (auto no1 : range_from_0_to(charts.size())) {
        if (ranges::find(processed, no1) == ranges::end(processed)) {
            std::vector<size_t> group;
            auto antigens1 = charts[no1]->antigens();
            const auto antigens1_names = get_reference_names(*antigens1);
            // AD_DEBUG("AG {}", antigens1_names);
            auto sera1 = charts[no1]->sera();
            const auto sera1_names = get_all_names(*sera1);
            // AD_DEBUG("SR {}", sera1_names);

            for (auto no2 : range_from_to(no1 + 1, charts.size())) {
                if (ranges::find(processed, no2) == ranges::end(processed)) {

                    auto antigens2 = charts[no2]->antigens();
                    const auto antigens2_names = get_reference_names(*antigens2);
                    auto sera2 = charts[no2]->sera();
                    const auto sera2_names = get_all_names(*sera2);

                    if (antigens1_names == antigens2_names && sera1_names == sera2_names &&
                        titers_same(*charts[no1]->titers(), antigens1_names, sera1_names, *charts[no2]->titers(), antigens2_names, sera2_names)) {
                        if (group.empty())
                            group.push_back(no1);
                        group.push_back(no2);
                        processed.push_back(no2);
                    }
                }
            }
            processed.push_back(no1);
            if (!group.empty())
                result.push_back(std::move(group));
        }
    }
    return result;

} // get_cheating_assays

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
