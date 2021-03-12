#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    using Indexes = PointIndexList;

    namespace common
    {
        struct CoreEntry
        {
            CoreEntry() = default;
            CoreEntry(CoreEntry&&) = default;
            template <typename AgSr> CoreEntry(size_t a_index, const AgSr& ag_sr) : index(a_index), name(ag_sr.name()), reassortant(ag_sr.reassortant()), annotations(ag_sr.annotations()) {}
            CoreEntry& operator=(CoreEntry&&) = default;

            static inline int compare(const CoreEntry& lhs, const CoreEntry& rhs)
            {
                if (auto n_c = lhs.name.compare(rhs.name); n_c != 0)
                    return n_c;
                if (auto r_c = lhs.reassortant.compare(rhs.reassortant); r_c != 0)
                    return r_c;
                return ::string::compare(fmt::format("{: }", lhs.annotations), fmt::format("{: }", rhs.annotations));
            }

            static inline bool less(const CoreEntry& lhs, const CoreEntry& rhs) { return compare(lhs, rhs) < 0; }

            size_t index;
            acmacs::virus::name_t name;
            acmacs::virus::Reassortant reassortant;
            Annotations annotations;

        }; // struct CoreEntry

        struct AntigenEntry : public CoreEntry
        {
            AntigenEntry() = default;
            AntigenEntry(AntigenEntry&&) = default;
            AntigenEntry(size_t a_index, const Antigen& antigen) : CoreEntry(a_index, antigen), passage(antigen.passage()) {}
            AntigenEntry& operator=(AntigenEntry&&) = default;

            std::string_view ag_sr() const
            {
                using namespace std::string_view_literals;
                return "AG"sv;
            }
            std::string full_name() const { return acmacs::string::join(acmacs::string::join_space, name, reassortant, acmacs::string::join(acmacs::string::join_space, annotations), passage); }
            size_t full_name_length() const { return name.size() + reassortant.size() + annotations.total_length() + passage.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size(); }
            bool operator<(const AntigenEntry& rhs) const { return compare(*this, rhs) < 0; }

            static inline int compare(const AntigenEntry& lhs, const AntigenEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
                    return np_c;
                return lhs.passage.compare(rhs.passage);
            }

            acmacs::virus::Passage passage;

        }; // class AntigenEntry

        struct SerumEntry : public CoreEntry
        {
            SerumEntry() = default;
            SerumEntry(SerumEntry&&) = default;
            SerumEntry(size_t a_index, const Serum& serum) : CoreEntry(a_index, serum), serum_id(serum.serum_id()), passage(serum.passage()) {}
            SerumEntry& operator=(SerumEntry&&) = default;

            std::string_view ag_sr() const
            {
                using namespace std::string_view_literals;
                return "SR"sv;
            }
            std::string full_name() const
            {
                return acmacs::string::join(acmacs::string::join_space, name, reassortant, acmacs::string::join(acmacs::string::join_space, annotations), serum_id, passage);
            }
            size_t full_name_length() const
            {
                return name.size() + reassortant.size() + annotations.total_length() + serum_id.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size() + passage.size() +
                       (passage.empty() ? 0 : 1);
            }
            bool operator<(const SerumEntry& rhs) const { return compare(*this, rhs) < 0; }

            static inline int compare(const SerumEntry& lhs, const SerumEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
                    return np_c;
                return lhs.serum_id.compare(rhs.serum_id);
            }

            SerumId serum_id;
            acmacs::virus::Passage passage;

        }; // class SerumEntry

        using antigen_selector_t = std::function<AntigenEntry(size_t, std::shared_ptr<Antigen>)>;
        using serum_selector_t = std::function<SerumEntry(size_t, std::shared_ptr<Serum>)>;

    } // namespace common

    // ----------------------------------------------------------------------

    class CommonAntigensSera
    {
      public:
        enum class match_level_t { strict, relaxed, ignored, automatic };

        CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level);
        CommonAntigensSera(const Chart& primary, const Chart& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor, match_level_t match_level);
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
        void antigens_only(); // remove sera from common lists
        void sera_only();     // remove antigens from common lists

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
        Indexes common_primary_sera() const; // returns serum indexes (NOT point indexes)!

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
