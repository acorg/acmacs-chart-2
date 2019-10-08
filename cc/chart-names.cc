#include "acmacs-base/argv.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static std::string format(const acmacs::chart::Chart& chart, const acmacs::chart::Antigen& antigen, size_t antigen_no, int num_digits, std::string_view pattern);
static std::string format(const acmacs::chart::Chart& chart, const acmacs::chart::Serum& serum, size_t serum_no, int num_digits, std::string_view pattern);

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> fields{*this, "fields", desc{"report names with fields"}};
    option<str>  format{*this, 'f', "format", dflt{"{ag_sr} {no0} {full_name_with_passage} {serum_species} [{date}] {lab_ids} {ref}"},
                        desc{"output format, supported fields: {ag_sr} {no0} {<no0} {no1} {<no1} {name} {full_name_with_passage} {full_name_with_fields} {abbreviated_name} {abbreviated_name_with_passage_type} {abbreviated_location_with_passage_type} {abbreviated_name_with_serum_id} {designation} {name_abbreviated} {name_without_subtype} {location_abbreviated} {abbreviated_location_year} {serum_species} {date} {lab_ids} {ref} {serum_id} {reassortant} {passage} {passage_type} {annotations} {lineage} {continent} {sera_with_titrations} "}};
    option<str>  antigens{*this, 'a', "antigens", desc{"comma separated list of antigens (zero based indexes) to report for them only"}};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};
    option<bool> verbose{*this, 'v', "verbose"};
    argument<str_array> charts{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        std::string pattern{opt.format};
        if (opt.fields)
            pattern = "{ag_sr} {no0} {full_name_with_fields}";
        for (const auto& chart_filename : *opt.charts) {
            auto chart = acmacs::chart::import_from_file(chart_filename, acmacs::chart::Verify::None, do_report_time(opt.report_time));
            auto antigens = chart->antigens();
            auto sera = chart->sera();
            const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;
            if (opt.antigens) {
                for (auto ag_no : acmacs::string::split_into_size_t(*opt.antigens, ","))
                    fmt::print("{}\n", string::strip(format(*chart, *antigens->at(ag_no), ag_no, num_digits, pattern)));
            }
            else {
                for (auto [ag_no, antigen] : acmacs::enumerate(*antigens))
                    fmt::print("{}\n", string::strip(format(*chart, *antigen, ag_no, num_digits, pattern)));
                for (auto [sr_no, serum] : acmacs::enumerate(*sera))
                    fmt::print("{}\n", string::strip(format(*chart, *serum, sr_no, num_digits, pattern)));
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

std::string format(const acmacs::chart::Chart& chart, const acmacs::chart::Antigen& antigen, size_t antigen_no, int num_digits, std::string_view pattern)
{
    return fmt::format(
        pattern,
        fmt::arg("ag_sr", "AG"),
        fmt::arg("no0", fmt::format("{:{}d}", antigen_no, num_digits)),
        fmt::arg("no0_left", fmt::format("{}", antigen_no)),
        fmt::arg("no1", fmt::format("{:{}d}", antigen_no + 1, num_digits)),
        fmt::arg("no1_left", fmt::format("{}", antigen_no + 1)),
        fmt::arg("name", *antigen.name()),
        fmt::arg("full_name", antigen.full_name()),
        fmt::arg("full_name_with_passage", antigen.full_name_with_passage()),
        fmt::arg("full_name_with_fields", antigen.full_name_with_fields()),
        fmt::arg("serum_species", ""),
        fmt::arg("date", *antigen.date()),
        fmt::arg("lab_ids", string::join(" ", antigen.lab_ids())),
        fmt::arg("ref", antigen.reference() ? "Ref" : ""),
        fmt::arg("serum_id", ""),
        fmt::arg("reassortant", *antigen.reassortant()),
        fmt::arg("passage", *antigen.passage()),
        fmt::arg("passage_type", antigen.passage_type()),
        fmt::arg("annotations", antigen.annotations()),
        fmt::arg("lineage", antigen.lineage()),
        fmt::arg("continent", *antigen.continent()),
        fmt::arg("abbreviated_name", antigen.abbreviated_name()),
        fmt::arg("abbreviated_name_with_passage_type", antigen.abbreviated_name_with_passage_type()),
        fmt::arg("abbreviated_name_with_serum_id", antigen.abbreviated_name()),
        fmt::arg("abbreviated_location_with_passage_type", antigen.abbreviated_location_with_passage_type()),
        fmt::arg("designation", antigen.designation()),
        fmt::arg("name_abbreviated", antigen.name_abbreviated()),
        fmt::arg("name_without_subtype", antigen.name_without_subtype()),
        fmt::arg("location_abbreviated", antigen.location_abbreviated()),
        fmt::arg("abbreviated_location_year", antigen.abbreviated_location_year()),
        fmt::arg("sera_with_titrations", chart.titers()->having_titers_with(antigen_no))
    );

} // format

// ----------------------------------------------------------------------

std::string format(const acmacs::chart::Chart& chart, const acmacs::chart::Serum& serum, size_t serum_no, int num_digits, std::string_view pattern)
{
    return fmt::format(
        pattern,
        fmt::arg("ag_sr", "SR"),
        fmt::arg("no0", fmt::format("{:{}d}", serum_no, num_digits)),
        fmt::arg("no0_left", fmt::format("{}", serum_no)),
        fmt::arg("no1", fmt::format("{:{}d}", serum_no + 1, num_digits)),
        fmt::arg("no1_left", fmt::format("{}", serum_no + 1)),
        fmt::arg("name", *serum.name()),
        fmt::arg("full_name", serum.full_name()),
        fmt::arg("full_name_with_passage", serum.full_name_with_passage()),
        fmt::arg("full_name_with_fields", serum.full_name_with_fields()),
        fmt::arg("serum_species", *serum.serum_species()),
        fmt::arg("date", ""),
        fmt::arg("lab_ids", ""),
        fmt::arg("ref", ""),
        fmt::arg("serum_id", *serum.serum_id()),
        fmt::arg("reassortant", *serum.reassortant()),
        fmt::arg("passage", *serum.passage()),
        fmt::arg("passage_type", serum.passage_type()),
        fmt::arg("annotations", serum.annotations()),
        fmt::arg("lineage", serum.lineage()),
        fmt::arg("continent", ""),
        fmt::arg("abbreviated_name", serum.abbreviated_name()),
        fmt::arg("abbreviated_name_with_passage_type", serum.abbreviated_name()),
        fmt::arg("abbreviated_name_with_serum_id", serum.abbreviated_name_with_serum_id()),
        fmt::arg("abbreviated_location_with_passage_type", serum.abbreviated_name()),
        fmt::arg("designation", serum.designation()),
        fmt::arg("name_abbreviated", serum.name_abbreviated()),
        fmt::arg("name_without_subtype", serum.name_without_subtype()),
        fmt::arg("location_abbreviated", serum.location_abbreviated()),
        fmt::arg("abbreviated_location_year", serum.abbreviated_location_year()),
        fmt::arg("sera_with_titrations", chart.titers()->having_titers_with(serum_no + chart.number_of_antigens()))
    );

} // format

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
