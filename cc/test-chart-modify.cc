#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <array>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/factory-export.hh"

static void test_chart_modify_no_changes(acmacs::chart::ChartP chart, const argc_argv& args, report_time report);
static void test_modify_titers(acmacs::chart::ChartP chart, const argc_argv& args, report_time report);
static void test_dont_care_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, const argc_argv& args, report_time report);
static void test_dont_care_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, const argc_argv& args, report_time report);
static void test_multiply_by_for_antigen(acmacs::chart::ChartP chart, size_t aAntigenNo, double aMult, const argc_argv& args, report_time report);
static void test_multiply_by_for_serum(acmacs::chart::ChartP chart, size_t aSerumNo, double aMult, const argc_argv& args, report_time report);

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
            test_chart_modify_no_changes(chart, args, report);
            if (chart->titers()->number_of_layers() == 0) {
                std::cout << "  test_modify_titers\n";
                test_modify_titers(chart, args, report);
                std::cout << "  test_dont_care_for_antigen\n";
                for (auto ag_no : acmacs::range(chart->number_of_antigens()))
                    test_dont_care_for_antigen(chart, ag_no, args, report);
                std::cout << "  test_dont_care_for_serum\n";
                for (auto sr_no : acmacs::range(chart->number_of_sera()))
                    test_dont_care_for_serum(chart, sr_no, args, report);
                std::cout << "  test_multiply_by_for_antigen\n";
                for (auto ag_no : acmacs::range(chart->number_of_antigens())) {
                    test_multiply_by_for_antigen(chart, ag_no, 2.0, args, report);
                    test_multiply_by_for_antigen(chart, ag_no, 0.5, args, report);
                }
                std::cout << "  test_multiply_by_for_serum\n";
                for (auto sr_no : acmacs::range(chart->number_of_sera())) {
                    test_multiply_by_for_serum(chart, sr_no, 2.0, args, report);
                    test_multiply_by_for_serum(chart, sr_no, 0.5, args, report);
                }
            }
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
    auto titers_source{chart->titers()}, titers_modified{imported->titers()};

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
