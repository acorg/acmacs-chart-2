#pragma once

#include <optional>
#include <algorithm>

#include "acmacs-base/debug.hh"
#include "acmacs-chart-2/titers.hh"

// ----------------------------------------------------------------------

namespace acmacs { class Layout; }

namespace acmacs::chart
{
    class SerumCircle;
    class Chart;

    enum class serum_circle_failure_reason { not_calculated, non_regular_homologous_titer, titer_too_low, serum_disconnected, antigen_disconnected };

    namespace detail
    {
        struct SerumCirclePerAntigen
        {
            SerumCirclePerAntigen() = default;
            SerumCirclePerAntigen(size_t ag_no, const Titer& tit) : antigen_no{ag_no}, titer{tit} { if (!tit.is_regular()) failure_reason = serum_circle_failure_reason::non_regular_homologous_titer; }

            constexpr bool operator<(const SerumCirclePerAntigen& rhs) const { return radius.has_value() ? (rhs.radius.has_value() ? *radius < *rhs.radius : true) : false; }
            constexpr bool valid() const { return radius.has_value(); }
            const char* report_reason() const;

            size_t antigen_no;
            Titer titer;
            std::optional<double> radius;
            serum_circle_failure_reason failure_reason{serum_circle_failure_reason::not_calculated};
        };
        void serum_circle_empirical(const SerumCircle& circle_data, SerumCirclePerAntigen& per_antigen, const Layout& layout, const Titers& titers, double fold, acmacs::verbose verbose);
        void serum_circle_theoretical(const SerumCircle& circle_data, SerumCirclePerAntigen& per_antigen, double fold);
    }

    class SerumCircle
    {
      public:
        SerumCircle() = default;
        SerumCircle(size_t antigen_no, size_t serum_no, double column_basis, Titer homologous_titer, double fold);
        SerumCircle(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers, double fold);
        SerumCircle(const PointIndexList& antigens, size_t serum_no, double column_basis, Titer homologous_titer, double fold);

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
            return std::max(*per_antigen_.front().radius, min_radius);
        }

        constexpr size_t serum_no() const { return serum_no_; }
        constexpr double fold() const { return fold_; }
        constexpr double column_basis() const { return column_basis_; }

        const char* report_reason() const
        {
            sort();
            return per_antigen_.empty() ? "not calculated" : per_antigen_.front().report_reason();
        }

        constexpr const std::vector<detail::SerumCirclePerAntigen>& per_antigen() const { return per_antigen_; }

      private:
        size_t serum_no_;
        double fold_ = 2.0;
        double column_basis_;
        double min_radius{2.0}; // Derek 2020-09-16 13:16 (Influenza B report and sig pages)
        mutable std::vector<detail::SerumCirclePerAntigen> per_antigen_;
        mutable bool sorted_ = false;

        void sort() const
        {
            if (!sorted_) {
                std::sort(std::begin(per_antigen_), std::end(per_antigen_));
                sorted_ = true;
            }
        }

        friend void detail::serum_circle_empirical(const SerumCircle& circle_data, detail::SerumCirclePerAntigen& per_antigen, const Layout& layout, const Titers& titers, double fold, acmacs::verbose verbose);
        friend void detail::serum_circle_theoretical(const SerumCircle& circle_data, detail::SerumCirclePerAntigen& per_antigen, double fold);
        friend SerumCircle serum_circle_empirical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers, double fold, acmacs::verbose verbose);
        friend SerumCircle serum_circle_theoretical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, double column_basis, double fold);
        friend SerumCircle serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers, double fold, acmacs::verbose verbose);
        friend SerumCircle serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers, double fold);
        friend SerumCircle serum_circle_empirical(const PointIndexList& antigens, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers, double fold, acmacs::verbose verbose);
        friend SerumCircle serum_circle_theoretical(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers, double fold);
    };

    SerumCircle serum_circle_empirical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, const Chart& chart, size_t aProjectionNo, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_empirical(size_t aAntigenNo, size_t aSerumNo, const Chart& chart, size_t aProjectionNo, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_empirical(const PointIndexList& antigens, size_t serum_no, const Chart& chart, size_t aProjectionNo, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);

    SerumCircle serum_circle_theoretical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, const Chart& chart, size_t aProjectionNo, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_theoretical(size_t aAntigenNo, size_t aSerumNo, const Chart& chart, size_t aProjectionNo, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_theoretical(const PointIndexList& antigens, size_t serum_no, const Chart& chart, size_t aProjectionNo, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);

    SerumCircle serum_circle_empirical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_theoretical(const PointIndexList& antigens, Titer homologous_titer, size_t serum_no, double column_basis, double fold = 2.0);
    SerumCircle serum_circle_empirical(size_t antigen_no, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_theoretical(size_t antigen_no, size_t serum_no, double column_basis, const Titers& titers, double fold = 2.0);
    SerumCircle serum_circle_empirical(const PointIndexList& antigens, size_t serum_no, const Layout& layout, double column_basis, const Titers& titers, double fold = 2.0, acmacs::verbose verbose = acmacs::verbose::no);
    SerumCircle serum_circle_theoretical(const PointIndexList& antigens, size_t serum_no, double column_basis, const Titers& titers, double fold = 2.0);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
