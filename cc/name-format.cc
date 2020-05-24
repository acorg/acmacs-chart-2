#include "acmacs-base/string-join.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

inline std::tuple<std::string, std::string, std::string, acmacs::locationdb::Latitude, acmacs::locationdb::Longitude> location_data(std::string_view location)
{
    using namespace std::string_literals;
    if (const auto loc = acmacs::locationdb::get().find(location, acmacs::locationdb::include_continent::yes); loc.has_value())
        return {std::move(loc->location_name), std::string{loc->country()}, loc->continent, loc->latitude(), loc->longitude()};
    else
        return {std::string{location}, "*unknown*"s, "*unknown*"s, 360.0, 360.0};
}

// ----------------------------------------------------------------------

std::string acmacs::chart::format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no)
{
    auto antigens = chart.antigens();
    auto antigen = antigens->at(antigen_no);
    auto sera = chart.sera();
    const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;

    const auto [location_name, country, continent, latitude, longitude] = location_data(antigen->location());

    return fmt::format(
        pattern,
        fmt::arg("ag_sr", "AG"),
        fmt::arg("no0", fmt::format("{:{}d}", antigen_no, num_digits)),
        fmt::arg("no0_left", fmt::format("{}", antigen_no)),
        fmt::arg("no1", fmt::format("{:{}d}", antigen_no + 1, num_digits)),
        fmt::arg("no1_left", fmt::format("{}", antigen_no + 1)),
        fmt::arg("name", *antigen->name()),
        fmt::arg("full_name", antigen->full_name()),
        fmt::arg("full_name_with_passage", antigen->full_name_with_passage()),
        fmt::arg("full_name_with_fields", antigen->full_name_with_fields()),
        fmt::arg("serum_species", ""),
        fmt::arg("date", *antigen->date()),
        fmt::arg("lab_ids", acmacs::string::join(acmacs::string::join_space, antigen->lab_ids())),
        fmt::arg("ref", antigen->reference() ? "Ref" : ""),
        fmt::arg("serum_id", ""),
        fmt::arg("reassortant", *antigen->reassortant()),
        fmt::arg("passage", *antigen->passage()),
        fmt::arg("passage_type", antigen->passage_type()),
        fmt::arg("annotations", antigen->annotations()),
        fmt::arg("lineage", antigen->lineage()),
        fmt::arg("abbreviated_name", antigen->abbreviated_name()),
        fmt::arg("abbreviated_name_with_passage_type", antigen->abbreviated_name_with_passage_type()),
        fmt::arg("abbreviated_name_with_serum_id", antigen->abbreviated_name()),
        fmt::arg("abbreviated_location_with_passage_type", antigen->abbreviated_location_with_passage_type()),
        fmt::arg("abbreviated_location_year", antigen->abbreviated_location_year()),
        fmt::arg("designation", antigen->designation()),
        fmt::arg("name_abbreviated", antigen->name_abbreviated()),
        fmt::arg("name_without_subtype", antigen->name_without_subtype()),
        fmt::arg("location", location_name),
        fmt::arg("location_abbreviated", antigen->location_abbreviated()),
        fmt::arg("country", country),
        fmt::arg("continent", continent),
        fmt::arg("latitude", latitude),
        fmt::arg("longitude", longitude),
        fmt::arg("sera_with_titrations", chart.titers()->having_titers_with(antigen_no))
    );

} // acmacs::chart::format_antigen

// ----------------------------------------------------------------------

std::string acmacs::chart::format_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t serum_no)
{
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto serum = sera->at(serum_no);
    const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;

    const auto [location_name, country, continent, latitude, longitude] = location_data(serum->location());

    std::string date;
    for (auto ag_no : serum->homologous_antigens()) {
        if (const auto ag_date  = antigens->at(ag_no)->date(); !ag_date.empty()) {
            date = ag_date;
            break;
        }
    }

    return fmt::format(
        pattern,
        fmt::arg("ag_sr", "SR"),
        fmt::arg("no0", fmt::format("{:{}d}", serum_no, num_digits)),
        fmt::arg("no0_left", fmt::format("{}", serum_no)),
        fmt::arg("no1", fmt::format("{:{}d}", serum_no + 1, num_digits)),
        fmt::arg("no1_left", fmt::format("{}", serum_no + 1)),
        fmt::arg("name", *serum->name()),
        fmt::arg("full_name", serum->full_name()),
        fmt::arg("full_name_with_passage", serum->full_name_with_passage()),
        fmt::arg("full_name_with_fields", serum->full_name_with_fields()),
        fmt::arg("serum_species", *serum->serum_species()),
        fmt::arg("date", date),
        fmt::arg("lab_ids", ""),
        fmt::arg("ref", ""),
        fmt::arg("serum_id", *serum->serum_id()),
        fmt::arg("reassortant", *serum->reassortant()),
        fmt::arg("passage", *serum->passage()),
        fmt::arg("passage_type", serum->passage_type()),
        fmt::arg("annotations", serum->annotations()),
        fmt::arg("lineage", serum->lineage()),
        fmt::arg("abbreviated_name", serum->abbreviated_name()),
        fmt::arg("abbreviated_name_with_passage_type", serum->abbreviated_name()),
        fmt::arg("abbreviated_name_with_serum_id", serum->abbreviated_name_with_serum_id()),
        fmt::arg("abbreviated_location_with_passage_type", serum->abbreviated_name()),
        fmt::arg("designation", serum->designation()),
        fmt::arg("name_abbreviated", serum->name_abbreviated()),
        fmt::arg("name_without_subtype", serum->name_without_subtype()),
        fmt::arg("location", location_name),
        fmt::arg("location_abbreviated", serum->location_abbreviated()),
        fmt::arg("country", country),
        fmt::arg("continent", continent),
        fmt::arg("latitude", latitude),
        fmt::arg("longitude", longitude),
        fmt::arg("abbreviated_location_year", serum->abbreviated_location_year()),
        fmt::arg("sera_with_titrations", chart.titers()->having_titers_with(serum_no + chart.number_of_antigens()))
    );

} // acmacs::chart::format_serum

// ----------------------------------------------------------------------

std::string acmacs::chart::format_help()
{
    fmt::memory_buffer output;
    return fmt::to_string(output);

} // acmacs::chart::format_help

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
