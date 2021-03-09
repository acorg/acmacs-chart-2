#include <functional>

#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-substitute.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/regex.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

#define COLLAPSABLE_SPACE "\x01"

template <typename AgSr> using format_subst_t = std::pair<std::string_view, std::function<void(fmt::memory_buffer&, std::string_view format, const AgSr&)>>;
template <typename AgSr> using format_subst_list_t = std::vector<format_subst_t<AgSr>>;

#define KEY_FUNC_AGSR(key, call) {key, [](fmt::memory_buffer& output, std::string_view format, const AgSr& ag_sr) { fmt::format_to(output, format, fmt::arg(key, call)); }}
#define KEY_FUNC_AG(key, call) {key, [](fmt::memory_buffer& output, std::string_view format, [[maybe_unused]] const acmacs::chart::Antigen& ag) { fmt::format_to(output, format, fmt::arg(key, call)); }}
#define KEY_FUNC_SR(key, call) {key, [](fmt::memory_buffer& output, std::string_view format, [[maybe_unused]] const acmacs::chart::Serum& sr) { fmt::format_to(output, format, fmt::arg(key, call)); }}

// ----------------------------------------------------------------------

template <typename AgSr> static std::string name_full_without_passage(const AgSr& ag_sr)
{
    return acmacs::string::join(acmacs::string::join_space, ag_sr.name(), fmt::format("{: }", ag_sr.annotations()), ag_sr.reassortant());
}

template <typename AgSr> static std::string name_full(const AgSr& ag_sr)
{
    return acmacs::string::join(acmacs::string::join_space, name_full_without_passage(ag_sr), ag_sr.passage());
}

template <typename AgSr> static std::string location_abbreviated(const AgSr& ag_sr)
{
    return acmacs::locationdb::get().abbreviation(acmacs::virus::location(ag_sr.name()));
}

template <typename AgSr> static std::string year4(const AgSr& ag_sr)
{
    if (const auto yr = acmacs::virus::year(ag_sr.name()); yr.has_value())
        return fmt::format("{}", *yr);
    else
        return {};
}

template <typename AgSr> static std::string year2(const AgSr& ag_sr)
{
    if (const auto yr = year4(ag_sr); yr.size() == 4)
        return yr.substr(2);
    else
        return yr;
}

template <typename AgSr> static std::string name_abbreviated(const AgSr& ag_sr)
{
    return acmacs::string::join(acmacs::string::join_slash, location_abbreviated(ag_sr), acmacs::virus::isolation(ag_sr.name()), year2(ag_sr));
}

template <typename AgSr> static std::string fields(const AgSr& ag_sr)
{
    fmt::memory_buffer output;
    fmt::format_to(output, "{}", ag_sr.name());
    if (const auto value = fmt::format("{: }", ag_sr.annotations()); !value.empty())
        fmt::format_to(output, " annotations=\"{}\"", value);
    if (const auto value = ag_sr.reassortant(); !value.empty())
        fmt::format_to(output, " reassortant=\"{}\"", *value);
    if constexpr (std::is_same_v<AgSr, acmacs::chart::Serum>) {
        if (const auto value = ag_sr.serum_id(); !value.empty())
            fmt::format_to(output, " serum_id=\"{}\"", *value);
    }
    if (const auto value = ag_sr.passage(); !value.empty())
        fmt::format_to(output, " passage=\"{}\" ptype={}", *value, value.passage_type());
    if constexpr (std::is_same_v<AgSr, acmacs::chart::Antigen>) {
        if (const auto value = ag_sr.date(); !value.empty())
            fmt::format_to(output, " date={}", *value);
    }
    else {
        if (const auto value = ag_sr.serum_species(); !value.empty())
            fmt::format_to(output, " serum_species=\"{}\"", *value);
    }
    if (const auto value = ag_sr.lineage(); value != acmacs::chart::BLineage::Unknown)
        fmt::format_to(output, " lineage={}", value);
    if constexpr (std::is_same_v<AgSr, acmacs::chart::Antigen>) {
        if (ag_sr.reference())
            fmt::format_to(output, " reference");
        if (const auto value = ag_sr.lab_ids().join(); !value.empty())
            fmt::format_to(output, " lab_ids=\"{}\"", value);
    }
    return fmt::to_string(output);
}

// ----------------------------------------------------------------------

