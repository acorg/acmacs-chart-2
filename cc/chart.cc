#include "acmacs-base/string.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/regex.hh"
#include "acmacs-base/counter.hh"
#include "acmacs-virus/virus-name-v1.hh"
#include "locationdb/locdb.hh"
#include "acmacs-whocc-data/labs.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/serum-circle.hh"

// ----------------------------------------------------------------------

// #include "acmacs-base/global-constructors-push.hh"
// static const std::regex sDate{"[12][90][0-9][0-9]-[0-2][0-9]-[0-3][0-9]"};
// #include "acmacs-base/diagnostics-pop.hh"

// void acmacs::chart::Date::checkx() const
// {
//     if (!empty() && !std::regex_match(std::begin(this->get()), std::end(this->get()), sDate))
//         throw invalid_data{fmt::format("invalid date (YYYY-MM-DD expected): {}", **this)};

// } // acmacs::chart::Date::check

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_info(size_t max_number_of_projections_to_show, unsigned inf) const
{
    fmt::memory_buffer text;
    fmt::format_to(text, "{}\nAntigens: {}   Sera: {}\n", info()->make_info(), number_of_antigens(), number_of_sera());
    if (const auto layers = titers()->number_of_layers(); layers)
        fmt::format_to(text, "Number of layers: {}\n", layers);
    if (const auto having_too_few_numeric_titers = titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers->empty())
        fmt::format_to(text, "Points having too few numeric titers:{} {}\n", having_too_few_numeric_titers->size(), having_too_few_numeric_titers);

    if (inf & info_data::column_bases) {
        auto cb = computed_column_bases(acmacs::chart::MinimumColumnBasis{});
        fmt::format_to(text, "computed column bases:                 {:5.2f}\n", *cb);
        for (auto projection_no : range_from_0_to(number_of_projections())) {
            if (auto fcb = projection(projection_no)->forced_column_bases(); fcb) {
                fmt::format_to(text, "forced column bases for projection {:2d}: {:5.2f}\n", projection_no, *fcb);
                fmt::format_to(text, "                                 diff: [");
                for (const auto sr_no : range_from_0_to(cb->size())) {
                    if (float_equal(cb->column_basis(sr_no), fcb->column_basis(sr_no)))
                        fmt::format_to(text, "  .   ");
                    else
                        fmt::format_to(text, "{:5.2f} ", cb->column_basis(sr_no) - fcb->column_basis(sr_no));
                }
                fmt::format_to(text, "]\n");
            }
        }
    }

    fmt::format_to(text, "{}\n", projections()->make_info(max_number_of_projections_to_show));

    if (inf & info_data::tables && info()->number_of_sources() > 0) {
        fmt::format_to(text, "\nTables:\n");
        for (const auto src_no : range_from_0_to(info()->number_of_sources()))
            fmt::format_to(text, "{:3d} {}\n", src_no, info()->source(src_no)->make_name());
    }

    if (inf & info_data::tables_for_sera && info()->number_of_sources() > 0) {
        auto titers = this->titers();
        auto sera = this->sera();
        for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
            fmt::format_to(text, "SR {:3d} {}\n", sr_no, serum->full_name_with_passage());
            for (const auto layer_no : titers->layers_with_serum(sr_no))
                fmt::format_to(text, "    {:3d} {}\n", layer_no, info()->source(layer_no)->make_name());
        }
    }

    if (inf & info_data::dates) {
        acmacs::Counter<std::string> dates;
        auto antigens = this->antigens();
        for (const auto antigen : *antigens) {
            if (const auto date = antigen->date(); !date.empty())
                dates.count(date->substr(0, 7));
            else
                dates.count("*empty*");
        }
        fmt::format_to(text, "Antigen dates ({})\n", dates.size());
        for (const auto& [date, count] : dates.counter())
            fmt::format_to(text, "    {:7s} {:4d}\n", date, count);
    }

    return fmt::to_string(text);

} // acmacs::chart::Chart::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_name(std::optional<size_t> aProjectionNo) const
{
    fmt::memory_buffer name;
    fmt::format_to(name, "{}", info()->make_name());
    if (auto prjs = projections(); !prjs->empty() && (!aProjectionNo || *aProjectionNo < prjs->size())) {
        auto prj = (*prjs)[aProjectionNo ? *aProjectionNo : 0];
        fmt::format_to(name, " {}", prj->minimum_column_basis().format(">={}", MinimumColumnBasis::use_none::no));
        if (const auto stress = prj->stress(); !std::isnan(stress))
            fmt::format_to(name, " {:.4f}", stress);
    }
    return fmt::to_string(name);

} // acmacs::chart::Chart::make_name

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::description() const
{
    fmt::memory_buffer desc;
    fmt::format_to(desc, "{}", info()->make_name());
    if (auto prjs = projections(); !prjs->empty()) {
        auto prj = (*prjs)[0];
        fmt::format_to(desc, "{}", prj->minimum_column_basis().format(">={}", MinimumColumnBasis::use_none::yes));
        if (const auto stress = prj->stress(); !std::isnan(stress))
            fmt::format_to(desc, " {:.4f}", stress);
    }
    if (info()->virus_type() == acmacs::virus::type_subtype_t{"B"})
        fmt::format_to(desc, " {}", lineage());
    fmt::format_to(desc, " AG:{} Sr:{}", number_of_antigens(), number_of_sera());
    if (const auto layers = titers()->number_of_layers(); layers > 1)
        fmt::format_to(desc, " ({} source tables)", layers);
    return fmt::to_string(desc);

} // acmacs::chart::Chart::description

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Chart::computed_column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis, use_cache a_use_cache) const
{
    if (a_use_cache == use_cache::yes) {
        if (auto found = computed_column_bases_.find(aMinimumColumnBasis); found != computed_column_bases_.end())
            return found->second;
    }
    return computed_column_bases_[aMinimumColumnBasis] = titers()->computed_column_bases(aMinimumColumnBasis);

} // acmacs::chart::Chart::computed_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Chart::column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis) const
{
    if (auto cb = forced_column_bases(aMinimumColumnBasis); cb)
        return cb;
    return computed_column_bases(aMinimumColumnBasis);

} // acmacs::chart::Chart::column_bases

