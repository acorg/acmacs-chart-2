#include "acmacs-base/range.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

std::string acmacs::to_string(const acmacs::chart::ColumnBases& aColumnBases)
{
    std::string result{'['};
    for (auto serum_no: acmacs::range(0UL, aColumnBases.size())) {
        if (serum_no)
            result += ' ';
        result += std::to_string(aColumnBases.column_basis(serum_no));
    }
    return result;

} // acmacs::to_string

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
