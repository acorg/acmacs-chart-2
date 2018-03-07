#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <array>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/read-file.hh"
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
    chart_modify.forced_column_bases_modify();

    const auto plain = acmacs::chart::export_factory(*chart, acmacs::chart::export_format::ace, args.program(), report);
    const auto modified = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    if (plain != modified) {
        if (args["--full"]) {
            std::cout << "======== PLAIN ============" << plain << '\n';
            std::cout << "======== MODIFIED ============" << modified << '\n';
        }
        else {
            acmacs::file::temp plain_file{".ace"}, modified_file{".ace"};
            write(plain_file, plain.data(), plain.size());
            write(modified_file, modified.data(), modified.size());
            std::system(("/usr/bin/diff -B -b " + static_cast<std::string>(plain_file) + " " + static_cast<std::string>(modified_file)).data());
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
    auto titers = chart_modify.titers_modify();
    for (const auto& new_t : test_data)
        titers->titer(new_t.ag_no, new_t.sr_no, new_t.titer);
    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};
    auto ti_source = titers_source->begin(), ti_modified = titers_modified->begin();
    for (; ti_source != titers_source->end(); ++ti_source, ++ti_modified) {
        if (ti_source->antigen != ti_modified->antigen || ti_source->serum != ti_modified->serum)
            throw std::runtime_error("test_modify_titers: titer iterator mismatch: [" + acmacs::to_string(*ti_source) + "] vs. [" + acmacs::to_string(*ti_modified) + ']');
        if (auto found = std::find_if(test_data.begin(), test_data.end(), [ag_no=ti_source->antigen, sr_no=ti_source->serum](const auto& entry) { return ag_no == entry.ag_no && sr_no == entry.sr_no; }); found != test_data.end()) {
            if (ti_source->titer == ti_modified->titer)
                throw std::runtime_error("test_modify_titers: unexpected titer match: [" + acmacs::to_string(*ti_source) + "] vs. [" + acmacs::to_string(*ti_modified) + ']');
            if (ti_modified->titer != found->titer)
                throw std::runtime_error("titer mismatch: [" + std::string(found->titer) + "] vs. [" + acmacs::to_string(*ti_modified) + ']');
        }
        else if (ti_source->titer != ti_modified->titer)
            throw std::runtime_error("test_modify_titers: titer mismatch: [" + acmacs::to_string(*ti_source) + "] vs. [" + acmacs::to_string(*ti_modified) + ']');
    }
    if (ti_modified != titers_modified->end())
        throw std::runtime_error("test_modify_titers: titer iterator end mismatch");

} // test_modify_titers

// ----------------------------------------------------------------------

void test_dont_care_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify()->dontcare_for_antigen(aAntigenNo);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

    for (auto ti_source : *titers_source) {
        if (ti_source.antigen == aAntigenNo) {
            if (!titers_modified->titer(ti_source.antigen, ti_source.serum).is_dont_care())
                throw std::runtime_error("test_dont_care_for_antigen: unexpected titer: [" + acmacs::to_string(ti_source) + "], expected: *");
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error("test_dont_care_for_antigen: titer mismatch: [" + acmacs::to_string(ti_source) + "] vs. " + titers_modified->titer(ti_source.antigen, ti_source.serum));
    }

} // test_dont_care_for_antigen

// ----------------------------------------------------------------------

void test_dont_care_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify()->dontcare_for_serum(aSerumNo);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

    for (auto ti_source : *titers_source) {
        if (ti_source.serum == aSerumNo) {
            if (!titers_modified->titer(ti_source.antigen, ti_source.serum).is_dont_care())
                throw std::runtime_error("test_dont_care_for_serum: unexpected titer: [" + acmacs::to_string(ti_source) + "], expected: *");
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error("test_dont_care_for_serum: titer mismatch: [" + acmacs::to_string(ti_source) + "] vs. " + titers_modified->titer(ti_source.antigen, ti_source.serum));
    }

} // test_dont_care_for_serum

// ----------------------------------------------------------------------

void test_multiply_by_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, double aMult, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify()->multiply_by_for_antigen(aAntigenNo, aMult);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source = chart->titers(), titers_modified = imported->titers();

    for (auto ti_source : *titers_source) {
        if (ti_source.antigen == aAntigenNo && !ti_source.titer.is_dont_care()) {
            const auto expected_value = static_cast<size_t>(std::lround(ti_source.titer.value() * aMult));
            if (titers_modified->titer(ti_source.antigen, ti_source.serum).value() != expected_value)
                throw std::runtime_error("test_multiply_by_for_antigen: unexpected titer: [ag:" + std::to_string(ti_source.antigen) + " sr:" + std::to_string(ti_source.serum) +
                                         " t:" + titers_modified->titer(ti_source.antigen, ti_source.serum) + "], expected: " + std::to_string(expected_value));
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error("test_multiply_by_for_antigen: titer mismatch: [" + acmacs::to_string(ti_source) + "] vs. " + titers_modified->titer(ti_source.antigen, ti_source.serum));
    }

} // test_multiply_by_for_antigen

// ----------------------------------------------------------------------

void test_multiply_by_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, double aMult, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.titers_modify()->multiply_by_for_serum(aSerumNo, aMult);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

    for (auto ti_source : *titers_source) {
        if (ti_source.serum == aSerumNo && !ti_source.titer.is_dont_care()) {
            const auto expected_value = static_cast<size_t>(std::lround(ti_source.titer.value() * aMult));
            if (titers_modified->titer(ti_source.antigen, ti_source.serum).value() != expected_value)
                throw std::runtime_error("test_multiply_by_for_serum: unexpected titer: [ag:" + std::to_string(ti_source.antigen) + " sr:" + std::to_string(ti_source.serum) +
                                         " t:" + titers_modified->titer(ti_source.antigen, ti_source.serum) + "], expected: " + std::to_string(expected_value));
        }
        else if (titers_modified->titer(ti_source.antigen, ti_source.serum) != ti_source.titer)
            throw std::runtime_error("test_multiply_by_for_serum: titer mismatch: [" + acmacs::to_string(ti_source) + "] vs. " + titers_modified->titer(ti_source.antigen, ti_source.serum));
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
        // acmacs::file::write("/r/a.ace", exported, acmacs::file::ForceCompression::Yes);
        throw std::runtime_error(std::string("test_remove_antigens: ") + err.what() + "\n  indexes:" + acmacs::to_string(indexes));
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
        // acmacs::file::write("/r/a.ace", exported, acmacs::file::ForceCompression::Yes);
        throw std::runtime_error(std::string("test_remove_sera: ") + err.what() + "\n  indexes:" + acmacs::to_string(indexes));
    }

} // test_remove_sera

// ----------------------------------------------------------------------

void test_insert_antigen(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_antigen(before);

    const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);

    try {
        auto antigens_source = chart->antigens(), antigens_imported = imported->antigens();
        auto sera_source = chart->sera(), sera_imported = imported->sera();
        auto titers_source = chart->titers(), titers_imported = imported->titers();
        auto projections_source = chart->projections(), projections_imported = imported->projections();
        auto plot_spec_source = chart->plot_spec(), plot_spec_imported = imported->plot_spec();

        auto check_inserted = [&](size_t imported_ag_no) {
            if (auto new_name = (*antigens_imported)[imported_ag_no]->full_name(); !new_name.empty())
                throw std::runtime_error("inserted antigen has name: " + new_name);
            for (auto sr_no : acmacs::range(chart->number_of_sera())) {
                if (!titers_imported->titer(imported_ag_no, sr_no).is_dont_care())
                    throw std::runtime_error("inserted antigen has titer: sr_no:" + std::to_string(sr_no) + " titer:" + titers_imported->titer(imported_ag_no, sr_no));
            }
            for (auto[p_no, projection] : acmacs::enumerate(*projections_source)) {
                if (auto imp = (*projections_imported)[p_no]->layout()->get(imported_ag_no); imp.not_nan())
                    throw std::runtime_error("inserted antigen has coordinates: " + acmacs::to_string(imp));
            }
        };

        size_t imported_ag_no = 0;
        for (auto[source_ag_no, source_antigen] : acmacs::enumerate(*antigens_source)) {
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
            throw std::runtime_error("invalid resulting imported_ag_no: " + acmacs::to_string(imported_ag_no) + ", expected: " + acmacs::to_string(imported->number_of_antigens()));

        for (auto[source_sr_no, source_serum] : acmacs::enumerate(*sera_source))
            compare_sera(chart, source_sr_no, source_serum, antigens_source->size(), imported, source_sr_no, antigens_imported->size(), compare_titers::no);
    }
    catch (std::exception& err) {
        // acmacs::file::write("/r/a.ace", exported, acmacs::file::ForceCompression::Yes);
        throw std::runtime_error(std::string("test_insert_antigen: ") + err.what() + "\n  before:" + acmacs::to_string(before));
    }

} // test_insert_antigen

