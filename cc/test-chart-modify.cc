#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <array>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/temp-file.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-export.hh"

static void test_chart_modify_no_changes(acmacs::chart::ChartP chart, const argc_argv& args, report_time report);
static void test_modify_titers(acmacs::chart::ChartP chart, const argc_argv& args, report_time report);
static void test_dont_care_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, const argc_argv& args, report_time report);
static void test_dont_care_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, const argc_argv& args, report_time report);
static void test_multiply_by_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, double aMult, const argc_argv& args, report_time report);
static void test_multiply_by_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, double aMult, const argc_argv& args, report_time report);
static void test_remove_antigens(acmacs::chart::ChartP chart, const acmacs::Indexes& indexes, const argc_argv& args, report_time report);
static void test_remove_sera(acmacs::chart::ChartP chart, const acmacs::Indexes& indexes, const argc_argv& args, report_time report);
static void test_insert_antigen(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report);
static void test_insert_serum(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report);
static void test_insert_remove_antigen(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report);
static void test_insert_remove_serum(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report);
static void test_extensions(acmacs::chart::ChartP chart, const argc_argv& args, report_time report);

enum class compare_titers { no, yes };
static void compare_antigens(acmacs::chart::ChartP chart_source, size_t source_ag_no, acmacs::chart::AntigenP source_antigen, acmacs::chart::ChartP chart_imported, size_t imported_ag_no, compare_titers ct);
static void compare_sera(acmacs::chart::ChartP chart_source, size_t source_sr_no, acmacs::chart::SerumP source_serum, size_t source_number_of_antigens, acmacs::chart::ChartP chart_imported, size_t imported_sr_no, size_t imported_number_of_antigens, compare_titers ct);

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {{"--full", false, "full output"}, {"--time", false, "report time of loading chart"}, {"-h", false}, {"--help", false}, {"-v", false}, {"--verbose", false}});
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            const std::array<size_t, 5> antigens_to_test{{0, 1, chart->number_of_antigens() / 2, chart->number_of_antigens() - 2, chart->number_of_antigens() - 1}};
            const std::array<size_t, 5> sera_to_test{{0, 1, chart->number_of_sera() / 2, chart->number_of_sera() - 2, chart->number_of_sera() - 1}};
            test_chart_modify_no_changes(chart, args, report);
            if (chart->titers()->number_of_layers() == 0) {
                std::cout << "  test_modify_titers\n";
                test_modify_titers(chart, args, report);
                std::cout << "  test_dont_care_for_antigen\n";
                for (auto ag_no : antigens_to_test)
                    test_dont_care_for_antigen(chart, ag_no, args, report);
                std::cout << "  test_dont_care_for_serum\n";
                for (auto sr_no : sera_to_test)
                    test_dont_care_for_serum(chart, sr_no, args, report);
                std::cout << "  test_multiply_by_for_antigen\n";
                for (auto ag_no : antigens_to_test) {
                    test_multiply_by_for_antigen(chart, ag_no, 2.0, args, report);
                    test_multiply_by_for_antigen(chart, ag_no, 0.5, args, report);
                }
                std::cout << "  test_multiply_by_for_serum\n";
                for (auto sr_no : sera_to_test) {
                    test_multiply_by_for_serum(chart, sr_no, 2.0, args, report);
                    test_multiply_by_for_serum(chart, sr_no, 0.5, args, report);
                }
                std::cout << "  test_insert_antigen\n";
                for (auto ag_no : antigens_to_test) {
                    test_insert_antigen(chart, ag_no, args, report);
                    test_insert_antigen(chart, ag_no + 1, args, report);
                    test_insert_remove_antigen(chart, ag_no, args, report);
                    test_insert_remove_antigen(chart, ag_no + 1, args, report);
                }
                std::cout << "  test_insert_serum\n";
                for (auto sr_no : sera_to_test) {
                    test_insert_serum(chart, sr_no, args, report);
                    test_insert_serum(chart, sr_no + 1, args, report);
                    test_insert_remove_serum(chart, sr_no, args, report);
                    test_insert_remove_serum(chart, sr_no + 1, args, report);
                }
            }
            std::cout << "  test_remove_antigens\n";
            for (auto ag_no : antigens_to_test)
                test_remove_antigens(chart, {ag_no}, args, report);
            std::cout << "  test_remove_sera\n";
            for (auto sr_no : sera_to_test)
                test_remove_sera(chart, {sr_no}, args, report);
            std::cout << "  test_extensions\n";
            test_extensions(chart, args, report);
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void test_chart_modify_no_changes(acmacs::chart::ChartP chart, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.info_modify();
    chart_modify.antigens_modify();
    chart_modify.sera_modify();
    chart_modify.titers_modify();
    chart_modify.forced_column_bases_modify(acmacs::chart::MinimumColumnBasis{});

    const auto plain = acmacs::chart::export_factory(*chart, acmacs::chart::export_format::ace, args.program(), report);
    const auto modified = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    if (plain != modified) {
        if (args["--full"]) {
            std::cout << "======== PLAIN ============" << plain << '\n';
            std::cout << "======== MODIFIED ============" << modified << '\n';
        }
        else {
            acmacs::file::temp plain_file{".ace"}, modified_file{".ace"};
            if (write(plain_file, plain.data(), plain.size()) < 0)
                throw std::runtime_error("write plain_file failed!");
            if (write(modified_file, modified.data(), modified.size()) < 0)
                throw std::runtime_error("write modified_file failed!");
            if (std::system(("/usr/bin/diff -B -b --ignore-matching-lines='\"?created\"' " + static_cast<std::string>(plain_file) + " " + static_cast<std::string>(modified_file)).data()))
                throw std::runtime_error("diff failed!");
        }
        throw std::runtime_error("different!");
    }

} // test_chart_modify_no_changes

