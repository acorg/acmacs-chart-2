#include <regex>

#include "acmacs-base/string.hh"
#include "acmacs-base/virus-name.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/range.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
static std::regex sDate{"[12][90][0-9][0-9]-[0-2][0-9]-[0-3][0-9]"};
#include "acmacs-base/diagnostics-pop.hh"

void acmacs::chart::Date::check() const
{
    if (!empty() && !std::regex_match(*this, sDate))
        throw invalid_data{"invalid date (YYYY-MM-DD expected): " + *this};

} // acmacs::chart::Date::check

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_info() const
{
    return string::join("\n", {info()->make_info(),
                    "Antigens:" + std::to_string(number_of_antigens()) + " Sera:" + std::to_string(number_of_sera()),
                    projections()->make_info()
                    });

} // acmacs::chart::Chart::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_name(std::optional<size_t> aProjectionNo) const
{
    std::string n = info()->make_name();
    if (auto prjs = projections(); !prjs->empty() && (!aProjectionNo || *aProjectionNo < prjs->size())) {
        auto prj = (*prjs)[aProjectionNo ? *aProjectionNo : 0];
        n += " >=" + static_cast<std::string>(prj->minimum_column_basis()) + " " + std::to_string(prj->stress());
    }
    return n;

} // acmacs::chart::Chart::make_name

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

std::string acmacs::chart::Chart::lineage() const
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

class TiterDistance
{
 public:
    inline TiterDistance(acmacs::chart::Titer aTiter, double aColumnBase, double aDistance)
        : titer(aTiter), similarity(aTiter.is_dont_care() ? 0.0 : aTiter.logged_for_column_bases()),
          final_similarity(std::min(aColumnBase, similarity)), distance(aDistance) {}
    inline TiterDistance() : similarity(0), final_similarity(0), distance(std::numeric_limits<double>::quiet_NaN()) {}
    inline operator bool() const { return !titer.is_dont_care(); }

    acmacs::chart::Titer titer;
    double similarity;
    double final_similarity;
    double distance;
};

inline std::ostream& operator << (std::ostream& out, const TiterDistance& td)
{
    if (td)
        return out << "t:" << td.titer << " s:" << td.similarity << " f:" << td.final_similarity << " d:" << td.distance << std::endl;
    else
        return out << "dont-care" << std::endl;
}

class SerumCircleRadiusCalculationError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

// Description of empirical radius calculation found in my message to Derek 2015-09-21 12:03 Subject: Serum protection radius
//
// Program "draws" some circle around a serum with some radius. Then for
// each antigen having titer with that serum program calculates:
// 1. Theoretical protection, i.e. if titer for antigen and serum is more
// or equal than (homologous-titer - 2)
// 2. Empirical protection, i.e. if antigen is inside the drawn circle,
// i.e. if distance between antigen and serum is less or equal than the
// circle radius.
//
// As the result for a circle we have four numbers:
// 1. Number of antigens both theoretically and empirically protected;
// 2. Number of antigens just theoretically protected;
// 3. Number of antigens just empirically protected;
// 4. Number of antigens not protected at all.
//
// Then the program optimizes the circle radius to minimize 2 and 3,
// i.e. the sum of number of antigens protected only theoretically and
// only empirically.
//
// Practically program first calculates stress for the radius equal to
// the distance of the closest antigen. Then it takes the radius as
// average between closest antigen distance and the second closest
// antigen distance and gets stress. Then it takes the radius as
// average between the second closest antigen distance and the third
// closest antigen and gets stress. And so on, the stress increases
// with each antigen included into the circle.
//
// If there are multiple optima with equal sums of 2 and 3, then the
// radius is a mean of optimal radii.

