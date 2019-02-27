#pragma once

#include <optional>
#include <algorithm>

#include "acmacs-chart-2/titers.hh"

// ----------------------------------------------------------------------

namespace acmacs { class Layout; }

namespace acmacs::chart
{
    class SerumCircle;

    enum class serum_circle_failure_reason { not_calculated, non_regular_homologous_titer, titer_too_low };

    namespace detail
    {
        struct PerAntigen
        {
            PerAntigen() = default;
            PerAntigen(size_t ag_no, const Titer& tit) : antigen_no{ag_no}, titer{tit} { if (!tit.is_regular()) failure_reason = serum_circle_failure_reason::non_regular_homologous_titer; }

            constexpr bool operator<(const PerAntigen& rhs) const { return radius.has_value() ? (rhs.radius.has_value() ? *radius < *rhs.radius : true) : false; }
            constexpr bool valid() const { return radius.has_value(); }

            size_t antigen_no;
            Titer titer;
            std::optional<double> radius;
            serum_circle_failure_reason failure_reason{serum_circle_failure_reason::not_calculated};
        };

        void serum_circle_empirical(const SerumCircle& circle_data, PerAntigen& per_antigen, const Layout& layout, const Titers& titers);
        void serum_circle_theoretical(const SerumCircle& circle_data, PerAntigen& per_antigen);
    }

    class SerumCircle
    {
      public:

        SerumCircle() = default;
        SerumCircle(size_t antigen_no, size_t serum_no, double column_basis, Titer homologous_titer);
        SerumCircle(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers);

        bool valid() const
        {
            sort();
            return !per_antigen_.empty() && per_antigen_.front().valid();
        }
        operator bool() const { return valid(); }
        serum_circle_failure_reason failure_reason() const
        {
            sort();
            return per_antigen_.empty() ? serum_circle_failure_reason::not_calculated : per_antigen_.front().failure_reason;
        }

        double radius() const
        {
            sort();
            return *per_antigen_.front().radius;
        }
        // constexpr size_t antigen_no() const { return antigen_no_; }
        constexpr size_t serum_no() const { return serum_no_; }
        constexpr double column_basis() const { return column_basis_; }
        // constexpr const Titer& homologous_titer() const { return homologous_titer_; }

        const char* report_reason() const;

      private:
        size_t serum_no_;
        double column_basis_;
        mutable std::vector<detail::PerAntigen> per_antigen_;
        mutable bool sorted_ = false;

        void sort() const
        {
            if (!sorted_) {
                std::sort(std::begin(per_antigen_), std::end(per_antigen_));
                sorted_ = true;
            }
        }

        friend void detail::serum_circle_empirical(const SerumCircle& circle_data, detail::PerAntigen& per_antigen, const Layout& layout, const Titers& titers);
        friend void detail::serum_circle_theoretical(const SerumCircle& circle_data, detail::PerAntigen& per_antigen);
        friend SerumCircle serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers);
        friend SerumCircle serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers);
        friend SerumCircle serum_circle_empirical(const PointIndexList& antigens, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers);
        friend SerumCircle serum_circle_theoretical(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers);
    };

    SerumCircle serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers);
    SerumCircle serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers);
    SerumCircle serum_circle_empirical(const PointIndexList& antigens, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers);
    SerumCircle serum_circle_theoretical(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: