#include <algorithm>

#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-chart-2/titers.hh"
#include "acmacs-chart-2/chart.hh"

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
          return *this;
      case LessThan:
      case MoreThan:
      case Dodgy:
          return front() + acmacs::to_string(logged());
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
          return std::stoul(substr(1)) - 1;
      case MoreThan:
          return std::stoul(substr(1)) + 1;
      case Dodgy:
          return std::stoul(substr(1));
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
          return std::stoul(*this);
      case LessThan:
      case MoreThan:
      case Dodgy:
          return std::stoul(substr(1));
    }
    return 0;

} // acmacs::chart::Titer::value

// ----------------------------------------------------------------------

acmacs::chart::Titer acmacs::chart::Titer::multiplied_by(double value) const // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return *this;
      case Regular:
          return std::to_string(std::lround(std::stoul(*this) * value));
      case LessThan:
      case MoreThan:
      case Dodgy:
          return front() + std::to_string(std::lround(std::stoul(substr(1)) * value));
    }
    return Titer{};

} // acmacs::chart::Titer::multiplied_by

// ----------------------------------------------------------------------

class ComputedColumnBases : public acmacs::chart::ColumnBases
{
 public:
    inline ComputedColumnBases(size_t aNumberOfSera, double aMinimumColumnBasis) : mData(aNumberOfSera, aMinimumColumnBasis) {}

    inline double column_basis(size_t aSerumNo) const override { return mData.at(aSerumNo); }
    inline size_t size() const override { return mData.size(); }

    inline void update(size_t aSerumNo, double aValue)
        {
            if (aValue > mData[aSerumNo])
                mData[aSerumNo] = aValue;
        }

 private:
    std::vector<double> mData;

}; // class ComputedColumnBases

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Titers::computed_column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis) const
{
    // auto cb1 = std::make_shared<ComputedColumnBases>(number_of_sera, aMinimumColumnBasis);
    // for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no)
    //     for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
    //         cb1->update(sr_no, titer(ag_no, sr_no).logged_for_column_bases());
    // // for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
    // //     cb->update(sr_no, aMinimumColumnBasis);

    // std::cerr << "DEBUG: computed_column_bases_old: " << *cb1 << '\n';

    auto cb = std::make_shared<ComputedColumnBases>(number_of_sera(), aMinimumColumnBasis);

    std::for_each(begin(), end(), [&cb](const auto& titer_data) { cb->update(titer_data.serum, titer_data.titer.logged_for_column_bases()); });

    // for (const auto& t : *this)
    //     if (t.serum == 30)
    //         std::cerr << "DEBUG: " << t.antigen << ' ' << t.serum << ' ' << t.titer << '\n';
    // std::cerr << "DEBUG: computed_column_bases_new: " << cb->column_basis(30) << '\n';

    return std::move(cb);

} // acmacs::chart::Titers::computed_column_bases

// ----------------------------------------------------------------------

template <typename Float> static void update(const acmacs::chart::Titers& titers, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters)
{
    const auto number_of_antigens = titers.number_of_antigens();
    const auto number_of_points = number_of_antigens + titers.number_of_sera();
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (titers.number_of_sera()) {
        for (const auto& titer_ref : titers) {
            if (!parameters.disconnected.contains(titer_ref.antigen) && !parameters.disconnected.contains(titer_ref.serum + number_of_antigens))
                table_distances.update(titer_ref.titer, titer_ref.antigen, titer_ref.serum + number_of_antigens, column_bases.column_basis(titer_ref.serum), logged_adjusts[titer_ref.antigen] + logged_adjusts[titer_ref.serum + number_of_antigens], parameters.mult);
        }
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
    }
}

void acmacs::chart::Titers::update(acmacs::chart::TableDistances<float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const
{
    ::update(*this, table_distances, column_bases, parameters);

} // acmacs::chart::Titers::update

// ----------------------------------------------------------------------

void acmacs::chart::Titers::update(acmacs::chart::TableDistances<double>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters) const
{
    ::update(*this, table_distances, column_bases, parameters);

} // acmacs::chart::Titers::update

// ----------------------------------------------------------------------

acmacs::chart::TableDistances<double> acmacs::chart::Titers::table_distances(const ColumnBases& column_bases, const StressParameters& parameters)
{
    acmacs::chart::TableDistances<double> result;
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
        for (const auto& titer_ref : *this)
            max_distance = std::max(max_distance, column_bases.column_basis(titer_ref.serum) - titer_ref.titer.logged_with_thresholded());
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
    }
      // std::cerr << "Titers::max_distance: " << max_distance << '\n';
    return max_distance;

} // acmacs::chart::Titers::max_distance