// ----------------------------------------------------------------------

void test_modify_titers(acmacs::chart::ChartP chart, const argc_argv& args, report_time report)
{
    struct ME
    {
        size_t ag_no, sr_no;
        const char* titer;
    };
    const std::array<ME, 4> test_data{{{0, 1, "11"}, {0, 2, "12"}, {1, 1, "21"}, {1, 2, "<22"}}};

    acmacs::chart::ChartModify chart_modify{chart};
    auto& titers = chart_modify.titers_modify();
    for (const auto& new_t : test_data)
        titers.titer(new_t.ag_no, new_t.sr_no, acmacs::chart::Titer{new_t.titer});
    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};
    const auto tim_source = titers_source->titers_existing(), tim_modified = titers_modified->titers_existing();
    auto ti_source = tim_source.begin(), ti_modified = tim_modified.begin();
    for (; ti_source != tim_source.end(); ++ti_source, ++ti_modified) {
        if (ti_source->antigen != ti_modified->antigen || ti_source->serum != ti_modified->serum)
            throw std::runtime_error{fmt::format("test_modify_titers: titer iterator mismatch: [{}] vs. [{}]", *ti_source, *ti_modified)};
        if (auto found = std::find_if(test_data.begin(), test_data.end(), [ag_no=ti_source->antigen, sr_no=ti_source->serum](const auto& entry) { return ag_no == entry.ag_no && sr_no == entry.sr_no; }); found != test_data.end()) {
            if (ti_source->titer == ti_modified->titer)
                throw std::runtime_error{fmt::format("test_modify_titers: unexpected titer match: [{}] vs. [{}]", *ti_source, *ti_modified)};
            if (ti_modified->titer != acmacs::chart::Titer{found->titer})
                throw std::runtime_error{fmt::format("titer mismatch: [{}] vs. [{}]", found->titer, *ti_modified)};
        }
        else if (ti_source->titer != ti_modified->titer)
            throw std::runtime_error{fmt::format("test_modify_titers: titer mismatch: [{}] vs. [{}]", *ti_source, *ti_modified)};
    }
    if (ti_modified != tim_modified.end())
        throw std::runtime_error("test_modify_titers: titer iterator end mismatch");

} // test_modify_titers

// ----------------------------------------------------------------------

void test_dont_care_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify().dontcare_for_antigen(aAntigenNo);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

    for (auto ti_source : titers_source->titers_existing()) {
        if (ti_source.antigen == aAntigenNo) {
            if (!titers_modified->titer(ti_source.antigen, ti_source.serum).is_dont_care())
                throw std::runtime_error{fmt::format("test_dont_care_for_antigen: unexpected titer: [{}], expected: *", ti_source)};
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error(fmt::format("test_dont_care_for_antigen: titer mismatch: [{} {}] vs. {}", ti_source.antigen, ti_source.serum, *titers_modified->titer(ti_source.antigen, ti_source.serum)));
    }

} // test_dont_care_for_antigen

