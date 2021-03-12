#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/regex.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

template <typename AgSr> static std::string name_full_without_passage(const AgSr& ag_sr)
{
    return acmacs::string::join(acmacs::string::join_space, ag_sr.name(), fmt::format("{: }", ag_sr.annotations()), ag_sr.reassortant());
}

inline static std::string name_full(const acmacs::chart::Antigen& ag)
{
    return acmacs::string::join(acmacs::string::join_space, name_full_without_passage(ag), ag.passage());
}

inline static std::string name_full(const acmacs::chart::Serum& sr)
{
    return name_full_without_passage(sr);
}

template <typename AgSr> static std::string name_full_passage(const AgSr& ag_sr)
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

std::string acmacs::chart::detail::AntigenSerum::format(std::string_view pattern, collapse_spaces_t cs) const
{
    fmt::memory_buffer output;
    format(output, pattern);
    return acmacs::chart::collapse_spaces(fmt::to_string(output), cs);

} // acmacs::chart::detail::AntigenSerum::format

// ----------------------------------------------------------------------

#define FKF(key, call) std::pair{key, [](fmt::memory_buffer& output, std::string_view format, [[maybe_unused]] const auto& ag_sr) { fmt::format_to(output, format, fmt::arg(key, call)); }}

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

const std::tuple format_subst_ag_sr{
    FKF("abbreviated_location_with_passage_type", acmacs::string::join(acmacs::string::join_space, location_abbreviated(ag_sr), ag_sr.passage().passage_type())), // mapi
    FKF("abbreviated_name_with_passage_type", fmt::format("{}-{}", name_abbreviated(ag_sr), ag_sr.passage().passage_type())),                                     // mapi
    FKF("annotations", ag_sr.annotations()),                                                                                                                      //
    FKF("continent", ag_sr.location_data().continent),                                                                                                            //
    FKF("country", ag_sr.location_data().country),                                                                                                                //
    FKF("fields", fields(ag_sr)),                                                                                                                                 //
    FKF("full_name", name_full(ag_sr)),                                                                                                                           //
    FKF("latitude", ag_sr.location_data().latitude),                                                                                                              //
    FKF("lineage", ag_sr.lineage().to_string()),                                                                                                                  //
    FKF("location", acmacs::virus::location(ag_sr.name())),                                                                                                       //
    FKF("location_abbreviated", location_abbreviated(ag_sr)),                                                                                                     //
    FKF("longitude", ag_sr.location_data().longitude),                                                                                                            //
    FKF("name", ag_sr.name()),                                                                                                                                    //
    FKF("name_abbreviated", name_abbreviated(ag_sr)),                                                                                                             //
    FKF("name_full", name_full(ag_sr)),                                                                                                                           //
    FKF("name_full_passage", name_full_passage(ag_sr)),                                                                                                           //
    FKF("name_without_subtype", acmacs::virus::without_subtype(ag_sr.name())),                                                                                    //
    FKF("passage", ag_sr.passage()),                                                                                                                              //
    FKF("passage_type", ag_sr.passage().passage_type()),                                                                                                          //
    FKF("reassortant", ag_sr.reassortant()),                                                                                                                      //
    FKF("year", year4(ag_sr)),                                                                                                                                    //
    FKF("year2", year2(ag_sr)),                                                                                                                                   //
    FKF("year4", year4(ag_sr)),                                                                                                                                   //
};

const std::tuple format_subst_antigen{
    FKF("ag_sr", "AG"),                                         //
    FKF("date", ag_sr.date()),                                  //
    FKF("date_in_brackets", fmt::format("[{}]", ag_sr.date())), //
    FKF("designation", name_full(ag_sr)),                       //
    FKF("lab_ids", ag_sr.lab_ids().join()),                     //
    FKF("ref", ag_sr.reference() ? "Ref" : ""),                 //
    FKF("serum_species", ""),                                   //
    FKF("species", ""),                                         //
};

const std::tuple format_subst_serum{
    FKF("ag_sr", "SR"),                                                                                                       //
    FKF("designation", acmacs::string::join(acmacs::string::join_space, name_full_without_passage(ag_sr), ag_sr.serum_id())), //
    FKF("designation_without_serum_id", name_full_without_passage(ag_sr)),                                                    //
    FKF("serum_id", ag_sr.serum_id()),                                                                                        //
    FKF("serum_species", ag_sr.serum_species()),                                                                              //
    FKF("serum_species", ag_sr.serum_species()),                                                                              //
    FKF("species", ag_sr.serum_species()),                                                                                    //
    FKF("date_in_brackets", ""),                                                                                              //
    FKF("lab_ids", ""),                                                                                                       //
    FKF("ref", ""),                                                                                                           //
};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