// ----------------------------------------------------------------------

void test_insert_serum(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_serum(before);

    // const auto exported = acmacs::chart::export_factory(chart_modify, acmacs::chart::export_format::ace, args.program(), report);
    // auto imported = acmacs::chart::import_from_data(exported, acmacs::chart::Verify::None, report);

    // acmacs::file::write("/r/a.ace", exported, acmacs::file::ForceCompression::Yes);
    // throw std::runtime_error("not implemented");

} // test_insert_serum

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
                throw std::runtime_error("titer mismatch: sr_no:" + std::to_string(sr_no) + " orig: " + std::to_string(source_ag_no) + ' ' + titers_source->titer(source_ag_no, sr_no) +
                                         ", imported: " + std::to_string(imported_ag_no) + ' ' + titers_imported->titer(imported_ag_no, sr_no));
        }
    }
    for (auto[p_no, projection] : acmacs::enumerate(*projections_source)) {
        if (auto src = projection->layout()->get(source_ag_no), imp = (*projections_imported)[p_no]->layout()->get(imported_ag_no); src != imp)
            throw std::runtime_error("antigen coordinates mismatch: orig:" + std::to_string(source_ag_no) + ' ' + acmacs::to_string(src) + " vs. imported:" + std::to_string(imported_ag_no) + ' ' +
                                     acmacs::to_string(imp));
    }
    if (auto src = plot_spec_source->style(source_ag_no), imp = plot_spec_imported->style(imported_ag_no); src != imp)
        throw std::runtime_error("antigen plot style mismatch:\n     orig:" + std::to_string(source_ag_no) + ' ' + acmacs::to_string(src) + "\n imported:" + std::to_string(imported_ag_no) + ' ' +
                                 acmacs::to_string(imp) + "\n  report: " + acmacs::equality_report(src, imp));

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
                throw std::runtime_error("titer mismatch: ag_no:" + std::to_string(ag_no) + " orig: " + std::to_string(source_sr_no) + ' ' + titers_source->titer(ag_no, source_sr_no) +
                                         ", imported: " + std::to_string(imported_sr_no) + ' ' + titers_imported->titer(ag_no, imported_sr_no));
        }
    }
    const auto point_no_source = source_sr_no + source_number_of_antigens, point_no_imported = imported_sr_no + imported_number_of_antigens;
    for (auto[p_no, projection] : acmacs::enumerate(*projections_source)) {
        if (auto src = projection->layout()->get(point_no_source), imp = (*projections_imported)[p_no]->layout()->get(point_no_imported); src != imp)
            throw std::runtime_error("serum coordinates mismatch: orig:" + std::to_string(source_sr_no) + ' ' + acmacs::to_string(src) + " vs. imported:" + std::to_string(imported_sr_no) + ' ' +
                                     acmacs::to_string(imp));
    }
    if (auto src = plot_spec_source->style(point_no_source), imp = plot_spec_imported->style(point_no_imported); src != imp)
        throw std::runtime_error("serum plot style mismatch:\n     orig:" + std::to_string(source_sr_no) + ' ' + acmacs::to_string(src) + "\n imported:" + std::to_string(imported_sr_no) +
                                 ' ' + acmacs::to_string(imp) + "\n  report: " + acmacs::equality_report(src, imp));

} // compare_sera

// ----------------------------------------------------------------------

void test_insert_remove_antigen(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_antigen(before);
    chart_modify.remove_antigens(acmacs::ReverseSortedIndexes({before}));

    std::cerr << "WARNING: test_insert_remove_antigen not implemented\n";

} // test_insert_remove_antigen

// ----------------------------------------------------------------------

void test_insert_remove_serum(acmacs::chart::ChartP chart, size_t before, const argc_argv& args, report_time report)
{
    acmacs::chart::ChartModify chart_modify{chart};
    chart_modify.insert_serum(before);
    chart_modify.remove_sera(acmacs::ReverseSortedIndexes({before}));

    std::cerr << "WARNING: test_insert_remove_serum not implemented\n";

} // test_insert_remove_serum

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
