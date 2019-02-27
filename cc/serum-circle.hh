#pragma once

#include <optional>

#include "acmacs-chart-2/titers.hh"

// ----------------------------------------------------------------------

namespace acmacs { class Layout; }

namespace acmacs::chart
{

    class SerumCircle
    {
      public:
        enum failure_reason { no_failure, non_regular_homologous_titer, titer_too_low }; // titer_too_low: protects everything

        SerumCircle(size_t antigen_no, size_t serum_no, double column_basis, Titer homologous_titer)
            : antigen_no_{antigen_no}, serum_no_{serum_no}, column_basis_{column_basis}, homologous_titer_{homologous_titer}
        {
        }

        constexpr bool exists() const { return radius_.has_value(); }
        constexpr operator bool() const { return exists(); }
        constexpr enum failure_reason failure_reason() const { return failure_reason_; }

        constexpr double radius() const { return *radius_; }
        constexpr size_t antigen_no() const { return antigen_no_; }
        constexpr size_t serum_no() const { return serum_no_; }
        constexpr double column_basis() const { return column_basis_; }
        constexpr const Titer& homologous_titer() const { return homologous_titer_; }

        const char* report_reason() const;

      private:
        const size_t antigen_no_, serum_no_;
        const double column_basis_;
        const Titer homologous_titer_;
        enum failure_reason failure_reason_{no_failure};
        std::optional<double> radius_;

        SerumCircle& fail(enum failure_reason reason) { failure_reason_ = reason; return *this; }

        friend SerumCircle serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers);
        friend SerumCircle serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers);
    };

    SerumCircle serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers);
    SerumCircle serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