double acmacs::chart::Chart::serum_circle_radius_empirical(size_t aAntigenNo, size_t aSerumNo, size_t aProjectionNo, bool aVerbose) const
{
    if (aVerbose)
        std::cerr << "DEBUG: serum_circle_radius_empirical for [sr:" << aSerumNo << ' ' << serum(aSerumNo)->full_name() << "] [ag:" << aAntigenNo << ' ' << antigen(aAntigenNo)->full_name() << ']' << std::endl;
    try {
        auto tts = titers();
        if (const auto homologous_titer = tts->titer(aAntigenNo, aSerumNo); !homologous_titer.is_regular())
            throw SerumCircleRadiusCalculationError("cannot handle non-regular homologous titer: " + homologous_titer);
        auto prj = projection(aProjectionNo);
        auto layout = prj->layout();
        double cb;
        if (auto forced = prj->forced_column_bases(); forced)
            cb = forced->column_basis(aSerumNo);
        else
            cb = computed_column_bases(prj->minimum_column_basis(), use_cache::yes)->column_basis(aSerumNo);
        std::vector<TiterDistance> titers_and_distances(number_of_antigens());
        size_t max_titer_for_serum_ag_no = 0;
        for (size_t ag_no = 0; ag_no < number_of_antigens(); ++ag_no) {
            const auto titer = tts->titer(ag_no, aSerumNo);
            if (!titer.is_dont_care()) {
                  // TODO: antigensSeraTitersMultipliers (acmacs/plot/serum_circle.py:113)
                titers_and_distances[ag_no] = TiterDistance(titer, cb, layout->distance(ag_no, aSerumNo + number_of_antigens()));
                if (max_titer_for_serum_ag_no != ag_no && titers_and_distances[max_titer_for_serum_ag_no].final_similarity < titers_and_distances[ag_no].final_similarity)
                    max_titer_for_serum_ag_no = ag_no;
            }
            // else if (ag_no == aAntigenNo)
            //     throw SerumCircleRadiusCalculationError("no homologous titer");
        }
        const double protection_boundary_titer = titers_and_distances[aAntigenNo].final_similarity - 2.0;
        if (protection_boundary_titer < 1.0)
            throw SerumCircleRadiusCalculationError("titer is too low, protects everything");
        // if (aVerbose) std::cerr << "DEBUG: titers_and_distances: " << titers_and_distances << std::endl;
        if (aVerbose) std::cerr << "DEBUG: serum_circle_radius_empirical protection_boundary_titer: " << protection_boundary_titer << std::endl;

          // sort antigen indices by antigen distance from serum, closest first
        auto antigens_by_distances_sorting = [&titers_and_distances](size_t a, size_t b) -> bool {
            const auto& aa = titers_and_distances[a];
            if (aa) {
                const auto& bb = titers_and_distances[b];
                return bb ? aa.distance < bb.distance : true;
            }
            else
                return false;
        };
        Indexes antigens_by_distances(acmacs::index_iterator(0UL), acmacs::index_iterator(number_of_antigens()));
        std::sort(antigens_by_distances.begin(), antigens_by_distances.end(), antigens_by_distances_sorting);
          //if (aVerbose) std::cerr << "DEBUG: antigens_by_distances " << antigens_by_distances << std::endl;

        constexpr const size_t None = static_cast<size_t>(-1);
        size_t best_sum = None;
        size_t previous = None;
        double sum_radii = 0;
        size_t num_radii = 0;
        for (size_t ag_no: antigens_by_distances) {
            if (!titers_and_distances[ag_no])
                break;
            if (!std::isnan(titers_and_distances[ag_no].distance)) {
                const double radius = previous == None ? titers_and_distances[ag_no].distance : (titers_and_distances[ag_no].distance + titers_and_distances[previous].distance) / 2.0;
                size_t protected_outside = 0, not_protected_inside = 0; // , protected_inside = 0, not_protected_outside = 0;
                for (const auto& protection_data: titers_and_distances) {
                    if (protection_data) {
                        const bool inside = protection_data.distance <= radius;
                        const bool protectd = protection_data.titer.is_regular() ? protection_data.final_similarity >= protection_boundary_titer : protection_data.final_similarity > protection_boundary_titer;
                        if (protectd && !inside)
                            ++protected_outside;
                        else if (!protectd && inside)
                            ++not_protected_inside;
                    }
                }
                const size_t summa = protected_outside + not_protected_inside;
                if (best_sum == None || best_sum >= summa) { // if sums are the same, choose the smaller radius (found earlier)
                    if (best_sum == summa) {
                        if (aVerbose)
                            std::cerr << "DEBUG: AG " << ag_no << " radius:" << radius << " distance:" << titers_and_distances[ag_no].distance << " prev:" << static_cast<int>(previous) << " protected_outside:" << protected_outside << " not_protected_inside:" << not_protected_inside << " best_sum:" << best_sum << std::endl;
                        sum_radii += radius;
                        ++num_radii;
                    }
                    else {
                        if (aVerbose)
                            std::cerr << "======================================================================" << std::endl
                                      << "DEBUG: AG " << ag_no << " radius:" << radius << " distance:" << titers_and_distances[ag_no].distance << " prev:" << static_cast<int>(previous) << " protected_outside:" << protected_outside << " not_protected_inside:" << not_protected_inside << " best_sum:" << best_sum << std::endl;
                        sum_radii = radius;
                        num_radii = 1;
                        best_sum = summa;
                    }
                }
                  // std::cerr << "AG " << ag_no << " radius:" << radius << " protected_outside:" << protected_outside << " not_protected_inside:" << not_protected_inside << " best_sum:" << best_sum << std::endl;
                previous = ag_no;
            }
        }
        return sum_radii / num_radii;
    }
    catch (SerumCircleRadiusCalculationError& err) {
        std::cerr << "WARNING: " << "Cannot calculate serum projection radius for sr " << aSerumNo << " ag " << aAntigenNo << ": " << err.what() << std::endl;
        return -1;
    }

} // acmacs::chart::Chart::serum_circle_radius_empirical