// ----------------------------------------------------------------------

double acmacs::chart::Chart::column_basis(size_t serum_no, size_t projection_no) const
{
    auto prj = projection(projection_no);
    if (auto forced = prj->forced_column_bases(); forced)
        return forced->column_basis(serum_no);
    else
        return computed_column_bases(prj->minimum_column_basis(), use_cache::yes)->column_basis(serum_no);

} // acmacs::chart::Chart::column_basis

// ----------------------------------------------------------------------

acmacs::virus::lineage_t acmacs::chart::Chart::lineage() const
{
    std::map<BLineage, size_t> lineages;
    auto ags = antigens();
    for (auto antigen: *ags) {
        if (const auto lineage = antigen->lineage(); lineage != BLineage::Unknown)
            ++lineages[lineage];
    }
    switch (lineages.size()) {
      case 0:
          return {};
      case 1:
          return lineages.begin()->first;
      default:
          return std::max_element(lineages.begin(), lineages.end(), [](const auto& a, const auto& b) -> bool { return a.second < b.second; })->first;
    }
    // return {};

} // acmacs::chart::Chart::lineage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::serum_coverage(Titer aHomologousTiter, size_t aSerumNo, Indexes& aWithinFold, Indexes& aOutsideFold, double aFold) const
{
    if (!aHomologousTiter.is_regular())
        throw serum_coverage_error(fmt::format("cannot handle non-regular homologous titer: {}", *aHomologousTiter));
    const double titer_threshold = aHomologousTiter.logged() - aFold;
    if (titer_threshold <= 0)
        throw serum_coverage_error(fmt::format("homologous titer is too low: {}", *aHomologousTiter));
    // AD_DEBUG("titer_threshold {}", titer_threshold);
    auto tts = titers();
    for (size_t ag_no = 0; ag_no < number_of_antigens(); ++ag_no) {
        const Titer titer = tts->titer(ag_no, aSerumNo);
        const double value = titer.is_dont_care() ? -1 : titer.logged_for_column_bases();
        // AD_DEBUG("{} -> {}", titer, value);
        if (value >= titer_threshold)
            aWithinFold.insert(ag_no);
        else if (value >= 0 && value < titer_threshold)
            aOutsideFold.insert(ag_no);
    }
    if (aWithinFold->empty()) {
        AD_WARNING("no antigens within 4fold from homologous titer (for serum coverage)");
        // throw serum_coverage_error("no antigens within 4fold from homologous titer (for serum coverage)"); // BUG? at least homologous antigen must be there!
    }

} // acmacs::chart::Chart::serum_coverage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::serum_coverage(size_t aAntigenNo, size_t aSerumNo, Indexes& aWithinFold, Indexes& aOutsideFold, double aFold) const
{
    serum_coverage(titers()->titer(aAntigenNo, aSerumNo), aSerumNo, aWithinFold, aOutsideFold, aFold);

} // acmacs::chart::Chart::serum_coverage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::set_homologous(find_homologous options, SeraP aSera, acmacs::debug dbg) const
{
    if (!aSera)
        aSera = sera();
    aSera->set_homologous(options, *antigens(), dbg);

} // acmacs::chart::Chart::set_homologous

// ----------------------------------------------------------------------

