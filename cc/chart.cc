#include "acmacs-base/string.hh"
#include "acmacs-base/virus-name.hh"
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
          return *lineages.begin() == BLineage::Victoria ? "VICTORIA" : "YAMAGATA";
      case 2:
          return "VICTORIA+YAMAGATA";
      default:
          return "VICTORIA+YAMAGATA+";
    }
    // return {};

} // acmacs::chart::Chart::lineage

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
