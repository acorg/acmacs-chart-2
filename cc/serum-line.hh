#pragma once

#include "acmacs-base/line.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Projection;

    class SerumLine
    {
      public:
        SerumLine(const Projection& projection);

        constexpr const acmacs::LineDefinedByEquation& line() const { return line_; }
        constexpr double standard_deviation() const { return standard_deviation_; }

      private:
        acmacs::LineDefinedByEquation line_;
        double standard_deviation_;

    }; // class SerumLine

    inline std::ostream& operator<<(std::ostream& out, const SerumLine& line) { return out << "SerumLine(sd:" << line.standard_deviation() << " slope:" << line.line().slope() << ", intercept:" << line.line().intercept() << ')'; }

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
