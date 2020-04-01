#include <algorithm>
#include <cctype>

#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/titers.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

bool acmacs::chart::equal(const Titers& t1, const Titers& t2, bool verbose)
{
    if (t1.number_of_antigens() != t2.number_of_antigens() || t1.number_of_sera() != t2.number_of_sera() || t1.number_of_layers() != t2.number_of_layers()) {
        if (verbose)
            fmt::print(stderr, "WARNING: number of ag/sr or layes are different\n");
        return false;
    }

    auto tt1 = t1.titers_existing(), tt2 = t2.titers_existing();
    for (auto it1 = tt1.begin(), it2 = tt2.begin(); it1 != tt1.end(); ++it1, ++it2) {
        if (it1 != it2)
            return false;
    }
    if (t1.number_of_layers() > 1) {
        for (size_t layer_no = 0; layer_no < t1.number_of_layers(); ++layer_no) {
            auto tl1 = t1.titers_existing_from_layer(layer_no), tl2 = t2.titers_existing_from_layer(layer_no);
            for (auto it1 = tl1.begin(), it2 = tl2.begin(); it1 != tl1.end(); ++it1, ++it2) {
                if (it1 != it2)
                    return false;
            }
        }
    }

    return true;

} // acmacs::chart::equal

// ----------------------------------------------------------------------

std::string_view acmacs::chart::Titer::validate(std::string_view titer)
{
    if (titer.empty())
        throw invalid_titer(titer);

    const auto just_digits = [titer](auto&& data) {
        if (!std::all_of(std::begin(data), std::end(data), [](auto val) { return std::isdigit(val); }))
            throw invalid_titer(titer);
    };

    switch (titer.front()) {
      case '*':
          if (titer.size() != 1)
              throw invalid_titer(titer);
          break;
      case '<':
      case '>':
      case '~':
          just_digits(titer.substr(1));
          break;
      default:
          just_digits(titer);
          break;
    }
    return titer;

} // acmacs::chart::Titer::validate

// ----------------------------------------------------------------------

double acmacs::chart::Titer::logged_with_thresholded() const
{
    switch (type()) {
      case Invalid:
      case Regular:
      case DontCare:
      case Dodgy:
          return logged();
      case LessThan:
          return logged() - 1;
      case MoreThan:
          return logged() + 1;
    }
    throw invalid_titer(*this); // for gcc 7.2

} // acmacs::chart::Titer::logged_with_thresholded

// ----------------------------------------------------------------------

std::string acmacs::chart::Titer::logged_as_string() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
          return acmacs::to_string(logged());
      case DontCare:
          return std::string{get()};
      case LessThan:
      case MoreThan:
      case Dodgy:
          return get().front() + acmacs::to_string(logged());
    }
    throw invalid_titer(*this); // for gcc 7.2

} // acmacs::chart::Titer::logged_as_string

// ----------------------------------------------------------------------

double acmacs::chart::Titer::logged_for_column_bases() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
      case LessThan:
          return logged();
      case MoreThan:
          return logged() + 1;
      case DontCare:
      case Dodgy:
          return -1;
    }
    throw invalid_titer(*this); // for gcc 7.2

} // acmacs::chart::Titer::logged_for_column_bases

// ----------------------------------------------------------------------

size_t acmacs::chart::Titer::value_for_sorting() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return std::stoul(*this);
      case LessThan:
          return std::stoul(get().substr(1)) - 1;
      case MoreThan:
          return std::stoul(get().substr(1)) + 1;
      case Dodgy:
          return std::stoul(get().substr(1));
    }
    return 0;

} // acmacs::chart::Titer::value_for_sorting

// ----------------------------------------------------------------------

size_t acmacs::chart::Titer::value() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return std::stoul(get());
      case LessThan:
      case MoreThan:
      case Dodgy:
          return std::stoul(get().substr(1));
    }
    return 0;

} // acmacs::chart::Titer::value

// ----------------------------------------------------------------------

size_t acmacs::chart::Titer::value_with_thresholded() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return std::stoul(*this);
      case LessThan:
          return std::stoul(get().substr(1)) / 2;
      case MoreThan:
          return std::stoul(get().substr(1)) * 2;
      case Dodgy:
          return std::stoul(get().substr(1));
    }
    return 0;

} // acmacs::chart::Titer::value_with_thresholded