// ----------------------------------------------------------------------

void test_dont_care_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify().dontcare_for_serum(aSerumNo);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

    for (auto ti_source : titers_source->titers_existing()) {
        if (ti_source.serum == aSerumNo) {
            if (!titers_modified->titer(ti_source.antigen, ti_source.serum).is_dont_care())
                throw std::runtime_error{fmt::format("test_dont_care_for_serum: unexpected titer: [{}], expected: *", ti_source)};
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error(fmt::format("test_dont_care_for_serum: titer mismatch: [{} {}] vs. {}", ti_source.antigen, ti_source.serum, *titers_modified->titer(ti_source.antigen, ti_source.serum)));
    }

} // test_dont_care_for_serum

// ----------------------------------------------------------------------

void test_multiply_by_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, double aMult, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify().multiply_by_for_antigen(aAntigenNo, aMult);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source = chart->titers(), titers_modified = imported->titers();

    for (auto ti_source : titers_source->titers_existing()) {
        if (ti_source.antigen == aAntigenNo && !ti_source.titer.is_dont_care()) {
            const auto expected_value = static_cast<size_t>(std::lround(static_cast<double>(ti_source.titer.value()) * aMult));
            if (titers_modified->titer(ti_source.antigen, ti_source.serum).value() != expected_value)
                throw std::runtime_error(fmt::format("test_multiply_by_for_antigen: unexpected titer: [ag:{} sr:{} t:{}], expected: {}", ti_source.antigen, ti_source.serum,
                                                     *titers_modified->titer(ti_source.antigen, ti_source.serum), expected_value));
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error(fmt::format("test_multiply_by_for_antigen: titer mismatch: [{} {}] vs. {}", ti_source.antigen, ti_source.serum, *titers_modified->titer(ti_source.antigen, ti_source.serum)));
    }

} // test_multiply_by_for_antigen

// ----------------------------------------------------------------------

void test_multiply_by_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, double aMult, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify().multiply_by_for_serum(aSerumNo, aMult);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

    for (auto ti_source : titers_source->titers_existing()) {
        if (ti_source.serum == aSerumNo && !ti_source.titer.is_dont_care()) {
            const auto expected_value = static_cast<size_t>(std::lround(static_cast<double>(ti_source.titer.value()) * aMult));
            if (titers_modified->titer(ti_source.antigen, ti_source.serum).value() != expected_value)
                throw std::runtime_error(fmt::format("test_multiply_by_for_serum: unexpected titer: [ag:{} sr:{} t:{}], expected: {}", ti_source.antigen, ti_source.serum,
                                                     *titers_modified->titer(ti_source.antigen, ti_source.serum), std::to_string(expected_value)));
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error(fmt::format("test_multiply_by_for_serum: titer mismatch: [{} {}] vs. {}", ti_source.antigen, ti_source.serum, *titers_modified->titer(ti_source.antigen, ti_source.serum)));
    }

} // test_multiply_by_for_serum

// ----------------------------------------------------------------------

void test_remove_antigens(acmacs::chart::ChartP chart, const acmacs::Indexes& indexes, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.remove_antigens(acmacs::ReverseSortedIndexes(indexes));

    // for (auto ag_no : acmacs::range(chart_modify.number_of_antigens())) {
    //     std::cerr << "plot_style orig: " << std::setw(2) << (ag_no + 1) << ' ' << chart->plot_spec()->style(ag_no + 1) << '\n';
    //     std::cerr << "plot_style mod:  " << std::setw(2) << ag_no << ' ' << chart_modify.plot_spec()->style(ag_no) << '\n' << '\n';
    // }

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);

    try {
        auto antigens_source = chart->antigens(), antigens_imported = imported->antigens();
        auto sera_source = chart->sera(), sera_imported = imported->sera();
        auto titers_source = chart->titers(), titers_imported = imported->titers();
        auto projections_source = chart->projections(), projections_imported = imported->projections();
        auto plot_spec_source = chart->plot_spec(), plot_spec_imported = imported->plot_spec();

        size_t imported_ag_no = 0;
        for (auto[source_ag_no, source_antigen] : acmacs::enumerate(*antigens_source)) {
            if (std::find(indexes.begin(), indexes.end(), source_ag_no) == indexes.end()) {
                compare_antigens(chart, source_ag_no, source_antigen, imported, imported_ag_no, compare_titers::yes);
                ++imported_ag_no;
            }
        }
        if (imported_ag_no != imported->number_of_antigens())
            throw std::runtime_error("invalid resulting imported_ag_no");

        for (auto[source_sr_no, source_serum] : acmacs::enumerate(*sera_source))
            compare_sera(chart, source_sr_no, source_serum, antigens_source->size(), imported, source_sr_no, antigens_imported->size(), compare_titers::no);
    }
    catch (std::exception& err) {
        // const std::string prefix = fs::exists("/r/ramdisk-id") ? "/r/" : "/tmp/";
        // acmacs::file::write(prefix + "a.ace", exported, acmacs::file::force_compression::yes);
        throw std::runtime_error(fmt::format("test_remove_antigens: {}\n  indexes:{}", err, indexes));
    }

} // test_remove_antigens