std::string acmacs::chart::detail::AntigenSerum::format(std::string_view pattern) const
{
    fmt::memory_buffer output;
    if (format(output, pattern))
        return acmacs::chart::collapse_spaces(fmt::to_string(output));
    else
        return fmt::to_string(output);

} // acmacs::chart::detail::AntigenSerum::format

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

// Note: longest keys must be first!

template <typename AgSr>
const format_subst_list_t<AgSr> format_subst_ag_sr{
    KEY_FUNC_AGSR("abbreviated_location_with_passage_type", acmacs::string::join(acmacs::string::join_space, location_abbreviated(ag_sr), ag_sr.passage().passage_type())), // mapi
    KEY_FUNC_AGSR("abbreviated_name_with_passage_type", fmt::format("{}-{}", name_abbreviated(ag_sr), ag_sr.passage().passage_type())),                                     // mapi
    KEY_FUNC_AGSR("location_abbreviated", location_abbreviated(ag_sr)),                                                                                                     //
    KEY_FUNC_AGSR("name_without_subtype", acmacs::virus::without_subtype(ag_sr.name())),                                                                                    //
    KEY_FUNC_AGSR("name_abbreviated", name_abbreviated(ag_sr)),                                                                                                             //
    KEY_FUNC_AGSR("passage_type", ag_sr.passage().passage_type()),                                                                                                          //
    KEY_FUNC_AGSR("reassortant", ag_sr.reassortant()),                                                                                                                      //
    KEY_FUNC_AGSR("annotations", ag_sr.annotations()),                                                                                                                      //
    KEY_FUNC_AGSR("name_full", name_full(ag_sr)),                                                                                                                           //
    KEY_FUNC_AGSR("full_name", name_full(ag_sr)),                                                                                                                           //
    KEY_FUNC_AGSR("location", acmacs::virus::location(ag_sr.name())),                                                                                                       //
    KEY_FUNC_AGSR("continent", ag_sr.location_data().continent),                                                                                                            //
    KEY_FUNC_AGSR("longitude", ag_sr.location_data().longitude),                                                                                                            //
    KEY_FUNC_AGSR("latitude", ag_sr.location_data().latitude),                                                                                                              //
    KEY_FUNC_AGSR("country", ag_sr.location_data().country),                                                                                                                //
    KEY_FUNC_AGSR("lineage", ag_sr.lineage().to_string()),                                                                                                                  //
    KEY_FUNC_AGSR("passage", ag_sr.passage()),                                                                                                                              //
    KEY_FUNC_AGSR("fields", fields(ag_sr)),                                                                                                                                 //
    KEY_FUNC_AGSR("year4", year4(ag_sr)),                                                                                                                                   //
    KEY_FUNC_AGSR("year2", year2(ag_sr)),                                                                                                                                   //
    KEY_FUNC_AGSR("year", year4(ag_sr)),                                                                                                                                    //
    KEY_FUNC_AGSR("name", ag_sr.name()),                                                                                                                                    //
};

const format_subst_list_t<acmacs::chart::Antigen> format_subst_antigen{
    KEY_FUNC_AG("designation", name_full(ag)),   //
    KEY_FUNC_AG("lab_ids", ag.lab_ids().join()), //
    KEY_FUNC_AG("ag_sr", "AG"),                      //
    KEY_FUNC_AG("date", ag.date()),                  //
    KEY_FUNC_AG("ref", ag.reference() ? "Ref" : ""), //
};

const format_subst_list_t<acmacs::chart::Serum> format_subst_serum {
    KEY_FUNC_SR("designation_without_serum_id",  name_full_without_passage(sr)), //
    KEY_FUNC_SR("designation",  acmacs::string::join(acmacs::string::join_space, name_full_without_passage(sr), sr.serum_id())), //
    KEY_FUNC_SR("serum_species", sr.serum_species()),  //
    KEY_FUNC_SR("serum_id", sr.serum_id()),  //
    KEY_FUNC_SR("species", sr.serum_species()),  //
    KEY_FUNC_SR("ag_sr", "SR"),  //
        // KEY_FUNC_SR("date", date),         // by homologous antigen
};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