acmacs::PointStyle acmacs::chart::Chart::default_style(acmacs::chart::Chart::PointType aPointType) const
{
    acmacs::PointStyle style;
    style.outline(BLACK);
    switch (aPointType) {
      case PointType::TestAntigen:
          style.shape(acmacs::PointShape::Circle);
          style.size(Pixels{5.0});
          style.fill(GREEN);
          break;
      case PointType::ReferenceAntigen:
          style.shape(acmacs::PointShape::Circle);
          style.size(Pixels{8.0});
          style.fill(TRANSPARENT);
          break;
      case PointType::Serum:
          style.shape(acmacs::PointShape::Box);
          style.size(Pixels{6.5});
          style.fill(TRANSPARENT);
          break;
    }
    return style;

} // acmacs::chart::Chart::default_style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> acmacs::chart::Chart::default_all_styles() const
{
    auto ags = antigens();
    auto srs = sera();
    std::vector<acmacs::PointStyle> result(ags->size() + srs->size());
    for (size_t ag_no = 0; ag_no < ags->size(); ++ag_no)
        result[ag_no] = default_style((*ags)[ag_no]->reference() ? PointType::ReferenceAntigen : PointType::TestAntigen);
    for (auto ps = result.begin() + static_cast<typename decltype(result.begin())::difference_type>(ags->size()); ps != result.end(); ++ps)
        *ps = default_style(PointType::Serum);
    return result;

} // acmacs::chart::Chart::default_all_styles

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::show_table(std::optional<size_t> layer_no) const
{
    fmt::memory_buffer output;

    // auto sr_label = [](size_t sr_no) -> char { return static_cast<char>('A' + sr_no); };
    auto sr_label = [](size_t sr_no) -> size_t { return sr_no + 1; };

    auto ags = antigens();
    auto srs = sera();
    auto tt = titers();
    PointIndexList antigen_indexes, serum_indexes;
    if (layer_no) {
        std::tie(antigen_indexes, serum_indexes) = tt->antigens_sera_of_layer(*layer_no);
    }
    else {
        antigen_indexes = PointIndexList{filled_with_indexes(ags->size())};
        serum_indexes = PointIndexList{filled_with_indexes(srs->size())};
    }

    const auto max_ag_name = static_cast<int>(max_full_name(*ags));

    fmt::format_to(output, "{:>{}s}Serum full names are under the table\n{:>{}s}", "", max_ag_name + 6, "", max_ag_name);
    for (auto sr_ind : range_from_0_to(serum_indexes->size()))
        fmt::format_to(output, "{:>7d}", sr_label(sr_ind));
    fmt::format_to(output, "\n");

    fmt::format_to(output, "{:{}s}", "", max_ag_name + 2);
    for (auto sr_no : serum_indexes)
        fmt::format_to(output, "{:>7s}", srs->at(sr_no)->abbreviated_location_year());
    fmt::format_to(output, "\n");

    for (auto ag_no : antigen_indexes) {
        fmt::format_to(output, "{:<{}s}", ags->at(ag_no)->full_name(), max_ag_name + 2);
        for (auto sr_no : serum_indexes)
            fmt::format_to(output, "{:>7s}", *tt->titer(ag_no, sr_no));
        fmt::format_to(output, "\n");
    }
    fmt::format_to(output, "\n");

    for (auto [sr_ind, sr_no] : acmacs::enumerate(serum_indexes))
        fmt::format_to(output, "{:3d} {} {}\n", sr_label(sr_ind), srs->at(sr_no)->abbreviated_location_year(), srs->at(sr_no)->full_name());

    return fmt::to_string(output);

} // acmacs::chart::Chart::show_table

// ----------------------------------------------------------------------