template <typename AgSr, typename... Args> static inline void format_ag_sr(fmt::memory_buffer& output, const AgSr& ag_sr, std::string_view pattern, Args&&... args)
{
    const auto format_matched = [&output, &ag_sr](std::string_view pattern_arg, const auto& key_value) {
        static_assert(std::is_same_v<std::decay_t<decltype(std::get<0>(key_value))>, const char*>);
        if constexpr (std::is_invocable_v<decltype(std::get<1>(key_value)), fmt::memory_buffer&, std::string_view, const AgSr&>)
            std::invoke(std::get<1>(key_value), output, pattern_arg, ag_sr);
        else
            fmt::format_to(output, pattern_arg, fmt::arg(std::get<0>(key_value), std::get<1>(key_value)));
    };
    const auto format_no_pattern = [&output](std::string_view no_pattern) { output.append(no_pattern); };
    const auto format_args = [pattern, format_matched, format_no_pattern](const auto&... fargs) {
        fmt::substitute_to(format_matched, format_no_pattern, pattern, fmt::if_no_substitution_found::leave_as_is, fargs...);
    };
    std::apply(format_args, std::tuple_cat(args...));
}

// ----------------------------------------------------------------------


void acmacs::chart::Antigen::format(fmt::memory_buffer& output, std::string_view pattern) const
{
    ::format_ag_sr(output, *this, pattern, format_subst_ag_sr, format_subst_antigen);

} // acmacs::chart::Antigen::format

// ----------------------------------------------------------------------

void acmacs::chart::Serum::format(fmt::memory_buffer& output, std::string_view pattern) const
{
    ::format_ag_sr(output, *this, pattern, format_subst_ag_sr, format_subst_serum);

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

#define CS_SPACES "([ \t])"
#define CS_SPACES_OPT CS_SPACES "([ \t]*)"
#define CS_SPACES_OR_CS_OPT "( |\t|\\{ \\})*"
#define CS_CS1 "(\\{ \\})+"

std::string acmacs::chart::collapse_spaces(std::string src, collapse_spaces_t cs)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::array replace_data{
        acmacs::regex::look_replace_t{std::regex("^" CS_SPACES_OPT CS_CS1, std::regex::icase), {"$1$'"}},
        acmacs::regex::look_replace_t{std::regex(CS_CS1 CS_SPACES_OPT "$", std::regex::icase), {"$`$1"}},
        acmacs::regex::look_replace_t{std::regex(CS_SPACES_OR_CS_OPT CS_CS1 CS_SPACES_OR_CS_OPT, std::regex::icase), {"$` $'"}},
    };
#include "acmacs-base/diagnostics-pop.hh"

    switch (cs) {
        case collapse_spaces_t::yes:
            while (true) {
                if (const auto replacement = scan_replace(src, replace_data); replacement.has_value())
                    src = replacement->back();
                else
                    break;
            }
            break;
        case collapse_spaces_t::no:
            break;
    }
    return src;

} // acmacs::chart::collapse_spaces

// ----------------------------------------------------------------------

std::string acmacs::chart::format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no, collapse_spaces_t cs)
{
    const auto num_digits = chart.number_of_digits_for_antigen_serum_index_formatting();
    auto antigen = chart.antigens()->at(antigen_no);

    const auto ag_formatted = antigen->format(pattern, collapse_spaces_t::no);
    try {
        const auto substituted = fmt::substitute(                                                                                     //
            ag_formatted,                                                                                                             //
            fmt::if_no_substitution_found::leave_as_is,                                                                               //
            std::tuple{"no0", antigen_no, "num_digits", num_digits},                                                                  //
            std::tuple{"no1", antigen_no + 1, "num_digits", num_digits},                                                                  //
            std::pair{"sera_with_titrations", [&chart, antigen_no] { return chart.titers()->having_titers_with(antigen_no, false); }} //
        );
        return acmacs::chart::collapse_spaces(substituted, cs);
    }
    catch (fmt::format_error& err) {
        AD_ERROR("format_error in {}: {}", ag_formatted, err);
        throw;
    }

} // acmacs::chart::format_antigen

// ----------------------------------------------------------------------

std::string acmacs::chart::format_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t serum_no, collapse_spaces_t cs)
{
    const auto num_digits = chart.number_of_digits_for_antigen_serum_index_formatting();
    auto serum = chart.sera()->at(serum_no);

    const auto substituted = fmt::substitute(                                                                        //
        serum->format(pattern, collapse_spaces_t::no),                                                                 //
        std::tuple{"no0", serum_no, "num_digits", num_digits},                                                                  //
        std::tuple{"no1", serum_no + 1, "num_digits", num_digits},                                                                  //
        std::pair{"sera_with_titrations", chart.titers()->having_titers_with(serum_no + chart.number_of_antigens())} //
    );

    return acmacs::chart::collapse_spaces(substituted, cs);

} // acmacs::chart::format_serum

// ----------------------------------------------------------------------

constexpr const std::string_view pattern = R"(
{{ag_sr}}                                  : {ag_sr}
{{no0}}                                    : {no0}
{{no0:{num_digits}d}}                      : {no0:{num_digits}d}
{{no1}}                                    : {no1}
{{no1:{num_digits}d}}                      : {no1:{num_digits}d}
{{name}}                                   : {name}
{{full_name}}                              : {full_name}
{{name_full_passage}}                      : {name_full_passage}
{{fields}}                                 : {fields}
{{species}}                                : {species}
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

    return fmt::format("{}\n\n{}\n", format_antigen(pattern, chart, 67, acmacs::chart::collapse_spaces_t::yes), format_serum(pattern, chart, 12, acmacs::chart::collapse_spaces_t::yes));

} // acmacs::chart::format_help

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