// ----------------------------------------------------------------------


acmacs::chart::Titer acmacs::chart::Titer::multiplied_by(double value) const // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return *this;
      case Regular:
          return Titer{std::to_string(std::lround(static_cast<double>(std::stoul(get())) * value))};
      case LessThan:
      case MoreThan:
      case Dodgy:
          return Titer{get().front() + std::to_string(std::lround(static_cast<double>(std::stoul(get().substr(1))) * value))};
    }
    return Titer{};

} // acmacs::chart::Titer::multiplied_by

// ----------------------------------------------------------------------

class ComputedColumnBases : public acmacs::chart::ColumnBasesData
{
 public:
    ComputedColumnBases(size_t aNumberOfSera, double aMinimumColumnBasis) : ColumnBasesData(aNumberOfSera, aMinimumColumnBasis) {}

    void update(size_t aSerumNo, double aValue)
        {
            if (aValue > column_basis(aSerumNo))
                set(aSerumNo, aValue);
        }

}; // class ComputedColumnBases

std::shared_ptr<acmacs::chart::ColumnBasesData> acmacs::chart::Titers::computed_column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis) const
{
    // auto cb1 = std::make_shared<ComputedColumnBases>(number_of_sera, aMinimumColumnBasis);
    // for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no)
    //     for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
    //         cb1->update(sr_no, titer(ag_no, sr_no).logged_for_column_bases());
    // // for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
    // //     cb->update(sr_no, aMinimumColumnBasis);

    // std::cerr << "DEBUG: computed_column_bases_old: " << *cb1 << '\n';

    auto cb = std::make_shared<ComputedColumnBases>(number_of_sera(), aMinimumColumnBasis);

    for (const auto& titer_ref : titers_existing())
        cb->update(titer_ref.serum, titer_ref.titer.logged_for_column_bases());

    // for (const auto& t : *this)
    //     if (t.serum == 30)
    //         std::cerr << "DEBUG: " << t.antigen << ' ' << t.serum << ' ' << t.titer << '\n';
    // std::cerr << "DEBUG: computed_column_bases_new: " << cb->column_basis(30) << '\n';

    return cb;

} // acmacs::chart::Titers::computed_column_bases

// ----------------------------------------------------------------------

void acmacs::chart::Titers::update(acmacs::chart::TableDistances& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const
{
    const auto num_antigens{number_of_antigens()};
    const auto number_of_points{num_antigens + number_of_sera()};
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (number_of_sera()) {
        for (const auto& titer_ref : titers_existing()) {
            if (!parameters.disconnected.contains(titer_ref.antigen) && !parameters.disconnected.contains(titer_ref.serum + num_antigens))
                table_distances.update(titer_ref.titer, titer_ref.antigen, titer_ref.serum + num_antigens, column_bases.column_basis(titer_ref.serum), logged_adjusts[titer_ref.antigen] + logged_adjusts[titer_ref.serum + num_antigens], parameters.mult);
        }
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + AD_DEBUG_FILE_LINE_FUNC_S);
    }
} // acmacs::chart::Titers::update

// ----------------------------------------------------------------------

acmacs::chart::TableDistances acmacs::chart::Titers::table_distances(const ColumnBases& column_bases, const StressParameters& parameters)
{
    acmacs::chart::TableDistances result;
    update(result, column_bases, parameters);
    return result;

} // acmacs::chart::Titers::table_distances

// ----------------------------------------------------------------------

bool acmacs::chart::Titers::is_dense() const noexcept
{
    try {
        rjson_list_dict();
        return false;
    }
    catch (data_not_available&) {
        try {
            rjson_list_list();
            return true;
        }
        catch (data_not_available&) {
            return percent_of_non_dont_cares() > dense_sparse_boundary;
        }
    }

} // acmacs::chart::Titers::is_dense

// ----------------------------------------------------------------------

double acmacs::chart::Titers::max_distance(const acmacs::chart::ColumnBases& column_bases)
{

    double max_distance = 0;
    if (number_of_sera()) {
        for (const auto& titer_ref : titers_existing())
            max_distance = std::max(max_distance, column_bases.column_basis(titer_ref.serum) - titer_ref.titer.logged_with_thresholded());
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + AD_DEBUG_FILE_LINE_FUNC_S);
    }
      // std::cerr << "Titers::max_distance: " << max_distance << '\n';
    return max_distance;

} // acmacs::chart::Titers::max_distance

