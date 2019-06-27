#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

std::string acmacs::to_string(const acmacs::chart::ColumnBases& aColumnBases)
{
    fmt::memory_buffer result;
    fmt::format_to(result, "[");
    for (auto serum_no: acmacs::range(0, aColumnBases.size())) {
        if (serum_no)
            fmt::format_to(result, " ");
        fmt::format_to(result, "{}", aColumnBases.column_basis(serum_no));
    }
    fmt::format_to(result, "]");
    return fmt::to_string(result);

} // acmacs::to_string

// ----------------------------------------------------------------------

void acmacs::chart::MinimumColumnBasis::from(std::string_view value)
{
    if (value.empty() || value == "none") {
        value_ = 0;
    }
    else if (value.find('.') != std::string::npos) {
        value_ = std::stod(std::string(value));
    }
    else {
        value_ = std::stol(std::string(value));
        if (value_ > 9)
            value_ = std::log2(value_ / 10.0);
    }
    if (value_ < 0 || value_ > 30)
        throw std::runtime_error{"Unrecognized minimum_column_basis value: " + std::string(value)};

} // acmacs::chart::MinimumColumnBasis::from

// ----------------------------------------------------------------------

acmacs::chart::MinimumColumnBasis::operator std::string() const noexcept
{
    if (is_none())
        return "none";
    else
        return acmacs::to_string(std::lround(std::exp2(value_) * 10.0));

} // operator std::string

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
