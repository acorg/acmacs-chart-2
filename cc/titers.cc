#include "acmacs-chart-2/titers.hh"

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


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