// ----------------------------------------------------------------------

std::pair<acmacs::chart::PointIndexList, acmacs::chart::PointIndexList> acmacs::chart::Titers::antigens_sera_of_layer(size_t aLayerNo) const
{
    acmacs::chart::PointIndexList antigens, sera;

    for (const auto& titer_ref : titers_existing_from_layer(aLayerNo)) {
        antigens.insert(titer_ref.antigen);
        sera.insert(titer_ref.serum);
    }
    return {antigens, sera};

} // acmacs::chart::Titers::antigens_sera_of_layer

// ----------------------------------------------------------------------

std::pair<acmacs::chart::PointIndexList, acmacs::chart::PointIndexList> acmacs::chart::Titers::antigens_sera_in_multiple_layers() const
{
    std::map<size_t, std::set<size_t>> antigen_to_layers, serum_to_layers;
    for (size_t layer_no = 0; layer_no < number_of_layers(); ++layer_no) {
        for (const auto& titer_ref : titers_existing_from_layer(layer_no)) {
            antigen_to_layers[titer_ref.antigen].insert(layer_no);
            serum_to_layers[titer_ref.serum].insert(layer_no);
        }
    }
    acmacs::chart::PointIndexList antigens, sera;
    for (const auto& ag : antigen_to_layers) {
        if (ag.second.size() > 1)
            antigens.insert(ag.first);
    }
    for (const auto& sr : serum_to_layers) {
        if (sr.second.size() > 1)
            sera.insert(sr.first);
    }
    return {antigens, sera};

} // acmacs::chart::Titers::antigens_sera_in_multiple_layers

// ----------------------------------------------------------------------

bool acmacs::chart::Titers::has_morethan_in_layers() const
{
    for (size_t layer_no = 0; layer_no < number_of_layers(); ++layer_no) {
        for (const auto& titer_ref : titers_existing_from_layer(layer_no)) {
            if (titer_ref.titer.is_more_than())
                return true;
        }
    }
    return false;

} // acmacs::chart::Titers::has_morethan_in_layers

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::chart::Titers::having_titers_with(size_t point_no) const
{
    const auto num_antigens = number_of_antigens();
    PointIndexList result;
    if (point_no < num_antigens) {
        for (const auto& titer_ref : titers_existing()) {
            if (titer_ref.antigen == point_no)
                result.insert(titer_ref.serum + num_antigens);
        }
    }
    else {
        const auto serum_no = point_no - num_antigens;
        for (const auto& titer_ref : titers_existing()) {
            if (titer_ref.serum == serum_no)
                result.insert(titer_ref.antigen);
        }
    }
    return result;

} // acmacs::chart::Titers::having_titers_with

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::chart::Titers::having_too_few_numeric_titers(size_t threshold) const
{
    std::vector<size_t> number_of_numeric_titers(number_of_antigens() + number_of_sera(), 0);
    for (const auto& titer_ref : titers_existing()) {
        if (titer_ref.titer.is_regular()) {
            ++number_of_numeric_titers[titer_ref.antigen];
            ++number_of_numeric_titers[titer_ref.serum + number_of_antigens()];
        }
    }
    PointIndexList result;
    for (auto [point_no, num_numeric_titers] : acmacs::enumerate(number_of_numeric_titers)) {
        if (num_numeric_titers < threshold)
            result.insert(point_no);
    }
    return result;

} // acmacs::chart::Titers::having_too_few_numeric_titers

// ----------------------------------------------------------------------

std::string acmacs::chart::Titers::print() const
{
    std::string result;
    for (auto ag_no : range(number_of_antigens())) {
        for (auto sr_no : range(number_of_sera())) {
            const auto tt = titer(ag_no, sr_no);
            result.append(static_cast<size_t>(std::max(0, (5 - static_cast<int>(tt.size())))) + (sr_no > 0 ? 1 : 0), ' ');
            result.append(titer(ag_no, sr_no));
        }
        result.append(1, '\n');
    }
    return result;

} // acmacs::chart::Titers::print

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