// ----------------------------------------------------------------------

void test_remove_sera(acmacs::chart::ChartP chart, const acmacs::Indexes& indexes, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.remove_sera(acmacs::ReverseSortedIndexes(indexes));

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);

    try {
        auto antigens_source = chart->antigens(), antigens_imported = imported->antigens();
        auto sera_source = chart->sera(), sera_imported = imported->sera();
        auto titers_source = chart->titers(), titers_imported = imported->titers();
        auto projections_source = chart->projections(), projections_imported = imported->projections();
        auto plot_spec_source = chart->plot_spec(), plot_spec_imported = imported->plot_spec();

        for (auto[source_ag_no, source_antigen] : acmacs::enumerate(*antigens_source))
            compare_antigens(chart, source_ag_no, source_antigen, imported, source_ag_no, compare_titers::no);

        size_t imported_sr_no = 0;
        for (auto[source_sr_no, source_serum] : acmacs::enumerate(*sera_source)) {
            if (std::find(indexes.begin(), indexes.end(), source_sr_no) == indexes.end()) {
                compare_sera(chart, source_sr_no, source_serum, antigens_source->size(), imported, imported_sr_no, antigens_imported->size(), compare_titers::yes);
                ++imported_sr_no;
            }
        }
        if (imported_sr_no != imported->number_of_sera())
            throw std::runtime_error("invalid resulting imported_sr_no");
    }
    catch (std::exception& err) {
        // const std::string prefix = fs::exists("/r/ramdisk-id") ? "/r/" : "/tmp/";
        // acmacs::file::write(prefix + "a.ace", exported, acmacs::file::force_compression::yes);
        throw std::runtime_error(fmt::format("test_remove_sera: {}\n  indexes:{}", err, indexes));
    }

} // test_remove_sera

// ----------------------------------------------------------------------

void test_insert_antigen(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    std::string exported;
    try {
        acmacs::chart::ChartModify chart_modify{chart};
        chart_modify.insert_antigen(before);

        exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
        auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);

        auto antigens_source = chart->antigens(), antigens_imported = imported->antigens();
        auto sera_source = chart->sera(), sera_imported = imported->sera();
        auto titers_source = chart->titers(), titers_imported = imported->titers();
        auto projections_source = chart->projections(), projections_imported = imported->projections();
        auto plot_spec_source = chart->plot_spec(), plot_spec_imported = imported->plot_spec();

        auto check_inserted = [&](size_t imported_ag_no) {
            if (auto new_name = (*antigens_imported)[imported_ag_no]->name_full(); new_name.empty())
                throw std::runtime_error("inserted antigen has no name");
            for (auto sr_no : acmacs::range(sera_source->size())) {
                if (!titers_imported->titer(imported_ag_no, sr_no).is_dont_care())
                    throw std::runtime_error{AD_FORMAT("inserted antigen has titer: sr_no:{} titer:{}", sr_no, *titers_imported->titer(imported_ag_no, sr_no))};
            }
            for (auto projection : *projections_imported) {
                if (auto imp = projection->layout()->at(imported_ag_no); imp.exists())
                    throw std::runtime_error{AD_FORMAT("inserted antigen has coordinates: {}", imp)};
            }
        };

        size_t imported_ag_no = 0;
        for (auto [source_ag_no, source_antigen] : acmacs::enumerate(*antigens_source)) {
            if (source_ag_no == before) {
                check_inserted(imported_ag_no);
                ++imported_ag_no;
            }
            else {
                compare_antigens(chart, source_ag_no, source_antigen, imported, imported_ag_no, compare_titers::yes);
            }
            ++imported_ag_no;
        }
        if (imported_ag_no == before) {
            check_inserted(imported_ag_no);
            ++imported_ag_no;
        }
        if (imported_ag_no != imported->number_of_antigens())
            throw std::runtime_error{AD_FORMAT("invalid resulting imported_ag_no: {}, expected: ", imported_ag_no, imported->number_of_antigens())};

        for (auto [source_sr_no, source_serum] : acmacs::enumerate(*sera_source))
            compare_sera(chart, source_sr_no, source_serum, antigens_source->size(), imported, source_sr_no, antigens_imported->size(), compare_titers::no);
    }
    catch (std::exception& err) {
        // const std::string prefix = fs::exists("/r/ramdisk-id") ? "/r/" : "/tmp/";
        // acmacs::file::write(prefix + "a.ace", exported, acmacs::file::force_compression::yes);
        throw std::runtime_error(fmt::format("test_insert_antigen: {}\n  before:{}", err, before));
    }

} // test_insert_antigen

