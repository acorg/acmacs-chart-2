#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "acmacs-chart-2/log.hh"
#include "acmacs-chart-2/point-index-list.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
    using Indexes = PointIndexList;

    class CommonAntigensSera
    {
     public:
        enum class match_level_t { strict, relaxed, ignored, automatic };

        CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level);
        CommonAntigensSera(const Chart& primary); // for procrustes between projections of the same chart
        CommonAntigensSera(const CommonAntigensSera&) = delete;
        CommonAntigensSera(CommonAntigensSera&&);
        ~CommonAntigensSera();
        CommonAntigensSera& operator=(const CommonAntigensSera&) = delete;
        CommonAntigensSera& operator=(CommonAntigensSera&&);

        [[nodiscard]] std::string report(size_t indent = 0) const;
        operator bool() const;
        size_t common_antigens() const;
        size_t common_sera() const;

        void keep_only(const PointIndexList& antigens, const PointIndexList& sera);
        void antigens_only();   // remove sera from common lists
        void sera_only();   // remove antigens from common lists

        struct common_t
        {
            common_t(size_t p, size_t s) : primary(p), secondary(s) {}
            size_t primary;
            size_t secondary;
        };

        std::vector<common_t> antigens() const;
        std::vector<common_t> sera() const; // returns serum indexes (NOT point indexes)!
        std::vector<common_t> sera_as_point_indexes() const;
        std::vector<common_t> points() const;

        Indexes common_primary_antigens() const;
        Indexes common_primary_sera() const;  // returns serum indexes (NOT point indexes)!

          // common antigen/serum mapping
        std::optional<size_t> antigen_primary_by_secondary(size_t secondary_no) const;
        std::optional<size_t> antigen_secondary_by_primary(size_t primary_no) const;
        std::optional<size_t> serum_primary_by_secondary(size_t secondary_no) const;
        std::optional<size_t> serum_secondary_by_primary(size_t primary_no) const;

        enum class subset { all, antigens, sera };
        std::vector<common_t> points(subset a_subset) const;
        std::vector<common_t> points_for_primary_antigens(const Indexes& antigen_indexes) const;
        std::vector<common_t> points_for_primary_sera(const Indexes& serum_indexes) const;

        static match_level_t match_level(std::string_view source);

     private:
        class Impl;
        std::unique_ptr<Impl> impl_;

    }; // class CommonAntigensSera

} // namespace acmacs::chart

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::chart::CommonAntigensSera::common_t> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::chart::CommonAntigensSera::common_t& common, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{{{},{}}}", common.primary, common.secondary);
    }
};

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