bool acmacs::chart::same_tables(const Chart& c1, const Chart& c2, bool verbose)
{
    if (!equal(*c1.antigens(), *c2.antigens(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: antigen sets are different\n");
        return false;
    }

    if (!equal(*c1.sera(), *c2.sera(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: serum sets are different\n");
        return false;
    }

    if (!equal(*c1.titers(), *c2.titers(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: titers are different\n");
        return false;
    }

    return true;

} // acmacs::chart::same_tables

// ----------------------------------------------------------------------

acmacs::chart::BLineage::Lineage acmacs::chart::BLineage::from(char aSource)
{
    switch (aSource) {
        case 'Y':
            return Yamagata;
        case 'V':
            return Victoria;
    }
    return Unknown;

} // acmacs::chart::BLineage::from

// ----------------------------------------------------------------------

acmacs::chart::BLineage::Lineage acmacs::chart::BLineage::from(std::string_view aSource)
{
    return aSource.empty() ? Unknown : from(aSource[0]);

} // acmacs::chart::BLineage::from

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_info() const
{
    const auto n_sources = number_of_sources();
    return acmacs::string::join(acmacs::string::join_space, name(), *virus(Compute::Yes), lab(Compute::Yes), virus_type(Compute::Yes), subset(Compute::Yes), assay(Compute::Yes), rbc_species(Compute::Yes),
                                date(Compute::Yes), n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{});

} // acmacs::chart::Info::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_name() const
{
    std::string n = name(Compute::No);
    if (n.empty()) {
        const auto vt = virus_type(Compute::Yes);
        n = acmacs::string::join(acmacs::string::join_space, lab(Compute::Yes), *virus_not_influenza(Compute::Yes), vt, subset(Compute::Yes), assay(Compute::Yes).HI_or_Neut(Assay::no_hi::yes),
                                 rbc_species(Compute::Yes), date(Compute::Yes));
    }
    return n;

} // acmacs::chart::Info::make_name

// ----------------------------------------------------------------------

size_t acmacs::chart::Info::max_source_name() const
{
    if (number_of_sources() < 2)
        return 0;
    size_t msn = 0;
    for (auto s_no : range_from_0_to(number_of_sources()))
        msn = std::max(msn, source(s_no)->name().size());
    return msn;

} // acmacs::chart::Info::max_source_name

// ----------------------------------------------------------------------

acmacs::Lab acmacs::chart::Info::fix_lab_name(Lab source, FixLab fix) const
{
    switch (fix) {
        case FixLab::no:
            break;
        case FixLab::yes:
            source = acmacs::whocc::lab_name_normalize(source);
            break;
        case FixLab::reverse:
            source = acmacs::whocc::lab_name_old(source);
            break;
    }
    return source;

} // acmacs::chart::Info::fix_lab_name

// ----------------------------------------------------------------------

std::string acmacs::chart::Projection::make_info() const
{
    fmt::memory_buffer result;
    auto lt = layout();
    fmt::format_to(result, "{:.14f} {}d", stress(), lt->number_of_dimensions());
    if (auto cmt = comment(); !cmt.empty())
        fmt::format_to(result, " <{}>", cmt);
    if (auto fcb = forced_column_bases(); fcb)
        fmt::format_to(result, " forced-column-bases"); // fcb
    else
        fmt::format_to(result, " >={}", minimum_column_basis());
    return fmt::to_string(result);

} // acmacs::chart::Projection::make_info

// ----------------------------------------------------------------------

double acmacs::chart::Projection::stress(acmacs::chart::RecalculateStress recalculate) const
{
    switch (recalculate) {
      case RecalculateStress::yes:
          return recalculate_stress();
      case RecalculateStress::if_necessary:
          if (const auto s = stored_stress(); s)
              return *s;
          else
              return recalculate_stress();
      case RecalculateStress::no:
          if (const auto s = stored_stress(); s)
              return *s;
          else
              return InvalidStress;
    }
    throw invalid_data("Projection::stress: internal");

} // acmacs::chart::Projection::stress

// ----------------------------------------------------------------------

double acmacs::chart::Projection::stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const
{
    acmacs::Layout new_layout(*layout());
    new_layout.update(point_no, move_to);
    return stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes).value(new_layout);

} // acmacs::chart::Projection::stress_with_moved_point

// ----------------------------------------------------------------------

acmacs::chart::Blobs acmacs::chart::Projection::blobs(double stress_diff, size_t number_of_drections, double stress_diff_precision) const
{
    Blobs blobs(stress_diff, number_of_drections, stress_diff_precision);
    blobs.calculate(*layout(), stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes));
    return blobs;

} // acmacs::chart::Projection::blobs

// ----------------------------------------------------------------------

acmacs::chart::Blobs acmacs::chart::Projection::blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections, double stress_diff_precision) const
{
    Blobs blobs(stress_diff, number_of_drections, stress_diff_precision);
    blobs.calculate(*layout(), points, stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes));
    return blobs;

} // acmacs::chart::Projection::blobs

// ----------------------------------------------------------------------

std::string acmacs::chart::Projections::make_info(size_t max_number_of_projections_to_show) const
{
    fmt::memory_buffer text;
    fmt::format_to(text, "Projections: {}", size());
    for (auto projection_no: range_from_0_to(std::min(max_number_of_projections_to_show, size())))
        fmt::format_to(text, "\n{:3d} {}", projection_no, operator[](projection_no)->make_info());
    return fmt::to_string(text);

} // acmacs::chart::Projections::make_info

// ----------------------------------------------------------------------

// size_t acmacs::chart::Projections::projection_no(const acmacs::chart::Projection* projection) const
// {
//     std::cerr << "projection_no " << projection << '\n';
//     for (size_t index = 0; index < size(); ++index) {
//         std::cerr << "p " << index << ' ' << operator[](index).get() << '\n';
//         if (operator[](index).get() == projection)
//             return index;
//     }
//     throw invalid_data("cannot find projection_no, total projections: " + std::to_string(size()));

// } // acmacs::chart::Projections::projection_no

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::full_name_with_fields() const
{
    std::string r{name()};
    if (const auto value = string::join(acmacs::string::join_space, annotations()); !value.empty())
        r += " annotations=\"" + value + '"';
    if (const auto value = reassortant(); !value.empty())
        r += " reassortant=\"" + *value + '"';
    if (const auto value = passage(); !value.empty())
        r += fmt::format(" passage=\"{}\" ptype={}", *value, value.passage_type());
    if (const auto value = date(); !value.empty())
        r += " date=" + *value;
    if (const auto value = lineage(); value != BLineage::Unknown)
        r += fmt::format(" lineage={}", value);
    if (reference())
        r += " reference";
    if (const auto value = string::join(acmacs::string::join_space, lab_ids()); !value.empty())
        r += " lab_ids=\"" + value + '"';
    return r;

} // acmacs::chart::Antigen::full_name_with_fields

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::full_name_with_fields() const
{
    std::string r{name()};
    if (const auto value = string::join(acmacs::string::join_space, annotations()); !value.empty())
        r += " annotations=\"" + value + '"';
    if (const auto value = reassortant(); !value.empty())
        r += " reassortant=\"" + *value + '"';
    if (const auto value = serum_id(); !value.empty())
        r += " serum_id=\"" + *value + '"';
    if (const auto value = passage(); !value.empty())
        r += fmt::format(" passage=\"{}\" ptype={}", *value, value.passage_type());
    if (const auto value = serum_species(); !value.empty())
        r += " serum_species=\"" + *value + '"';
    if (const auto value = lineage(); value != BLineage::Unknown)
        r += fmt::format(" lineage={}", value);
    return r;

} // acmacs::chart::Serum::full_name_with_fields

// ----------------------------------------------------------------------

// std::string name_abbreviated(std::string aName);
static inline std::string name_abbreviated(std::string_view aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        return acmacs::string::join(acmacs::string::join_slash, acmacs::locationdb::get().abbreviation(location), isolation, year.substr(2));
    }
    catch (virus_name::Unrecognized&) {
        return std::string{aName};
    }

} // name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::name_abbreviated() const
{
    return ::name_abbreviated(name());

} // acmacs::chart::Antigen::name_abbreviated

// ----------------------------------------------------------------------

static inline std::string name_without_subtype(std::string_view aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        if (virus_type.size() > 1 && virus_type[0] == 'A' && virus_type[1] == '(')
            virus_type.resize(1);
        return acmacs::string::join(acmacs::string::join_slash, virus_type, host, location, isolation, year);
    }
    catch (virus_name::Unrecognized&) {
        return std::string{aName};
    }
}

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::name_without_subtype() const
{
    return ::name_without_subtype(name());

} // acmacs::chart::Antigen::name_without_subtype

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::location() const
{
    try {
        return ::virus_name::location(name());
    }
    catch (virus_name::Unrecognized&) {
        return "*no-location-extracted*";
    }

} // acmacs::chart::Antigen::location

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::location_abbreviated() const
{
    try {
        return acmacs::locationdb::get().abbreviation(::virus_name::location(name()));
    }
    catch (virus_name::Unrecognized&) {
        return "*no-location-extracted*";
    }

} // acmacs::chart::Antigen::location_abbreviated

