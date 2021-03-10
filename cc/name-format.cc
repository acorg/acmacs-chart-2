#include <functional>

#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/regex.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

#define COLLAPSABLE_SPACE "\x01"

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

std::string acmacs::chart::detail::AntigenSerum::format(std::string_view pattern) const
{
    fmt::memory_buffer output;
    format(output, pattern);
    // acmacs::chart::collapse_spaces(fmt::to_string(output));
    return fmt::to_string(output);

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
    const auto format_matched = [&output, &ag_sr](std::string_view pattern_arg, std::string_view key, const auto& value) {
        if constexpr (std::is_invocable_v<decltype(value), fmt::memory_buffer&, std::string_view, const AgSr&>)
            std::invoke(value, output, pattern_arg, ag_sr);
        else
            fmt::format_to(output, pattern_arg, fmt::arg(key, value));
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

namespace fmt
{
    template <typename FormatMatched, typename FormatNoPattern, typename... Args>
    void x_substitute_to(FormatMatched&& format_matched, FormatNoPattern&& format_no_pattern, std::string_view pattern, if_no_substitution_found insf, Args&&... args)
    {
        const auto match_and_format = [&format_matched](std::string_view look_for, std::string_view pattern_arg, const auto& en) {
            if (look_for == en.first) {
                format_matched(pattern_arg, en.first, en.second);
                return true;
            }
            else
                return false;
        };

        for (const auto& [key, pattern_arg] : split_for_formatting(pattern)) {
            if (!key.empty()) {
                if (!(match_and_format(key, pattern_arg, args) || ...)) {
                    // not matched
                    switch (insf) {
                        case if_no_substitution_found::leave_as_is:
                            format_no_pattern(pattern_arg);
                            break;
                        case if_no_substitution_found::empty:
                            break;
                    }
                }
            }
            else
                format_no_pattern(pattern_arg);
        }
    }
}

std::string acmacs::chart::format_antigen(std::string_view pattern, const acmacs::chart::Chart& chart, size_t antigen_no)
{
    auto antigens = chart.antigens();
    auto antigen = antigens->at(antigen_no);
    auto sera = chart.sera();
    const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;

    return fmt::substitute(                                                               //
        antigen->format(pattern),                                                         //
        // fmt::if_no_substitution_found::leave_as_is, //
        std::pair{"no0", [=] { return fmt::format("{:{}d}", antigen_no, num_digits); }},  //
        std::pair{"no0_left", antigen_no},                             //
        std::pair{"no1", [=] { return fmt::format("{:{}d}", antigen_no + 1, num_digits); }},              //
        std::pair{"no1_left", antigen_no + 1},                         //
        std::pair{"sera_with_titrations", [&chart, antigen_no] { return chart.titers()->having_titers_with(antigen_no, false); }} //
    );

} // acmacs::chart::format_antigen

// ----------------------------------------------------------------------

std::string acmacs::chart::format_serum(std::string_view pattern, const acmacs::chart::Chart& chart, size_t serum_no)
{
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto serum = sera->at(serum_no);
    const auto num_digits = static_cast<int>(std::log10(std::max(antigens->size(), sera->size()))) + 1;

    return fmt::substitute(     //
        serum->format(pattern), //
        // fmt::if_no_substitution_found::leave_as_is, //
        std::pair{"no0", [=] { return fmt::format("{:{}d}", serum_no, num_digits); }},                               //
        std::pair{"no0_left", fmt::format("{}", serum_no)},                                                          //
        std::pair{"no1", fmt::format("{:{}d}", serum_no + 1, num_digits)},                                           //
        std::pair{"no1_left", fmt::format("{}", serum_no + 1)},                                                      //
        std::pair{"sera_with_titrations", chart.titers()->having_titers_with(serum_no + chart.number_of_antigens())} //
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
