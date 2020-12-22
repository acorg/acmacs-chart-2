#include "acmacs-base/argv.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-virus/virus-name-normalize.hh"
#include "acmacs-virus/passage.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/log.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str> input_chart{*this, arg_name{"input-chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}};
};

static void check_unknown(std::string_view name);
static void update_antigens(acmacs::chart::AntigensModify& antigens, const acmacs::virus::type_subtype_t& subtype, const acmacs::Lab& lab);
static void update_sera(acmacs::chart::SeraModify& sera, const acmacs::virus::type_subtype_t& subtype);

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);

        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.input_chart)};
        const auto subtype = chart.info()->virus_type();

        update_antigens(chart.antigens_modify(), subtype, chart.info()->lab());
        update_sera(chart.sera_modify(), subtype);

        chart.detect_reference_antigens(acmacs::chart::remove_reference_before_detecting::yes);
        acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name());
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void check_unknown(std::string_view name)
{
    using namespace std::string_view_literals;

    if (name.find("UNKNOWN"sv) != std::string_view::npos)
        AD_WARNING("name contains UNKNOWN: {}", name);

} // check_unknown

// ----------------------------------------------------------------------

void update_antigens(acmacs::chart::AntigensModify& antigens, const acmacs::virus::type_subtype_t& subtype, const acmacs::Lab& lab)
{
    std::vector<std::pair<size_t, std::string>> full_names; // to find duplicates
    for (size_t ag_no{0}; ag_no < antigens.size(); ++ag_no) {
        auto& antigen = antigens.at(ag_no);

        AD_LOG(acmacs::log::name_parsing, "AG {} \"{}\"", ag_no, antigen.name());
        AD_LOG_INDENT;
        auto parsed_name = acmacs::virus::name::parse(antigen.name(), acmacs::virus::name::warn_on_empty::yes, acmacs::virus::name::extract_passage::no);
        if (parsed_name.not_good())
            AD_WARNING("AG {} \"{}\": {}", ag_no, antigen.name(), parsed_name.messages);
        if (*parsed_name.subtype == "A")
            parsed_name.subtype = subtype;
        const auto new_name = parsed_name.name();
        if (antigen.name() != new_name) {
            AD_LOG(acmacs::log::name_parsing, "\"{}\" -> {}", antigen.name(), parsed_name);
            if (!parsed_name.mutations.empty())
                AD_WARNING("Name has mutations: {} <-- \"{}\"", parsed_name.mutations, antigen.name());
            antigen.name(*new_name);
            if (!parsed_name.reassortant.empty())
                antigen.reassortant(parsed_name.reassortant);
            if (!parsed_name.extra.empty())
                antigen.add_annotation(parsed_name.extra);
        }
        check_unknown(antigen.name());

        auto [passage, extra] = acmacs::virus::parse_passage(antigen.passage(), acmacs::virus::passage_only::no);
        if (!extra.empty())
            AD_WARNING("extra in passage \"{}\" \"{}\" <- \"{}\"", passage, extra, antigen.passage());
        if (passage != antigen.passage()) {
            antigen.passage(passage);
            if (!extra.empty())
                antigen.add_annotation(extra);
        }

        if (auto date = antigen.date(); !date.empty())
            antigen.date(date::display(date::from_string(*date, date::allow_incomplete::no, date::throw_on_error::yes, *lab == "CDC" ? date::month_first::yes : date::month_first::no)));

        full_names.emplace_back(ag_no, antigen.full_name());
    }

    // mark duplicates as distinct
    std::sort(std::begin(full_names), std::end(full_names), [](const auto& e1, const auto& e2) { return e1.second == e2.second ? e1.first < e2.first : e1.second < e2.second; });
    for (const auto index : range_from_to(1ul, full_names.size())) {
        if (full_names[index - 1].second == full_names[index].second) {
            antigens.at(full_names[index].first).set_distinct();
            AD_LOG(acmacs::log::distinct, "AG {} \"{}\" vs. AG {}", full_names[index].first, antigens.at(full_names[index].first).full_name(), full_names[index - 1].first);
        }
    }

} // update_antigens

// ----------------------------------------------------------------------

void update_sera(acmacs::chart::SeraModify& sera, const acmacs::virus::type_subtype_t& subtype)
{
    using namespace std::string_view_literals;

    for (size_t sr_no{0}; sr_no < sera.size(); ++sr_no) {
        auto& serum = sera.at(sr_no);

        AD_LOG(acmacs::log::name_parsing, "SR {} \"{}\"", sr_no, serum.name());
        AD_LOG_INDENT;
        auto parsed_name = acmacs::virus::name::parse(serum.name(), acmacs::virus::name::warn_on_empty::yes, acmacs::virus::name::extract_passage::no);
        if (parsed_name.not_good())
            AD_WARNING("SR {} \"{}\": {}", sr_no, serum.name(), parsed_name.messages);
        if (*parsed_name.subtype == "A")
            parsed_name.subtype = subtype;
        const auto new_name = parsed_name.name();
        if (serum.name() != new_name) {
            AD_LOG(acmacs::log::name_parsing, "\"{}\" -> {}", serum.name(), parsed_name);
            serum.name(*new_name);
            if (!parsed_name.reassortant.empty() && parsed_name.reassortant != serum.reassortant()) {
                if (serum.reassortant().empty())
                    serum.reassortant(parsed_name.reassortant);
                else
                    serum.reassortant(acmacs::virus::Reassortant{fmt::format("{} {}", serum.reassortant(), parsed_name.reassortant)});
            }
            if (!parsed_name.extra.empty())
                serum.add_annotation(parsed_name.extra);
        }
        check_unknown(serum.name());

        auto [passage, extra] = acmacs::virus::parse_passage(serum.passage(), acmacs::virus::passage_only::no);
        if (!extra.empty() && !(passage == "E?"sv && acmacs::string::startswith(extra, "10-"sv)))
            AD_WARNING("passage \"{}\" has extra \"{}\" <-- \"{}\"", passage, extra, serum.passage());
        if (passage != serum.passage()) {
            serum.passage(passage);
            if (!extra.empty())
                serum.add_annotation(extra);
        }
    }

} // update_sera

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