// ----------------------------------------------------------------------

// Low reactors are defined as >4-fold from the homologous titer,
// hence the theoretical radius is 2 units plus the number of 2-folds
// between max titer and the homologous titer for a serum. Saying the
// same thing mathematically the theoretical radius for a serum circle
// is 2 + log2(max titer for serum S against any antigen A) - log2(homologous titer for serum S).

double acmacs::chart::Chart::serum_circle_radius_theoretical(size_t aAntigenNo, size_t aSerumNo, size_t aProjectionNo, bool /*aVerbose*/) const
{
    try {
        const auto homologous_titer = titers()->titer(aAntigenNo, aSerumNo);
        if (!homologous_titer.is_regular())
            throw SerumCircleRadiusCalculationError("cannot handle non-regular homologous titer: " + homologous_titer);
        double cb;
        auto prj = projection(aProjectionNo);
        if (auto forced = prj->forced_column_bases(); forced)
            cb = forced->column_basis(aSerumNo);
        else
            cb = computed_column_bases(prj->minimum_column_basis(), use_cache::yes)->column_basis(aSerumNo);
        return 2.0 + cb - homologous_titer.logged_for_column_bases();
    }
    catch (SerumCircleRadiusCalculationError& err) {
        std::cerr << "WARNING: " << "Cannot calculate serum projection radius for sr " << aSerumNo << " ag " << aAntigenNo << ": " << err.what() << std::endl;
        return -1;
    }

} // acmacs::chart::Chart::serum_circle_radius_theoretical

// ----------------------------------------------------------------------

void acmacs::chart::Chart::serum_coverage(size_t aAntigenNo, size_t aSerumNo, Indexes& aWithin4Fold, Indexes& aOutside4Fold) const
{
    auto tts = titers();
    const auto homologous_titer = tts->titer(aAntigenNo, aSerumNo);
    if (!homologous_titer.is_regular())
        throw serum_coverage_error("cannot handle non-regular homologous titer: " + homologous_titer);
    const double titer_threshold = homologous_titer.logged() - 2;
    if (titer_threshold <= 0)
        throw serum_coverage_error("homologous titer is too low: " + homologous_titer);
    for (size_t ag_no = 0; ag_no < number_of_antigens(); ++ag_no) {
        const Titer titer = tts->titer(ag_no, aSerumNo);
        const double value = titer.is_dont_care() ? -1 : titer.logged_for_column_bases();
        if (value >= titer_threshold)
            aWithin4Fold.insert(ag_no);
        else if (value >= 0 && value < titer_threshold)
            aOutside4Fold.insert(ag_no);
    }
    if (aWithin4Fold.empty())
        throw serum_coverage_error("no antigens within 4fold from homologous titer (for serum coverage)"); // BUG? at least homologous antigen must be there!

} // acmacs::chart::Chart::serum_coverage

