#include "acmacs-base/string.hh"
#include "acmacs-base/virus-name.hh"
#include "acmacs-base/enumerate.hh"
#include "locationdb/locdb.hh"
#include "acmacs-chart-2/chart.hh"

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

std::string acmacs::chart::Chart::lineage() const
{
    std::set<BLineage> lineages;
    auto ags = antigens();
    for (auto antigen: *ags) {
        if (const auto lineage = antigen->lineage(); lineage != BLineage::Unknown)
            lineages.insert(lineage);
    }
    switch (lineages.size()) {
      case 0:
          return {};
      case 1:
          return *lineages.begin();
      case 2:
          return "VICTORIA+YAMAGATA";
      default:
          return "VICTORIA+YAMAGATA+";
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

double acmacs::chart::Chart::serum_circle_radius(size_t aAntigenNo, size_t aSerumNo, size_t aProjectionNo, bool aVerbose) const
{
    if (aVerbose)
        std::cerr << "DEBUG: serum_circle_radius for [sr:" << aSerumNo << ' ' << serum(aSerumNo)->full_name() << "] [ag:" << aAntigenNo << ' ' << antigen(aAntigenNo)->full_name() << ']' << std::endl;
    try {
        auto prj = projection(aProjectionNo);
        auto layout = prj->layout();
        auto tts = titers();
        double cb;
        if (auto forced = prj->forced_column_bases(); forced->exists())
            cb = forced->column_basis(aSerumNo);
        else
            cb = computed_column_bases(prj->minimum_column_basis())->column_basis(aSerumNo);
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
            else if (ag_no == aAntigenNo)
                throw SerumCircleRadiusCalculationError("no homologous titer");
        }
        const double protection_boundary_titer = titers_and_distances[aAntigenNo].final_similarity - 2.0;
        if (protection_boundary_titer < 1.0)
            throw SerumCircleRadiusCalculationError("titer is too low, protects everything");
        // if (aVerbose) std::cerr << "DEBUG: titers_and_distances: " << titers_and_distances << std::endl;
        if (aVerbose) std::cerr << "DEBUG: serum_circle_radius protection_boundary_titer: " << protection_boundary_titer << std::endl;

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
        Indexes antigens_by_distances(acmacs::incrementer<size_t>::begin(0), acmacs::incrementer<size_t>::end(number_of_antigens()));
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

} // acmacs::chart::Chart::serum_circle_radius

// ----------------------------------------------------------------------

void acmacs::chart::Chart::serum_coverage(size_t aAntigenNo, size_t aSerumNo, Indexes& aWithin4Fold, Indexes& aOutside4Fold) const
{
    auto tts = titers();
    const Titer homologous_titer = tts->titer(aAntigenNo, aSerumNo);
    if (!homologous_titer.is_regular())
        throw std::runtime_error("serum_coverage: cannot handle non-regular homologous titer: " + homologous_titer.data());
    const double titer_threshold = homologous_titer.logged() - 2;
    if (titer_threshold <= 0)
        throw std::runtime_error("serum_coverage: homologous titer is too low: " + homologous_titer.data());
    for (size_t ag_no = 0; ag_no < number_of_antigens(); ++ag_no) {
        const Titer titer = tts->titer(ag_no, aSerumNo);
        const double value = titer.is_dont_care() ? -1 : titer.logged_for_column_bases();
        if (value >= titer_threshold)
            aWithin4Fold.push_back(ag_no);
        else if (value >= 0 && value < titer_threshold)
            aOutside4Fold.push_back(ag_no);
    }
    if (aWithin4Fold.empty())
        throw std::runtime_error("serum_coverage: no antigens within 4fold from homologous titer (for serum coverage)"); // BUG? at least homologous antigen must be there!

} // acmacs::chart::Chart::serum_coverage

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
    std::string n = name();
    if (n.empty())
        n = string::join({lab(Compute::Yes), virus(Compute::Yes), virus_type(Compute::Yes), subset(Compute::Yes), assay(Compute::Yes), rbc_species(Compute::Yes), date(Compute::Yes)});
    return n;

} // acmacs::chart::Info::make_name

// ----------------------------------------------------------------------

double acmacs::chart::Titer::logged() const
{
    constexpr auto log_titer = [](std::string source) -> double { return std::log2(std::stod(source) / 10.0); };

    switch (type()) {
      case Invalid:
          throw invalid_titer(data());
      case Regular:
          return log_titer(data());
      case DontCare:
          throw invalid_titer(data());
      case LessTnan:
      case MoreThan:
      case Dodgy:
          return log_titer(data().substr(1));
    }
    throw invalid_titer(data()); // for gcc 7.2

} // acmacs::chart::Titer::logged

// ----------------------------------------------------------------------

std::string acmacs::chart::Titer::logged_as_string() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(data());
      case Regular:
          return acmacs::to_string(logged());
      case DontCare:
          return data();
      case LessTnan:
      case MoreThan:
      case Dodgy:
          return data()[0] + acmacs::to_string(logged());
    }
    throw invalid_titer(data()); // for gcc 7.2

} // acmacs::chart::Titer::logged_as_string