// ----------------------------------------------------------------------

namespace                       // to make class static in the module
{
    class TiterIteratorImplementation : public acmacs::chart::TiterIterator::Implementation
    {
     public:
        using titer_getter_t = std::function<std::string (size_t, size_t)>;

          // begin
        TiterIteratorImplementation(titer_getter_t titer_getter, size_t number_of_antigens, size_t number_of_sera)
            : acmacs::chart::TiterIterator::Implementation(titer_getter(0, 0), 0, 0), titer_getter_{titer_getter}, number_of_antigens_{number_of_antigens}, number_of_sera_{number_of_sera}
            {
                if (data_.titer.is_dont_care())
                    operator++();
            }

          // end
        TiterIteratorImplementation(size_t number_of_antigens)
            : acmacs::chart::TiterIterator::Implementation({}, number_of_antigens, 0), titer_getter_{[](size_t, size_t) -> std::string { return ""; }}, number_of_antigens_{number_of_antigens}, number_of_sera_{0} {}

        void operator++() override
            {
                while (data_.antigen < number_of_antigens_) {
                    if (++data_.serum >= number_of_sera_) {
                        ++data_.antigen;
                        data_.serum = 0;
                    }
                    if (data_.antigen < number_of_antigens_) {
                        data_.titer = titer_getter_(data_.antigen, data_.serum);
                        if (!data_.titer.is_dont_care())
                            break;
                    }
                    else
                        data_.titer = acmacs::chart::Titer{};
                }
            }

     private:
        titer_getter_t titer_getter_;
        size_t number_of_antigens_, number_of_sera_;

    }; // class TiterIteratorImplementation
}

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::Titers::begin() const
{
    return {new TiterIteratorImplementation([this](size_t ag, size_t sr) -> std::string { return titer(ag, sr); }, number_of_antigens(), number_of_sera())};

} // acmacs::chart::Titers::begin

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::Titers::end() const
{
    return {new TiterIteratorImplementation(number_of_antigens())};

} // acmacs::chart::Titers::end

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::Titers::begin(size_t aLayerNo) const
{
    return {new TiterIteratorImplementation([this,aLayerNo](size_t ag, size_t sr) -> std::string { return titer_of_layer(aLayerNo, ag, sr); }, number_of_antigens(), number_of_sera())};

} // acmacs::chart::Titers::begin

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::Titers::end(size_t /*aLayerNo*/) const
{
    return {new TiterIteratorImplementation(number_of_antigens())};

} // acmacs::chart::Titers::end

// ----------------------------------------------------------------------

std::pair<acmacs::chart::PointIndexList, acmacs::chart::PointIndexList> acmacs::chart::Titers::antigens_sera_of_layer(size_t aLayerNo) const
{
    acmacs::chart::PointIndexList antigens, sera;
    for (auto titer_it = begin(aLayerNo); titer_it != end(aLayerNo); ++titer_it) {
        antigens.insert(titer_it->antigen);
        sera.insert(titer_it->serum);
    }
    return {antigens, sera};

} // acmacs::chart::Titers::antigens_sera_of_layer

// ----------------------------------------------------------------------

std::pair<acmacs::chart::PointIndexList, acmacs::chart::PointIndexList> acmacs::chart::Titers::antigens_sera_in_multiple_layers() const
{
    std::map<size_t, std::set<size_t>> antigen_to_layers, serum_to_layers;
    for (size_t layer_no = 0; layer_no < number_of_layers(); ++layer_no) {
        for (auto titer_it = begin(layer_no); titer_it != end(layer_no); ++titer_it) {
            antigen_to_layers[titer_it->antigen].insert(layer_no);
            serum_to_layers[titer_it->serum].insert(layer_no);
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
        for (auto titer_it = begin(layer_no); titer_it != end(layer_no); ++titer_it) {
            if (titer_it->titer.is_more_than())
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
        for (const auto& titer_ref : *this) {
            if (titer_ref.antigen == point_no)
                result.insert(titer_ref.serum + num_antigens);
        }
    }
    else {
        const auto serum_no = point_no - num_antigens;
        for (const auto& titer_ref : *this) {
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
    for (const auto& titer_ref : *this) {
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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