// ----------------------------------------------------------------------

void test_insert_serum(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_serum(before);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);

    try {
        auto antigens_source = chart->antigens(), antigens_imported = imported->antigens();
        auto sera_source = chart->sera(), sera_imported = imported->sera();
        auto titers_source = chart->titers(), titers_imported = imported->titers();
        auto projections_source = chart->projections(), projections_imported = imported->projections();
        auto plot_spec_source = chart->plot_spec(), plot_spec_imported = imported->plot_spec();

        auto check_inserted = [&](size_t imported_sr_no) {
            if (auto new_name = (*sera_imported)[imported_sr_no]->name_full(); new_name.empty())
                throw std::runtime_error("inserted serum has no name");
            for (auto ag_no : acmacs::range(antigens_source->size())) {
                if (!titers_imported->titer(ag_no, imported_sr_no).is_dont_care())
                    throw std::runtime_error{AD_FORMAT("inserted serum has titer: ag_no:{} titer:{}", ag_no, *titers_imported->titer(ag_no, imported_sr_no))};
            }
            for (auto projection : *projections_imported) {
                if (auto imp = projection->layout()->at(imported_sr_no + antigens_source->size()); imp.exists())
                    throw std::runtime_error{AD_FORMAT("inserted serum has coordinates: ", imp)};
            }
        };

        for (auto [source_ag_no, source_antigen] : acmacs::enumerate(*antigens_source))
            compare_antigens(chart, source_ag_no, source_antigen, imported, source_ag_no, compare_titers::no);

        size_t imported_sr_no = 0;
        for (auto [source_sr_no, source_serum] : acmacs::enumerate(*sera_source)) {
            if (source_sr_no == before) {
                check_inserted(imported_sr_no);
                ++imported_sr_no;
            }
            else {
                compare_sera(chart, source_sr_no, source_serum, antigens_source->size(), imported, imported_sr_no, antigens_imported->size(), compare_titers::yes);
            }
            ++imported_sr_no;
        }
        if (imported_sr_no == before) {
            check_inserted(imported_sr_no);
            ++imported_sr_no;
        }
        if (imported_sr_no != imported->number_of_sera())
            throw std::runtime_error("invalid resulting imported_sr_no: " + acmacs::to_string(imported_sr_no) + ", expected: " + acmacs::to_string(imported->number_of_sera()));
    }
    catch (std::exception& err) {
        // const std::string prefix = fs::exists("/r/ramdisk-id") ? "/r/" : "/tmp/";
        // acmacs::file::write(prefix + "a.ace", exported, acmacs::file::force_compression::yes);
        throw std::runtime_error(std::string("test_insert_serum: ") + err.what() + "\n  before:" + acmacs::to_string(before));
    }

} // test_insert_serum

// ----------------------------------------------------------------------

inline std::string equality_report(const acmacs::PointStyle& s1, const acmacs::PointStyle& s2)
{
    return fmt::format("shown:{} fill:{} outline:{} outline_width:{} size:{} rotation:{} aspect:{} shape:{} label:{} label_text:{}", s1.shown() == s2.shown(), s1.fill() == s2.fill(),
                       s1.outline() == s2.outline(), s1.outline_width() == s2.outline_width(), s1.size() == s2.size(), s1.rotation() == s2.rotation(), s1.aspect() == s2.aspect(),
                       s1.shape() == s2.shape(), s1.label() == s2.label(), s1.label_text() == s2.label_text());
}