// ----------------------------------------------------------------------

inline std::string abbreviated_location_year(std::string_view aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        return acmacs::string::join(acmacs::string::join_slash, acmacs::locationdb::get().abbreviation(location), year.substr(2, 2));
    }
    catch (virus_name::Unrecognized&) {
        return std::string{aName};
    }
}

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::abbreviated_location_year() const
{
    return ::abbreviated_location_year(name());

} // acmacs::chart::Antigen::abbreviated_location_year

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::name_abbreviated() const
{
    return ::name_abbreviated(name());

} // acmacs::chart::Serum::name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::name_without_subtype() const
{
    return ::name_without_subtype(name());

} // acmacs::chart::Serum::name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::location() const
{
    try {
        return ::virus_name::location(name());
    }
    catch (virus_name::Unrecognized&) {
        return "*no-location-extracted*";
    }

} // acmacs::chart::Serum::location

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::location_abbreviated() const
{
    try {
        return acmacs::locationdb::get().abbreviation(::virus_name::location(name()));
    }
    catch (virus_name::Unrecognized&) {
        return "*no-location-extracted*";
    }

} // acmacs::chart::Serum::location_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::abbreviated_location_year() const
{
    return ::abbreviated_location_year(name());

} // acmacs::chart::Serum::abbreviated_location_year

// ----------------------------------------------------------------------

static inline bool not_in_country(std::string_view aName, std::string_view aCountry)
{
    try {
        return acmacs::locationdb::get().country(virus_name::location(aName)) != aCountry;
    }
    catch (std::exception&) {
        return true;
    }

} // AntigensSera<AgSr>::filter_country

// ----------------------------------------------------------------------

static inline bool not_in_continent(std::string_view aName, std::string_view aContinent)
{
    try {
        return acmacs::locationdb::get().continent(virus_name::location(aName)) != aContinent;
    }
    catch (std::exception&) {
        return true;
    }

} // AntigensSera<AgSr>::filter_continent

// ----------------------------------------------------------------------

