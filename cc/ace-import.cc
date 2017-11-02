#include <set>
#include <vector>

#include "acmacs-base/stream.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart/ace-import.hh"

using namespace acmacs::chart;

// ----------------------------------------------------------------------

std::shared_ptr<Chart> acmacs::chart::ace_import(const std::string_view& aData, bool aVerify)
{
    auto chart = std::make_shared<AceChart>(rjson::parse_string(aData));
    if (aVerify)
        chart->verify_data();
    return chart;

} // acmacs::chart::ace_import

// ----------------------------------------------------------------------

void AceChart::verify_data() const
{
    std::cerr << "WARNING: AceChart::verify_data not implemented\n";

} // AceChart::verify_data

// ----------------------------------------------------------------------

std::shared_ptr<Info> AceChart::info() const
{
    return std::make_shared<AceInfo>(mData["c"]["i"]);

} // AceChart::info

// ----------------------------------------------------------------------

std::shared_ptr<Antigens> AceChart::antigens() const
{
    return std::make_shared<AceAntigens>(mData["c"].get_or_empty_array("a"));

} // AceChart::antigens

// ----------------------------------------------------------------------

std::shared_ptr<Sera> AceChart::sera() const
{
    return std::make_shared<AceSera>(mData["c"].get_or_empty_array("s"));

} // AceChart::sera

// ----------------------------------------------------------------------

std::shared_ptr<Projections> AceChart::projections() const
{
    return std::make_shared<AceProjections>(mData["c"].get_or_empty_array("P"));

} // AceChart::projections

// ----------------------------------------------------------------------

std::string AceInfo::make_info() const
{
    const auto n_sources = number_of_sources();
    return string::join(" ", {name(),
                    virus(),
                    lab(),
                    virus_type(),
                    subset(),
                    assay(),
                    rbc_species(),
                    date(),
                    n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{}
                             });

} // AceInfo::make_info

// ----------------------------------------------------------------------

std::string AceInfo::make_field(const char* aField, const char* aSeparator) const
{
    std::string result{mData.get_or_default(aField, "")};
    if (result.empty()) {
        const auto& sources{mData.get_or_empty_array("S")};
        if (!sources.empty()) {
            std::set<std::string> composition;
            std::transform(std::begin(sources), std::end(sources), std::inserter(composition, composition.begin()), [aField](const auto& sinfo) { return sinfo.get_or_default(aField, ""); });
            result = string::join(aSeparator, composition);
        }
    }
    return result;

} // AceInfo::make_field

// ----------------------------------------------------------------------

std::string AceInfo::date() const
{
    std::string result{mData.get_or_default("D", "")};
    if (result.empty()) {
        const auto& sources{mData.get_or_empty_array("S")};
        if (!sources.empty()) {
            std::vector<std::string> composition{sources.size()};
            std::transform(std::begin(sources), std::end(sources), std::begin(composition), [](const auto& sinfo) { return sinfo.get_or_default("D", ""); });
            std::sort(std::begin(composition), std::end(composition));
            result = string::join("-", {composition.front(), composition.back()});
        }
    }
    return result;

} // AceInfo::date

// ----------------------------------------------------------------------

static inline BLineage b_lineage(std::string aLin)
{
    if (!aLin.empty()) {
        switch (aLin[0]) {
          case 'Y':
              return BLineage::Yamagata;
          case 'V':
              return BLineage::Victoria;
        }
    }
    return BLineage::Unknown;
}

BLineage AceAntigen::lineage() const
{
    return b_lineage(mData["L"]);

} // AceAntigen::lineage

BLineage AceSerum::lineage() const
{
    return b_lineage(mData["L"]);

} // AceSerum::lineage

// ----------------------------------------------------------------------

size_t AceProjection::number_of_dimensions() const
{
    try {
        for (const rjson::array& row: static_cast<const rjson::array&>(mData["l"])) {
            if (!row.empty())
                return row.size();
        }
    }
    catch (rjson::field_not_found&) {
    }
    catch (std::bad_variant_access&) {
    }
    return 0;

} // AceProjection::number_of_dimensions

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
