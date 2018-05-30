#include <algorithm>
#include <cmath>

#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/titers.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

double acmacs::chart::Titer::logged() const
{
    constexpr auto log_titer = [](std::string source) -> double { return std::log2(std::stod(source) / 10.0); };

    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
          return log_titer(*this);
      case DontCare:
          throw invalid_titer(*this);
      case LessThan:
      case MoreThan:
      case Dodgy:
          return log_titer(substr(1));
    }
    throw invalid_titer(*this); // for gcc 7.2

} // acmacs::chart::Titer::logged

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
    //     std::cerr << "DEBUG: " << t.antigen << ' ' << t.serum << ' ' << t.titer << '\n';
    // std::cerr << "DEBUG: computed_column_bases_new: " << *cb << '\n';
    return cb;

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
        TiterIteratorImplementation(const acmacs::chart::Titers& titers)
            : acmacs::chart::TiterIterator::Implementation(titers.titer(0, 0), 0, 0), titers_{titers}
            {
                if (data_.titer.is_dont_care())
                    operator++();
            }

        TiterIteratorImplementation(const acmacs::chart::Titers& titers, size_t number_of_antigens)
            : acmacs::chart::TiterIterator::Implementation({}, number_of_antigens, 0), titers_{titers} {}

        void operator++() override
            {
                const auto num_antigens = titers_.number_of_antigens();
                const auto num_sera = titers_.number_of_sera();
                while (data_.antigen < num_antigens) {
                    if (++data_.serum >= num_sera) {
                        ++data_.antigen;
                        data_.serum = 0;
                    }
                    if (data_.antigen < num_antigens) {
                        data_.titer = titers_.titer(data_.antigen, data_.serum);
                        if (!data_.titer.is_dont_care())
                            break;
                    }
                    else
                        data_.titer = acmacs::chart::Titer{};
                }
            }

     private:
        const acmacs::chart::Titers& titers_;

    }; // class TiterIteratorImplementation
}

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::Titers::begin() const
{
    return {new TiterIteratorImplementation(*this)};

} // acmacs::chart::Titers::begin

// ----------------------------------------------------------------------

acmacs::chart::TiterIterator acmacs::chart::Titers::end() const
{
    return {new TiterIteratorImplementation(*this, number_of_antigens())};

} // acmacs::chart::Titers::end

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