// ----------------------------------------------------------------------

void compare_antigens(acmacs::chart::ChartP chart_source, size_t source_ag_no, acmacs::chart::AntigenP source_antigen, acmacs::chart::ChartP chart_imported, size_t imported_ag_no, compare_titers ct)
{
    auto antigens_imported = chart_imported->antigens();
    auto projections_source = chart_source->projections(), projections_imported = chart_imported->projections();
    auto plot_spec_source = chart_source->plot_spec(), plot_spec_imported = chart_imported->plot_spec();

    if (*source_antigen != *(*antigens_imported)[imported_ag_no])
        throw std::runtime_error("antigen mismatch: orig:" + std::to_string(source_ag_no) + " vs. imported:" + std::to_string(imported_ag_no));
    if (ct == compare_titers::yes) {
        auto titers_source = chart_source->titers(), titers_imported = chart_imported->titers();
        for (auto sr_no : acmacs::range(chart_source->number_of_sera())) {
            if (titers_source->titer(source_ag_no, sr_no) != titers_imported->titer(imported_ag_no, sr_no))
                throw std::runtime_error(fmt::format("titer mismatch: sr_no:{} orig: {} {}, imported: {} {}", sr_no, source_ag_no, *titers_source->titer(source_ag_no, sr_no),
                                                     imported_ag_no, *titers_imported->titer(imported_ag_no, sr_no)));
        }
    }
    for (auto[p_no, projection] : acmacs::enumerate(*projections_source)) {
        if (auto src = projection->layout()->at(source_ag_no), imp = (*projections_imported)[p_no]->layout()->at(imported_ag_no); src != imp)
            throw std::runtime_error(fmt::format("antigen coordinates mismatch: orig:{} {} vs. imported:", source_ag_no, src, imported_ag_no, imp));
    }
    if (auto src = plot_spec_source->style(source_ag_no), imp = plot_spec_imported->style(imported_ag_no); src != imp)
        throw std::runtime_error(fmt::format("antigen plot style mismatch:\n     orig:{} {}\n imported:{} {}\n  report: {}", source_ag_no, src, imported_ag_no, imp, equality_report(src, imp)));

} // void compare_antigens

// ----------------------------------------------------------------------

void compare_sera(acmacs::chart::ChartP chart_source, size_t source_sr_no, acmacs::chart::SerumP source_serum, size_t source_number_of_antigens, acmacs::chart::ChartP chart_imported,
                  size_t imported_sr_no, size_t imported_number_of_antigens, compare_titers ct)
{
    auto sera_imported = chart_imported->sera();
    auto projections_source = chart_source->projections(), projections_imported = chart_imported->projections();
    auto plot_spec_source = chart_source->plot_spec(), plot_spec_imported = chart_imported->plot_spec();

    if (*source_serum != *(*sera_imported)[imported_sr_no])
        throw std::runtime_error("serum mismatch: " + std::to_string(source_sr_no) + " vs. " + std::to_string(imported_sr_no));
    if (ct == compare_titers::yes) {
        auto titers_source = chart_source->titers(), titers_imported = chart_imported->titers();
        for (auto ag_no : acmacs::range(source_number_of_antigens)) {
            if (titers_source->titer(ag_no, source_sr_no) != titers_imported->titer(ag_no, imported_sr_no))
                throw std::runtime_error(fmt::format("titer mismatch: ag_no:{} orig: {} {}, imported: {} {}", ag_no, source_sr_no, *titers_source->titer(ag_no, source_sr_no),
                                                     imported_sr_no, *titers_imported->titer(ag_no, imported_sr_no)));
        }
    }
    const auto point_no_source = source_sr_no + source_number_of_antigens, point_no_imported = imported_sr_no + imported_number_of_antigens;
    for (auto[p_no, projection] : acmacs::enumerate(*projections_source)) {
        if (auto src = projection->layout()->at(point_no_source), imp = (*projections_imported)[p_no]->layout()->at(point_no_imported); src != imp)
            throw std::runtime_error(fmt::format("serum coordinates mismatch: orig:{} {} vs. imported:", source_sr_no, src, imported_sr_no, imp));
    }
    if (auto src = plot_spec_source->style(point_no_source), imp = plot_spec_imported->style(point_no_imported); src != imp)
        throw std::runtime_error(fmt::format("serum plot style mismatch:\n     orig:{} {}\n imported:{} {}\n  report: {}", source_sr_no, src, imported_sr_no, imp, equality_report(src, imp)));

} // compare_sera