// ----------------------------------------------------------------------

void acmacs::chart::Chart::set_homologous(find_homologous_for_big_chart force, SeraP aSera) const
{
    if (!mHomologousFound && (force == find_homologous_for_big_chart::yes || (!is_merge() && number_of_antigens() < 200))) {
        // Timeit ti("set homologous for sera: ");
        if (!aSera)
            aSera = sera();
        aSera->set_homologous(*antigens());
        mHomologousFound = true;
    }

} // acmacs::chart::Chart::set_homologous

// ----------------------------------------------------------------------

acmacs::PointStyle acmacs::chart::Chart::default_style(acmacs::chart::Chart::PointType aPointType) const
{
    acmacs::PointStyle style;
    style.outline = BLACK;
    switch (aPointType) {
      case PointType::TestAntigen:
          style.shape = acmacs::PointShape::Circle;
          style.size = Pixels{5.0};
          style.fill = GREEN;
          break;
      case PointType::ReferenceAntigen:
          style.shape = acmacs::PointShape::Circle;
          style.size = Pixels{8.0};
          style.fill = TRANSPARENT;
          break;
      case PointType::Serum:
          style.shape = acmacs::PointShape::Box;
          style.size = Pixels{6.5};
          style.fill = TRANSPARENT;
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

acmacs::chart::BLineage::Lineage acmacs::chart::BLineage::from(std::string aSource)
{
    if (!aSource.empty()) {
        switch (aSource[0]) {
          case 'Y':
              return Yamagata;
          case 'V':
              return Victoria;
        }
    }
    return Unknown;

} // acmacs::chart::BLineage::from

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_info() const
{
    const auto n_sources = number_of_sources();
    return string::join(" ", {name(),
                    virus(Compute::Yes),
                    lab(Compute::Yes),
                    virus_type(Compute::Yes),
                    subset(Compute::Yes),
                    assay(Compute::Yes),
                    rbc_species(Compute::Yes),
                    date(Compute::Yes),
                    n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{}
                             });

} // acmacs::chart::Info::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_name() const
{
    std::string n = name(Compute::No);
    if (n.empty())
        n = string::join({lab(Compute::Yes), virus_not_influenza(Compute::Yes), virus_type(Compute::Yes), subset(Compute::Yes), assay(Compute::Yes), rbc_species(Compute::Yes), date(Compute::Yes)});
    return n;

} // acmacs::chart::Info::make_name

// ----------------------------------------------------------------------

std::string acmacs::chart::Projection::make_info() const
{
    auto lt = layout();
    std::string result = std::to_string(stress()) + " " + std::to_string(lt->number_of_dimensions()) + 'd';
    if (auto cmt = comment(); !cmt.empty())
        result += " <" + cmt + '>';
    if (auto fcb = forced_column_bases(); fcb)
        result += " forced-column-bases"; // + acmacs::to_string(fcb);
    else
        result += " >=" + static_cast<std::string>(minimum_column_basis());
    return result;

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

std::string acmacs::chart::Projections::make_info() const
{
    std::string result = "Projections: " + std::to_string(size());
    for (auto projection_no: acmacs::range(0UL, std::min(20UL, size())))
        result += "\n  " + std::to_string(projection_no) + ' ' + operator[](projection_no)->make_info();
    return result;

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
    std::string r = name();
    if (const auto value = reassortant(); !value.empty())
        r += " reassortant=\"" + value + '"';
    if (const auto value = ::string::join(" ", annotations()); !value.empty())
        r += " annotations=\"" + value + '"';
    if (const auto value = passage(); !value.empty())
        r += " passage=\"" + value + "\" ptype=" + value.passage_type();
    if (const auto value = date(); !value.empty())
        r += " date=" + value;
    if (const auto value = lineage(); value != BLineage::Unknown)
        r += " lineage=" + static_cast<std::string>(value);
    if (reference())
        r += " reference";
    if (const auto value = ::string::join(" ", lab_ids()); !value.empty())
        r += " lab_ids=\"" + value + '"';
    return r;

} // acmacs::chart::Antigen::full_name_with_fields

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::full_name_with_fields() const
{
    std::string r = name();
    if (const auto value = reassortant(); !value.empty())
        r += " reassortant=\"" + value + '"';
    if (const auto value = ::string::join(" ", annotations()); !value.empty())
        r += " annotations=\"" + value + '"';
    if (const auto value = serum_id(); !value.empty())
        r += " serum_id=\"" + value + '"';
    if (const auto value = passage(); !value.empty())
        r += " passage=\"" + value + "\" ptype=" + value.passage_type();
    if (const auto value = serum_species(); !value.empty())
        r += " serum_species=\"" + value + '"';
    if (const auto value = lineage(); value != BLineage::Unknown)
        r += " lineage=" + static_cast<std::string>(value);
    return r;

} // acmacs::chart::Serum::full_name_with_fields

// ----------------------------------------------------------------------

std::string name_abbreviated(std::string aName);
std::string name_abbreviated(std::string aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage, extra;
        virus_name::split_with_extra(aName, virus_type, host, location, isolation, year, passage, extra);
        return string::join("/", {get_locdb().abbreviation(location), isolation, year.substr(2)});
    }
    catch (virus_name::Unrecognized&) {
        return aName;
    }

} // name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::name_abbreviated() const
{
    return ::name_abbreviated(name());

} // acmacs::chart::Antigen::name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Antigen::location_abbreviated() const
{
    return get_locdb().abbreviation(virus_name::location(name()));

} // acmacs::chart::Antigen::location_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::name_abbreviated() const
{
    return ::name_abbreviated(name());

} // acmacs::chart::Serum::name_abbreviated

// ----------------------------------------------------------------------

std::string acmacs::chart::Serum::location_abbreviated() const
{
    return get_locdb().abbreviation(virus_name::location(name()));

} // acmacs::chart::Serum::location_abbreviated

// ----------------------------------------------------------------------

static inline bool not_in_country(std::string aName, std::string aCountry)
{
    try {
        return get_locdb().country(virus_name::location(aName)) != aCountry;
    }
    catch (virus_name::Unrecognized&) {
    }
    catch (LocationNotFound&) {
    }
    return true;

} // AntigensSera<AgSr>::filter_country

// ----------------------------------------------------------------------

static inline bool not_in_continent(std::string aName, std::string aContinent)
{
    try {
        return get_locdb().continent(virus_name::location(aName)) != aContinent;
    }
    catch (virus_name::Unrecognized&) {
    }
    catch (LocationNotFound&) {
    }
    return true;

} // AntigensSera<AgSr>::filter_continent

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::Antigens::find_by_full_name(std::string aFullName) const
{
    const auto found = std::find_if(begin(), end(), [aFullName](auto antigen) -> bool { return antigen->full_name() == aFullName; });
    if (found == end())
        return {};
    else
        return found.index();

} // acmacs::chart::Antigens::find_by_full_name

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Antigens::find_by_name(std::string aName) const
{
    Indexes indexes;
    for (auto iter = begin(); iter != end(); ++iter) {
        if ((*iter)->name() == aName)
            indexes.insert(iter.index());
    }
    return indexes;

} // acmacs::chart::Antigens::find_by_name

// ----------------------------------------------------------------------

void acmacs::chart::Antigens::filter_country(Indexes& aIndexes, std::string aCountry) const
{
    remove(aIndexes, [aCountry](const auto& entry) { return not_in_country(entry.name(), aCountry); });

} // acmacs::chart::Antigens::filter_country

// ----------------------------------------------------------------------

void acmacs::chart::Antigens::filter_continent(Indexes& aIndexes, std::string aContinent) const
{
    remove(aIndexes, [aContinent](const auto& entry) { return not_in_continent(entry.name(), aContinent); });

} // acmacs::chart::Antigens::filter_continent

// ----------------------------------------------------------------------

void acmacs::chart::Sera::set_homologous(const Antigens& aAntigens)
{
    std::map<std::string, std::vector<size_t>> antigen_name_index;
    for (auto [ag_no, antigen]: acmacs::enumerate(aAntigens))
        antigen_name_index.emplace(antigen->name(), std::vector<size_t>{}).first->second.push_back(ag_no);

      // std::cerr << "DEBUG: set_homologous " << size() << '\n';
    for (auto serum: *this) {
        if (serum->homologous_antigens().empty()) {
            std::vector<size_t> homologous;
              // std::cerr << "DEBUG: " << serum->full_name() << ' ' << serum->passage() << '\n';
            if (auto ags = antigen_name_index.find(serum->name()); ags != antigen_name_index.end()) {
                for (auto ag_no: ags->second) {
                    auto antigen = aAntigens[ag_no];
                    if (antigen->reassortant() == serum->reassortant()) {
                        if (!serum->passage().empty()) {
                            if (antigen->passage().is_egg() == serum->passage().is_egg()) {
                                homologous.push_back(ag_no);
                                  // std::cerr << "       " << antigen->full_name() << '\n';
                            }
                        }
                        else {      // niid has passage type data in serum_id
                            const bool egg = serum->serum_id().find("EGG") != std::string::npos;
                            if (antigen->passage().is_egg() == egg) {
                                homologous.push_back(ag_no);
                                  // std::cerr << "       " << antigen->full_name() << '\n';
                            }
                        }
                    }
                }
                  // std::cerr << "DEBUG: " << serum->full_name() << ' ' << serum->passage() << ' ' << homologous << ' ' << ags->second.size() << '\n';
            }
              // std::cerr << "DEBUG: " << serum->full_name() << ' ' << serum->passage() << ' ' << homologous << '\n';
            serum->set_homologous(homologous);
        }
    }

} // acmacs::chart::Sera::set_homologous

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::Sera::find_by_full_name(std::string aFullName) const
{
    const auto found = std::find_if(begin(), end(), [aFullName](auto serum) -> bool { return serum->full_name() == aFullName; });
    if (found == end())
        return {};
    else
        return found.index();

} // acmacs::chart::Sera::find_by_full_name

// ----------------------------------------------------------------------

acmacs::chart::Indexes acmacs::chart::Sera::find_by_name(std::string aName) const
{
    Indexes indexes;
    for (auto iter = begin(); iter != end(); ++iter) {
        if ((*iter)->name() == aName)
            indexes.insert(iter.index());
    }
    return indexes;

} // acmacs::chart::Sera::find_by_name

// ----------------------------------------------------------------------

void acmacs::chart::Sera::filter_country(Indexes& aIndexes, std::string aCountry) const
{
    remove(aIndexes, [aCountry](const auto& entry) { return not_in_country(entry.name(), aCountry); });

} // acmacs::chart::Sera::filter_country

// ----------------------------------------------------------------------

void acmacs::chart::Sera::filter_continent(Indexes& aIndexes, std::string aContinent) const
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

// ----------------------------------------------------------------------

// acmacs::chart::Chart::~Chart()
// {
// } // acmacs::chart::Chart::~Chart

// // ----------------------------------------------------------------------

// acmacs::chart::Info::~Info()
// {
// } // acmacs::chart::Info::~Info

// // ----------------------------------------------------------------------

// acmacs::chart::Antigen::~Antigen()
// {
// } // acmacs::chart::Antigen::~Antigen

// // ----------------------------------------------------------------------

// acmacs::chart::Serum::~Serum()
// {
// } // acmacs::chart::Serum::~Serum

// // ----------------------------------------------------------------------

// acmacs::chart::Antigens::~Antigens()
// {
// } // acmacs::chart::Antigens::~Antigens

// // ----------------------------------------------------------------------

// acmacs::chart::Sera::~Sera()
// {
// } // acmacs::chart::Sera::~Sera

// // ----------------------------------------------------------------------

// acmacs::chart::ColumnBases::~ColumnBases()
// {
// } // acmacs::chart::ColumnBases::~ColumnBases

// // ----------------------------------------------------------------------

// acmacs::chart::Projection::~Projection()
// {
// } // acmacs::chart::Projection::~Projection

// // ----------------------------------------------------------------------

// acmacs::chart::Projections::~Projections()
// {
// } // acmacs::chart::Projections::~Projections

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
