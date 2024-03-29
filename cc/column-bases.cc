#include "acmacs-base/fmt.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/string-from-chars.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

// std::string acmacs::to_string(const acmacs::chart::ColumnBases& aColumnBases)
// {
//     fmt::memory_buffer result;
//     fmt::format_to_mb(result, "[");
//     for (auto serum_no: acmacs::range(0, aColumnBases.size())) {
//         if (serum_no)
//             fmt::format_to_mb(result, " ");
//         fmt::format_to_mb(result, "{}", aColumnBases.column_basis(serum_no));
//     }
//     fmt::format_to_mb(result, "]");
//     return fmt::to_string(result);

// } // acmacs::to_string

// ----------------------------------------------------------------------

void acmacs::chart::MinimumColumnBasis::from(std::string_view value)
{
    if (value.empty() || value == "none") {
        value_ = 0;
    }
    else if (value.find('.') != std::string::npos) {
        value_ = acmacs::string::from_chars<double>(value);
    }
    else {
        value_ = static_cast<double>(acmacs::string::from_chars<long>(value));
        if (value_ > 9)
            value_ = std::log2(value_ / 10.0);
    }
    if (value_ < 0 || value_ > 30)
        throw std::runtime_error{fmt::format("Unrecognized minimum_column_basis value: {}", value)};

} // acmacs::chart::MinimumColumnBasis::from

// ----------------------------------------------------------------------

std::string acmacs::chart::MinimumColumnBasis::format(std::string_view format, use_none un) const noexcept
{
    if (is_none()) {
        if (un == use_none::yes)
            return fmt::format(fmt::runtime(format), "none");
        else
            return {};
    }
    else
        return fmt::format(fmt::runtime(format), std::lround(std::exp2(value_) * 10.0));

} // acmacs::chart::MinimumColumnBasis::format

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