// ----------------------------------------------------------------------

void test_insert_remove_antigen(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_antigen(before);
    chart_modify.remove_antigens(acmacs::ReverseSortedIndexes(std::vector<size_t>{before}));

    const auto source_exported = acmacs::chart::export_factory(*chart, acmacs::chart::export_format::text, args.program(), report);
    const auto modified_exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::text, args.program(), report);
    if (source_exported != modified_exported) {
        acmacs::file::write("/tmp/source.txt", source_exported, acmacs::file::force_compression::no);
        acmacs::file::write("/tmp/modified.txt", modified_exported, acmacs::file::force_compression::no);
        throw std::runtime_error(fmt::format("test_insert_remove_antigen: exported chart difference, diff /tmp/source.txt /tmp/modified.txt\n  before: {}", before));
    }

} // test_insert_remove_antigen

// ----------------------------------------------------------------------

void test_insert_remove_serum(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_serum(before);
    chart_modify.remove_sera(acmacs::ReverseSortedIndexes(acmacs::Indexes{before}));

    const auto source_exported = acmacs::chart::export_factory(*chart, acmacs::chart::export_format::text, args.program(), report);
    const auto modified_exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::text, args.program(), report);
    if (source_exported != modified_exported) {
        acmacs::file::write("/tmp/source.txt", source_exported, acmacs::file::force_compression::no);
        acmacs::file::write("/tmp/modified.txt", modified_exported, acmacs::file::force_compression::no);
        throw std::runtime_error(fmt::format("test_insert_remove_serum:: exported chart difference, (no ediff!) diff /tmp/source.txt /tmp/modified.txt\n  before: {}", before));
    }

} // test_insert_remove_serum

// ----------------------------------------------------------------------

void test_extensions(acmacs::chart::ChartP chart, const argc_argv& args, report_time report)
{
    {
        acmacs::chart::ChartModify chart_modify{chart};
        if (const auto& ext1 = chart_modify.extension_field("test-chart-modify"); !ext1.is_const_null())
            throw std::runtime_error("test_extensions: initial test-chart-modify value is not const_null");
        if (const auto& ext2 = chart_modify.extension_field_modify("test-chart-modify"); !ext2.is_const_null())
            throw std::runtime_error("test_extensions: initial modifiy test-chart-modify value is not const_null");
    }

    {
        acmacs::chart::ChartModify chart_modify{chart};
        const rjson::value test_value_1{rjson::object{{"A", 1}, {"B", 2.1}, {"C", "D"}}};
        chart_modify.extension_field_modify("test-chart-modify-1", test_value_1);
        const rjson::value test_value_2{rjson::object{{"E", rjson::array{11, "F", 12.12}}}};
        chart_modify.extension_field_modify("test-chart-modify-2", test_value_2);
        // std::cerr << "EXT: " << chart_modify.extension_fields() << '\n';
        // std::cerr << "EXT1: " << chart_modify.extension_field("test-chart-modify-1") << '\n';
        // std::cerr << "EXT2: " << chart_modify.extension_field("test-chart-modify-2") << '\n';
        if (const auto& r1 = chart_modify.extension_field("test-chart-modify-1"); r1 != test_value_1)
            throw std::runtime_error("test_extensions: r1 != test_value_1");
        if (const auto& r2 = chart_modify.extension_field("test-chart-modify-2"); r2 != test_value_2)
            throw std::runtime_error("test_extensions: r2 != test_value_2");

        // replace
        const rjson::value test_value_3{rjson::array{rjson::object{{"G", "GG"}}, "HHHH", 123.123}};
        chart_modify.extension_field_modify("test-chart-modify-2", test_value_3);
        // std::cerr << "EXT: " << chart_modify.extension_fields() << '\n';
        if (const auto& r3 = chart_modify.extension_field("test-chart-modify-2"); r3 != test_value_3)
            throw std::runtime_error("test_extensions: r3 != test_value_3");

        const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
        auto chart_imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
        // std::cerr << "EXT imported: " << chart_imported->extension_fields() << '\n';
        if (const auto& r4 = chart_imported->extension_field("test-chart-modify-2"); r4 != test_value_3)
            throw std::runtime_error("test_extensions: r4 != test_value_3");
    }

} // test_extensions

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