template <typename AgSr, typename List> static inline bool apply(fmt::memory_buffer& output, std::string_view chunk, const List& list, const AgSr& ag_sr)
{
    for (const auto& [name, func] : list) {
        if (acmacs::string::startswith(chunk.substr(1), name)) {
            func(output, chunk, ag_sr);
            return true;
        }
    }
    return false;
}

// ----------------------------------------------------------------------

bool acmacs::chart::Antigen::format(fmt::memory_buffer& output, std::string_view pattern) const
{
    bool collapsable_spaces{false};
    for (const auto& chunk : acmacs::string::split_for_formatting(pattern)) {
        bool applied{false};
        if (chunk.size() > 3 && chunk[0] == '{')
            applied = apply(output, chunk, format_subst_ag_sr<decltype(*this)>, *this) || apply(output, chunk, format_subst_antigen, *this);
        else if (chunk == "{ }") { // collapsable space
            fmt::format_to(output, COLLAPSABLE_SPACE);
            collapsable_spaces = true;
            applied = true;
        }
        if (!applied)
            fmt::format_to(output, "{}", chunk);
    }
    return collapsable_spaces;

} // acmacs::chart::Antigen::format

// ----------------------------------------------------------------------

bool acmacs::chart::Serum::format(fmt::memory_buffer& output, std::string_view pattern) const
{
    bool collapsable_spaces{false};
    for (const auto& chunk : acmacs::string::split_for_formatting(pattern)) {
        bool applied{false};
        if (chunk.size() > 3 && chunk[0] == '{')
            applied = apply(output, chunk, format_subst_ag_sr<decltype(*this)>, *this) || apply(output, chunk, format_subst_serum, *this);
        else if (chunk == "{ }") { // collapsable space
            fmt::format_to(output, COLLAPSABLE_SPACE);
            collapsable_spaces = true;
            applied = true;
        }
        if (!applied)
            fmt::format_to(output, "{}", chunk);
    }
    return collapsable_spaces;

} // acmacs::chart::Serum::format

// ----------------------------------------------------------------------

const acmacs::chart::detail::location_data_t& acmacs::chart::detail::AntigenSerum::location_data() const
{
    if (!location_data_filled_) {
        const auto location{acmacs::virus::location(name())};
        if (const auto loc = acmacs::locationdb::get().find(location, acmacs::locationdb::include_continent::yes); loc.has_value())
            location_data_ = location_data_t{std::move(loc->name), std::string{loc->country()}, loc->continent, loc->latitude(), loc->longitude()};
        else
            location_data_ = location_data_t{std::string{location}, "*unknown*", "*unknown*", 360.0, 360.0};
        location_data_filled_ = true;
    }
    return location_data_;

} // acmacs::chart::detail::AntigenSerum::location_data

// ----------------------------------------------------------------------

std::string acmacs::chart::collapse_spaces(std::string src)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::array replace_data{
        acmacs::regex::look_replace_t{std::regex("^(\\s*)" COLLAPSABLE_SPACE, std::regex::icase), {"$1$'"}},
        acmacs::regex::look_replace_t{std::regex(COLLAPSABLE_SPACE "(\\s*)$", std::regex::icase), {"$`$1"}},
        acmacs::regex::look_replace_t{std::regex("[\\s" COLLAPSABLE_SPACE "]*" COLLAPSABLE_SPACE "[\\s" COLLAPSABLE_SPACE "]*", std::regex::icase), {"$` $'"}},
    };
#include "acmacs-base/diagnostics-pop.hh"

    while (true) {
        if (const auto replacement = scan_replace(src, replace_data); replacement.has_value())
            src = replacement->back();
        else
            break;
    }
    return src;

} // acmacs::chart::collapse_spaces

// ----------------------------------------------------------------------

// inline std::tuple<std::string, std::string, std::string, acmacs::locationdb::Latitude, acmacs::locationdb::Longitude> location_data(std::string_view location)
// {
//     using namespace std::string_literals;
//     if (const auto loc = acmacs::locationdb::get().find(location, acmacs::locationdb::include_continent::yes); loc.has_value())
//         return {std::move(loc->name), std::string{loc->country()}, loc->continent, loc->latitude(), loc->longitude()};
//     else
//         return {std::string{location}, "*unknown*"s, "*unknown*"s, 360.0, 360.0};
// }

// ----------------------------------------------------------------------