// ----------------------------------------------------------------------

double acmacs::chart::Titer::logged_for_column_bases() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(data());
      case Regular:
      case LessTnan:
          return logged();
      case MoreThan:
          return logged() + 1;
      case DontCare:
      case Dodgy:
          return -1;
    }
    throw invalid_titer(data()); // for gcc 7.2

} // acmacs::chart::Titer::logged_for_column_bases

// ----------------------------------------------------------------------

class ComputedColumnBases : public acmacs::chart::ColumnBases
{
 public:
    inline ComputedColumnBases(size_t aNumberOfSera) : mData(aNumberOfSera, 0) {}

    inline bool exists() const override { return true; }
    inline double column_basis(size_t aSerumNo) const override { return mData.at(aSerumNo); }
    inline size_t size() const override { return mData.size(); }

    inline void update(size_t aSerumNo, double aValue) { if (aValue > mData[aSerumNo]) mData[aSerumNo] = aValue; }

 private:
    std::vector<double> mData;

}; // class ComputedColumnBases

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Chart::computed_column_bases(MinimumColumnBasis aMinimumColumnBasis) const
{
    auto cb = std::make_shared<ComputedColumnBases>(number_of_sera());
    auto tts = titers();
    for (size_t ag_no = 0; ag_no < number_of_antigens(); ++ag_no)
        for (size_t sr_no = 0; sr_no < number_of_sera(); ++sr_no)
            cb->update(sr_no, tts->titer(ag_no, sr_no).logged_for_column_bases());
    for (size_t sr_no = 0; sr_no < number_of_sera(); ++sr_no)
        cb->update(sr_no, aMinimumColumnBasis);
    return cb;

} // acmacs::chart::Chart::computed_column_bases

// ----------------------------------------------------------------------

std::string acmacs::chart::Projection::make_info() const
{
    auto lt = layout();
    return std::to_string(stress()) + " " + std::to_string(lt->number_of_dimensions()) + "d";

} // acmacs::chart::Projection::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Projections::make_info() const
{
    std::string result = "Projections: " + std::to_string(size());
    if (!empty())
        result += "\n" + operator[](0)->make_info();
    return result;

} // acmacs::chart::Projections::make_info

// ----------------------------------------------------------------------

static inline std::string name_abbreviated(std::string aName)
{
    try {
        std::string virus_type, host, location, isolation, year, passage;
        virus_name::split(aName, virus_type, host, location, isolation, year, passage);
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
            indexes.push_back(iter.index());
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
            indexes.push_back(iter.index());
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

acmacs::chart::Chart::~Chart()
{
} // acmacs::chart::Chart::~Chart

// ----------------------------------------------------------------------

acmacs::chart::Info::~Info()
{
} // acmacs::chart::Info::~Info

// ----------------------------------------------------------------------

acmacs::chart::Antigen::~Antigen()
{
} // acmacs::chart::Antigen::~Antigen

// ----------------------------------------------------------------------

acmacs::chart::Serum::~Serum()
{
} // acmacs::chart::Serum::~Serum

// ----------------------------------------------------------------------

acmacs::chart::Antigens::~Antigens()
{
} // acmacs::chart::Antigens::~Antigens

// ----------------------------------------------------------------------

acmacs::chart::Sera::~Sera()
{
} // acmacs::chart::Sera::~Sera

// ----------------------------------------------------------------------

acmacs::chart::Titers::~Titers()
{
} // acmacs::chart::Titers::~Titers

// ----------------------------------------------------------------------

acmacs::chart::ColumnBases::~ColumnBases()
{
} // acmacs::chart::ColumnBases::~ColumnBases

// ----------------------------------------------------------------------

acmacs::chart::Projection::~Projection()
{
} // acmacs::chart::Projection::~Projection

// ----------------------------------------------------------------------

acmacs::chart::Projections::~Projections()
{
} // acmacs::chart::Projections::~Projections

// ----------------------------------------------------------------------

acmacs::chart::PlotSpec::~PlotSpec()
{
} // acmacs::chart::PlotSpec::~PlotSpec

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