template <typename AgSr> acmacs::chart::duplicates_t find_duplicates(const AgSr& ag_sr)
{
    std::map<std::string, std::vector<size_t>> designations_to_indexes;
    for (size_t index = 0; index < ag_sr.size(); ++index) {
        auto [pos, inserted] = designations_to_indexes.insert({ag_sr[index]->designation(), {}});
        pos->second.push_back(index);
    }

    acmacs::chart::duplicates_t result;
    for (auto [designation, indexes] : designations_to_indexes) {
        if (indexes.size() > 1 && designation.find(" DISTINCT") == std::string::npos) {
            result.push_back(indexes);
        }
    }
    return result;
}

// ----------------------------------------------------------------------

acmacs::chart::duplicates_t acmacs::chart::Antigens::find_duplicates() const
{
    return ::find_duplicates(*this);

} // acmacs::chart::Antigens::find_duplicates

// ----------------------------------------------------------------------

void acmacs::chart::Antigens::filter_country(Indexes& aIndexes, std::string_view aCountry) const
{
    remove(aIndexes, [aCountry](const auto& entry) { return not_in_country(entry.name(), aCountry); });

} // acmacs::chart::Antigens::filter_country

// ----------------------------------------------------------------------

void acmacs::chart::Antigens::filter_continent(Indexes& aIndexes, std::string_view aContinent) const
{
    remove(aIndexes, [aContinent](const auto& entry) { return not_in_continent(entry.name(), aContinent); });

} // acmacs::chart::Antigens::filter_continent

// ----------------------------------------------------------------------

std::vector<acmacs::chart::Date> acmacs::chart::Antigens::all_dates(include_reference inc_ref) const
{
    std::vector<acmacs::chart::Date> dates;
    for (auto antigen : *this) {
        if (inc_ref == include_reference::yes || !antigen->reference()) {
            if (auto date{antigen->date()}; !date.empty())
                dates.push_back(std::move(date));
        }
    }
    std::sort(std::begin(dates), std::end(dates));
    dates.erase(std::unique(std::begin(dates), std::end(dates)), std::end(dates));
    return dates;

} // acmacs::chart::Antigens::all_dates

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
static const std::regex sAnntotationToIgnore{"(CONC|RDE@|BOOST|BLEED|LAIV|^CDC$)"};
#include "acmacs-base/diagnostics-pop.hh"

bool acmacs::chart::Annotations::match_antigen_serum(const acmacs::chart::Annotations& antigen, const acmacs::chart::Annotations& serum)
{
    std::vector<std::string_view> antigen_fixed(antigen->size());
    auto antigen_fixed_end = antigen_fixed.begin();
    for (const auto& anno : antigen) {
        *antigen_fixed_end++ = anno;
    }
    antigen_fixed.erase(antigen_fixed_end, antigen_fixed.end());
    std::sort(antigen_fixed.begin(), antigen_fixed.end());

    std::vector<std::string_view> serum_fixed(serum->size());
    auto serum_fixed_end = serum_fixed.begin();
    for (const auto& anno : serum) {
        const std::string_view annos = static_cast<std::string_view>(anno);
        if (!std::regex_search(std::begin(annos), std::end(annos), sAnntotationToIgnore))
            *serum_fixed_end++ = anno;
    }
    serum_fixed.erase(serum_fixed_end, serum_fixed.end());
    std::sort(serum_fixed.begin(), serum_fixed.end());

    return antigen_fixed == serum_fixed;

} // acmacs::chart::Annotations::match_antigen_serum

// ----------------------------------------------------------------------

acmacs::chart::Sera::homologous_canditates_t acmacs::chart::Sera::find_homologous_canditates(const Antigens& aAntigens, acmacs::debug dbg) const
{
    const auto match_passage = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    std::map<std::string, std::vector<size_t>, std::less<>> antigen_name_index;
    for (auto [ag_no, antigen] : acmacs::enumerate(aAntigens))
        antigen_name_index.emplace(antigen->name(), std::vector<size_t>{}).first->second.push_back(ag_no);

    acmacs::chart::Sera::homologous_canditates_t result(size());
    for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
        if (auto ags = antigen_name_index.find(*serum->name()); ags != antigen_name_index.end()) {
            for (auto ag_no : ags->second) {
                auto antigen = aAntigens[ag_no];
                if (dbg == debug::yes)
                    fmt::print(stderr, "DEBUG: SR {} {} R:{} A:{} P:{} -- AG {} {} R:{} A:{} P:{} -- A_match:{} R_match: {} P_match:{}\n",
                               sr_no, *serum->name(), serum->annotations(), *serum->reassortant(), *serum->passage(),
                               ag_no, *antigen->name(), antigen->annotations(), *antigen->reassortant(), *antigen->passage(),
                               Annotations::match_antigen_serum(antigen->annotations(), serum->annotations()), antigen->reassortant() == serum->reassortant(),
                               match_passage(antigen->passage(), serum->passage(), *serum));
                if (Annotations::match_antigen_serum(antigen->annotations(), serum->annotations()) && antigen->reassortant() == serum->reassortant() &&
                    match_passage(antigen->passage(), serum->passage(), *serum)) {
                    result[sr_no].insert(ag_no);
                }
            }
        }
    }

    return result;

} // acmacs::chart::Sera::find_homologous_canditates