std::string acmacs::chart::format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no)
{
    auto antigens = chart.antigens();
    auto antigen = antigens->at(antigen_no);
    auto sera = chart.sera();
    const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;

    // const auto [location_name, country, continent, latitude, longitude] = location_data(antigen->location());

    AD_DEBUG("format_antigen1 \"{}\"", pattern);
    const std::string intermediate = antigen->format(pattern);
    AD_DEBUG("format_antigen2 \"{}\"", intermediate);
    return fmt::format(
        intermediate,
        fmt::arg("no0", fmt::format("{:{}d}", antigen_no, num_digits)),
        fmt::arg("no0_left", fmt::format("{}", antigen_no)),
        fmt::arg("no1", fmt::format("{:{}d}", antigen_no + 1, num_digits)),
        fmt::arg("no1_left", fmt::format("{}", antigen_no + 1)),
    //     fmt::arg("name", *antigen->name()),
    //     fmt::arg("full_name", antigen->full_name()),
    //     fmt::arg("full_name_with_passage", antigen->full_name_with_passage()),
    //     fmt::arg("full_name_with_fields", antigen->full_name_with_fields()),
    //     fmt::arg("serum_species", ""),
    //     fmt::arg("date", *antigen->date()),
    //     fmt::arg("lab_ids", acmacs::string::join(acmacs::string::join_space, antigen->lab_ids())),
    //     fmt::arg("ref", antigen->reference() ? "Ref" : ""),
    //     fmt::arg("serum_id", ""),
    //     fmt::arg("reassortant", *antigen->reassortant()),
    //     fmt::arg("passage", *antigen->passage()),
    //     fmt::arg("passage_type", antigen->passage_type()),
    //     fmt::arg("annotations", acmacs::string::join(acmacs::string::join_space, antigen->annotations())),
    //     fmt::arg("lineage", antigen->lineage()),
    //     fmt::arg("abbreviated_name", antigen->abbreviated_name()),
    //     fmt::arg("abbreviated_name_with_passage_type", antigen->abbreviated_name_with_passage_type()),
    //     fmt::arg("abbreviated_name_with_serum_id", antigen->abbreviated_name()),
    //     fmt::arg("abbreviated_location_with_passage_type", antigen->abbreviated_location_with_passage_type()),
    //     fmt::arg("abbreviated_location_year", antigen->abbreviated_location_year()),
    //     fmt::arg("designation", antigen->designation()),
    //     fmt::arg("name_abbreviated", antigen->name_abbreviated()),
    //     fmt::arg("name_without_subtype", antigen->name_without_subtype()),
    //     fmt::arg("location", location_name),
    //     fmt::arg("location_abbreviated", antigen->location_abbreviated()),
    //     fmt::arg("country", country),
    //     fmt::arg("continent", continent),
    //     fmt::arg("latitude", latitude),
    //     fmt::arg("longitude", longitude),
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

    // const auto [location_name, country, continent, latitude, longitude] = location_data(serum->location());

    // std::string date;
    // for (auto ag_no : serum->homologous_antigens()) {
    //     if (const auto ag_date  = antigens->at(ag_no)->date(); !ag_date.empty()) {
    //         date = ag_date;
    //         break;
    //     }
    // }

    const std::string intermediate = serum->format(pattern);
    AD_DEBUG("format_serum \"{}\"", intermediate);
    return fmt::format(
        intermediate,
        fmt::arg("no0", fmt::format("{:{}d}", serum_no, num_digits)),
        fmt::arg("no0_left", fmt::format("{}", serum_no)),
        fmt::arg("no1", fmt::format("{:{}d}", serum_no + 1, num_digits)),
        fmt::arg("no1_left", fmt::format("{}", serum_no + 1)),
    //     fmt::arg("name", *serum->name()),
    //     fmt::arg("full_name", serum->full_name()),
    //     fmt::arg("full_name_with_passage", serum->full_name_with_passage()),
    //     fmt::arg("full_name_with_fields", serum->full_name_with_fields()),
    //     fmt::arg("serum_species", *serum->serum_species()),
    //     fmt::arg("date", date),
    //     fmt::arg("lab_ids", ""),
    //     fmt::arg("ref", ""),
    //     fmt::arg("serum_id", *serum->serum_id()),
    //     fmt::arg("reassortant", *serum->reassortant()),
    //     fmt::arg("passage", *serum->passage()),
    //     fmt::arg("passage_type", serum->passage_type()),
    //     fmt::arg("annotations", acmacs::string::join(acmacs::string::join_space, serum->annotations())),
    //     fmt::arg("lineage", serum->lineage()),
    //     fmt::arg("abbreviated_name", serum->abbreviated_name()),
    //     fmt::arg("abbreviated_name_with_passage_type", serum->abbreviated_name()),
    //     fmt::arg("abbreviated_name_with_serum_id", serum->abbreviated_name_with_serum_id()),
    //     fmt::arg("abbreviated_location_with_passage_type", serum->abbreviated_name()),
    //     fmt::arg("designation", serum->designation()),
    //     fmt::arg("name_abbreviated", serum->name_abbreviated()),
    //     fmt::arg("name_without_subtype", serum->name_without_subtype()),
    //     fmt::arg("location", location_name),
    //     fmt::arg("location_abbreviated", serum->location_abbreviated()),
    //     fmt::arg("country", country),
    //     fmt::arg("continent", continent),
    //     fmt::arg("latitude", latitude),
    //     fmt::arg("longitude", longitude),
    //     fmt::arg("abbreviated_location_year", serum->abbreviated_location_year()),
        fmt::arg("sera_with_titrations", chart.titers()->having_titers_with(serum_no + chart.number_of_antigens()))
    );

} // acmacs::chart::format_serum

// ----------------------------------------------------------------------

constexpr const std::string_view pattern = R"(
{{ag_sr}}                                  : {ag_sr}
{{no0}}                                    : {no0}
{{no0_left}}                               : {no0_left}
{{no1}}                                    : {no1}
{{no1_left}}                               : {no1_left}
{{name}}                                   : {name}
{{full_name}}                              : {full_name}
{{full_name_with_passage}}                 : {full_name_with_passage}
{{full_name_with_fields}}                  : {full_name_with_fields}
{{serum_species}}                          : {serum_species}
{{date}}                                   : {date}
{{lab_ids}}                                : {lab_ids}
{{ref}}                                    : {ref}
{{serum_id}}                               : {serum_id}
{{reassortant}}                            : {reassortant}
{{passage}}                                : {passage}
{{passage_type}}                           : {passage_type}
{{annotations}}                            : {annotations}
{{lineage}}                                : {lineage}
{{abbreviated_name}}                       : {abbreviated_name}
{{abbreviated_name_with_passage_type}}     : {abbreviated_name_with_passage_type}
{{abbreviated_name_with_serum_id}}         : {abbreviated_name_with_serum_id}
{{abbreviated_location_with_passage_type}} : {abbreviated_location_with_passage_type}
{{abbreviated_location_year}}              : {abbreviated_location_year}
{{designation}}                            : {designation}
{{name_abbreviated}}                       : {name_abbreviated}
{{name_without_subtype}}                   : {name_without_subtype}
{{location}}                               : {location}
{{location_abbreviated}}                   : {location_abbreviated}
{{country}}                                : {country}
{{continent}}                              : {continent}
{{latitude}}                               : {latitude}
{{longitude}}                              : {longitude}
{{sera_with_titrations}}                   : {sera_with_titrations}
)";

std::string acmacs::chart::format_help()
{
    ChartNew chart{101, 101};

    auto& antigen = chart.antigens_modify().at(67);
    antigen.name("A(H3N2)/KLAGENFURT/24/2020");
    antigen.date("2020-05-24");
    antigen.passage(acmacs::virus::Passage{"E1 (2020-04-01)"});
    antigen.reassortant(acmacs::virus::Reassortant{"NYMC-1A"});
    antigen.add_annotation("FLEDERMAUS");
    antigen.add_annotation("BAT");
    antigen.reference(true);
    antigen.add_clade("3C.3A");

    auto& serum = chart.sera_modify().at(12);
    serum.name("B/WUHAN/24/2020");
    serum.passage(acmacs::virus::Passage{"MDCK1/SIAT2 (2020-04-01)"});
    serum.lineage("YAMAGATA");
    serum.serum_id(SerumId{"2020-031"});
    serum.serum_species(SerumSpecies{"RAT"});

    return fmt::format("{}\n\n{}\n", format_antigen(pattern, chart, 67), format_serum(pattern, chart, 12));

} // acmacs::chart::format_help

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
