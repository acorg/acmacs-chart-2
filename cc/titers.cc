#include "acmacs-base/debug.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/titers.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

acmacs::chart::Titer::Type acmacs::chart::Titer::type() const
{
    if (data().empty())
        return Invalid;
    switch (data()[0]) {
      case '*':
          return DontCare;
      case '<':
          return LessThan;
      case '>':
          return MoreThan;
      case '~':
          return Dodgy;
      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
          return Regular;
      default:
          return Invalid;
    }
    // return Invalid;

} // acmacs::chart::Titer::type

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
      case LessThan:
      case MoreThan:
      case Dodgy:
          return log_titer(data().substr(1));
    }
    throw invalid_titer(data()); // for gcc 7.2

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
    throw invalid_titer(data()); // for gcc 7.2

} // acmacs::chart::Titer::logged_with_thresholded

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
      case LessThan:
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
      case LessThan:
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

size_t acmacs::chart::Titer::value_for_sorting() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return std::stoul(data());
      case LessThan:
          return std::stoul(data().substr(1)) - 1;
      case MoreThan:
          return std::stoul(data().substr(1)) + 1;
      case Dodgy:
          return std::stoul(data().substr(1));
    }
    return 0;

} // acmacs::chart::Titer::value_for_sorting

// ----------------------------------------------------------------------

class ComputedColumnBases : public acmacs::chart::ColumnBases
{
 public:
    inline ComputedColumnBases(size_t aNumberOfSera) : mData(aNumberOfSera, 0) {}

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

std::shared_ptr<acmacs::chart::ColumnBases> acmacs::chart::Titers::computed_column_bases(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis, size_t number_of_antigens, size_t number_of_sera) const
{
    auto cb = std::make_shared<ComputedColumnBases>(number_of_sera);
    for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no)
        for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
            cb->update(sr_no, titer(ag_no, sr_no).logged_for_column_bases());
    for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no)
        cb->update(sr_no, aMinimumColumnBasis);
    return cb;

} // acmacs::chart::Titers::computed_column_bases

// ----------------------------------------------------------------------

template <typename Float> static void update(const acmacs::chart::Titers& titers, acmacs::chart::TableDistances<Float>& table_distances, const acmacs::chart::ColumnBases& column_bases, const acmacs::chart::StressParameters& parameters)
{
    const auto number_of_points = titers.number_of_antigens() + titers.number_of_sera();
    const auto logged_adjusts = parameters.avidity_adjusts.logged(number_of_points);
    table_distances.dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (titers.number_of_sera()) {
        for (auto p1 : acmacs::range(titers.number_of_antigens())) {
            if (!parameters.disconnected.exist(p1)) {
                for (auto p2 : acmacs::range(titers.number_of_antigens(), number_of_points)) {
                    if (!parameters.disconnected.exist(p2)) {
                        const auto serum_no = p2 - titers.number_of_antigens();
                        table_distances.update(titers.titer(p1, serum_no), p1, p2, column_bases.column_basis(serum_no), logged_adjusts[p1] + logged_adjusts[p2], parameters.multiply_antigen_titer_until_column_adjust);
                    }
                }
            }
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

double acmacs::chart::Titers::max_distance(const acmacs::chart::ColumnBases& column_bases)
{
    double max_distance{0};
    if (number_of_sera()) {
        for (auto ag_no : acmacs::range(number_of_antigens())) {
            for (auto sr_no : acmacs::range(number_of_sera())) {
                const auto t = titer(ag_no, sr_no);
                if (!t.is_dont_care())
                    max_distance = std::max(max_distance, column_bases.column_basis(sr_no) - t.logged_with_thresholded());
            }
        }
    }
    else {
        throw std::runtime_error("genetic table support not implemented in " + DEBUG_LINE_FUNC_S);
    }
    // std::cerr << "Titers::max_distance: " << max_distance << '\n';
    return max_distance;

} // acmacs::chart::Titers::max_distance

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