// ----------------------------------------------------------------------

void acmacs::chart::Sera::set_homologous(find_homologous options, const Antigens& aAntigens, acmacs::debug dbg)
{
    const auto match_passage_strict = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage == serum_passage;
    };

    const auto match_passage_relaxed = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    const auto homologous_canditates = find_homologous_canditates(aAntigens, dbg);

    if (options == find_homologous::all) {
        for (auto [sr_no, serum] : acmacs::enumerate(*this))
            serum->set_homologous(*homologous_canditates[sr_no], dbg);
    }
    else {
        std::vector<std::optional<size_t>> homologous(size()); // for each serum
        for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
            const auto& canditates = homologous_canditates[sr_no];
            for (auto canditate : canditates) {
                if (match_passage_strict(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                    homologous[sr_no] = canditate;
                    break;
                }
            }
        }

        if (options != find_homologous::strict) {
            for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
                if (!homologous[sr_no]) {
                    const auto& canditates = homologous_canditates[sr_no];
                    for (auto canditate : canditates) {
                        const auto occupied = std::any_of(homologous.begin(), homologous.end(), [canditate](std::optional<size_t> ag_no) -> bool { return ag_no && *ag_no == canditate; });
                        if (!occupied && match_passage_relaxed(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                            homologous[sr_no] = canditate;
                            break;
                        }
                    }
                }
            }

            if (options != find_homologous::relaxed_strict) {
                for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
                    if (!homologous[sr_no]) {
                        const auto& canditates = homologous_canditates[sr_no];
                        for (auto canditate : canditates) {
                            if (match_passage_relaxed(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                                homologous[sr_no] = canditate;
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (auto [sr_no, serum] : acmacs::enumerate(*this))
            if (const auto homol = homologous[sr_no]; homol)
                serum->set_homologous({*homol}, dbg);
    }

} // acmacs::chart::Sera::set_homologous

// ----------------------------------------------------------------------

acmacs::chart::duplicates_t acmacs::chart::Sera::find_duplicates() const
{
    return ::find_duplicates(*this);

} // acmacs::chart::Sera::find_duplicates

// ----------------------------------------------------------------------

void acmacs::chart::Sera::filter_country(Indexes& aIndexes, std::string_view aCountry) const
{
    remove(aIndexes, [aCountry](const auto& entry) { return not_in_country(entry.name(), aCountry); });

} // acmacs::chart::Sera::filter_country

// ----------------------------------------------------------------------

void acmacs::chart::Sera::filter_continent(Indexes& aIndexes, std::string_view aContinent) const
{
    remove(aIndexes, [aContinent](const auto& entry) { return not_in_continent(entry.name(), aContinent); });

} // acmacs::chart::Sera::filter_continent

// ----------------------------------------------------------------------

acmacs::PointStylesCompacted acmacs::chart::PlotSpec::compacted() const
{
    acmacs::PointStylesCompacted result;
    for (const auto& style: all_styles()) {
        if (auto found = std::find(result.styles.begin(), result.styles.end(), style); found == result.styles.end()) {
            result.styles.push_back(style);
            result.index.push_back(result.styles.size() - 1);
        }
        else {
            result.index.push_back(static_cast<size_t>(found - result.styles.begin()));
        }
    }
    return result;

} // acmacs::chart::PlotSpec::compacted

// ======================================================================

template <typename AgSr> acmacs::chart::Indexes find_by_name(const AgSr& ag_sr, std::string_view aName)
{
    auto find = [&ag_sr](auto name) -> acmacs::chart::Indexes {
        acmacs::chart::Indexes indexes;
        for (auto iter = ag_sr.begin(); iter != ag_sr.end(); ++iter) {
            if ((*iter)->name() == acmacs::virus::name_t{name})
                indexes.insert(iter.index());
        }
        return indexes;
    };

    acmacs::chart::Indexes indexes = find(aName);
    if (indexes->empty() && aName.size() > 2) {
        if (const auto first_name = (*ag_sr.begin())->name(); first_name.size() > 2) {
        // handle names with "A/" instead of "A(HxNx)/" or without subtype prefix (for A and B)
            if ((aName[0] == 'A' && aName[1] == '/' && first_name[0] == 'A' && first_name[1] == '(' && first_name.find(")/") != std::string::npos) || (aName[0] == 'B' && aName[1] == '/'))
                indexes = find(acmacs::string::concat(first_name->substr(0, first_name.find('/')), aName.substr(1)));
            else if (aName[1] != '/' && aName[1] != '(')
                indexes = find(acmacs::string::concat(first_name->substr(0, first_name.find('/') + 1), aName));
        }
    }
    return indexes;
}

// ----------------------------------------------------------------------

template <typename AgSr> acmacs::chart::Indexes find_by_name(const AgSr& ag_sr, const std::regex& aName)
{
    acmacs::chart::Indexes indexes;
    for (auto iter = ag_sr.begin(); iter != ag_sr.end(); ++iter) {
        if (std::regex_search((*iter)->full_name(), aName))
            indexes.insert(iter.index());
    }
    return indexes;
}

// ----------------------------------------------------------------------

template <typename AgSr> bool name_matches(const AgSr& ag_sr, size_t index, std::string_view aName)
{
    if (!aName.empty() && aName[0] == '~') {
        const std::regex re{std::next(std::begin(aName), 1), std::end(aName), acmacs::regex::icase};
        return std::regex_search(ag_sr.at(index)->full_name(), re);
    }
    else {
        const auto name = ag_sr.at(index)->name();
        if (name == acmacs::virus::name_t{aName})
            return true;
        else if (aName.size() > 2 && name.size() > 2) {
            if ((aName[0] == 'A' && aName[1] == '/' && name[0] == 'A' && name[1] == '(' && name.find(")/") != std::string::npos) || (aName[0] == 'B' && aName[1] == '/'))
                return name == acmacs::virus::name_t{acmacs::string::concat(name->substr(0, name.find('/')), aName.substr(1))};
            else if (aName[1] != '/' && aName[1] != '(')
                return name == acmacs::virus::name_t{acmacs::string::concat(name->substr(0, name.find('/') + 1), aName)};
        }
        else
            return false;
    }
    return false;               // bug in clang-10?
}

// ----------------------------------------------------------------------

bool acmacs::chart::Antigens::name_matches(size_t index, std::string_view aName) const
{
    return ::name_matches(*this, index, aName);

} // acmacs::chart::Antigens::name_matches

// ----------------------------------------------------------------------

bool acmacs::chart::Sera::name_matches(size_t index, std::string_view aName) const
{
    return ::name_matches(*this, index, aName);

} // acmacs::chart::Sera::name_matches

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Antigens::find_by_name(std::string_view aName) const
{
    if (!aName.empty() && aName[0] == '~')
        return ::find_by_name(*this, std::regex{std::next(std::begin(aName), 1), std::end(aName), acmacs::regex::icase});
    else
        return ::find_by_name(*this, ::string::upper(aName));

} // acmacs::chart::Antigens::find_by_name

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Sera::find_by_name(std::string_view aName) const
{
    if (!aName.empty() && aName[0] == '~')
        return ::find_by_name(*this, std::regex{std::next(std::begin(aName), 1), std::end(aName), acmacs::regex::icase});
    else
        return ::find_by_name(*this, ::string::upper(aName));

} // acmacs::chart::Sera::find_by_name

// ======================================================================

template <typename AgSr> std::optional<size_t> find_by_full_name(const AgSr& ag_sr, std::string_view aFullName)
{
    const auto found = std::find_if(ag_sr.begin(), ag_sr.end(), [aFullName](auto antigen) -> bool { return antigen->full_name() == aFullName; });
    if (found == ag_sr.end())
        return std::nullopt;
    else
        return found.index();
}

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::Antigens::find_by_full_name(std::string_view aFullName) const
{
    return ::find_by_full_name(*this, aFullName);

} // acmacs::chart::Antigens::find_by_full_name

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::Sera::find_by_full_name(std::string_view aFullName) const
{
    return ::find_by_full_name(*this, aFullName);

} // acmacs::chart::Sera::find_by_full_name

// ======================================================================

acmacs::chart::Indexes acmacs::chart::Antigens::find_by_name(const std::regex& aName) const
{
    return ::find_by_name(*this, aName);

} // acmacs::chart::Antigens::find_by_name

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Sera::find_by_name(const std::regex& aName) const
{
    return ::find_by_name(*this, aName);

} // acmacs::chart::Sera::find_by_name

// ----------------------------------------------------------------------

acmacs::chart::TableDate acmacs::chart::table_date_from_sources(std::vector<std::string>&& sources)
{
    std::sort(std::begin(sources), std::end(sources));
    std::string front{sources.front()}, back{sources.back()};
    const std::string_view tables_separator{"+"};
    if (const auto fparts = acmacs::string::split(front, tables_separator, acmacs::string::Split::RemoveEmpty); fparts.size() > 1)
        front = fparts.front();
    if (const auto bparts = acmacs::string::split(back, tables_separator, acmacs::string::Split::RemoveEmpty); bparts.size() > 1)
        back = bparts.back();
    return TableDate{fmt::format("{}-{}", front, back)};

} // acmacs::chart::table_date_from_sources

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
