#include "acmacs-base/string.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/common.hh"
#include "acmacs-chart-2/chart.hh"

using namespace acmacs::chart;

// ----------------------------------------------------------------------

class CommonAntigensSera::Impl
{
 public:
    Impl(const Chart& primary, const Chart& secondary, match_level_t match_level, acmacs::debug dbg);
    Impl(const Chart& primary);

    struct CoreEntry
    {
        CoreEntry() = default;
        CoreEntry(CoreEntry&&) = default;
        template <typename AgSr> CoreEntry(size_t a_index, const AgSr& ag_sr)
            : index(a_index), name(ag_sr.name()), reassortant(ag_sr.reassortant()), annotations(ag_sr.annotations()) {}
        CoreEntry& operator=(CoreEntry&&) = default;

        static int compare(const CoreEntry& lhs, const CoreEntry& rhs)
            {
                if (auto n_c = lhs.name.compare(rhs.name); n_c != 0)
                    return n_c;
                if (auto r_c = lhs.reassortant.compare(rhs.reassortant); r_c != 0)
                    return r_c;
                return ::string::compare(lhs.annotations.join(), rhs.annotations.join());
            }

        static bool less(const CoreEntry& lhs, const CoreEntry& rhs) { return compare(lhs, rhs) < 0; }

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

        std::string full_name() const { return acmacs::string::join(acmacs::string::join_space, name, reassortant, acmacs::string::join(acmacs::string::join_space, annotations), passage); }
        size_t full_name_length() const { return name.size() + reassortant.size() + annotations.total_length() + passage.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size(); }
        bool operator<(const AntigenEntry& rhs) const { return compare(*this, rhs) < 0; }

        static int compare(const AntigenEntry& lhs, const AntigenEntry& rhs)
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

        std::string full_name() const { return acmacs::string::join(acmacs::string::join_space, name, reassortant, acmacs::string::join(acmacs::string::join_space, annotations), serum_id, passage); }
        size_t full_name_length() const { return name.size() + reassortant.size() + annotations.total_length() + serum_id.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size() + passage.size() + (passage.empty() ? 0 : 1); }
        bool operator<(const SerumEntry& rhs) const { return compare(*this, rhs) < 0; }

        static int compare(const SerumEntry& lhs, const SerumEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
                    return np_c;
                return lhs.serum_id.compare(rhs.serum_id);
            }

        SerumId serum_id;
        acmacs::virus::Passage passage;

    }; // class SerumEntry

    enum class score_t : size_t { no_match = 0, passage_serum_id_ignored = 1, egg = 2, without_date = 3, full_match = 4 };

    struct MatchEntry
    {
        size_t primary_index;
        size_t secondary_index;
        score_t score;
        bool use{false};

    }; // class MatchEntry

    template <typename AgSrEntry> class ChartData
    {
     public:
        ChartData(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level, acmacs::debug dbg);
        ChartData(const acmacs::chart::Chart& primary);
        std::string report(size_t indent, std::string_view prefix, std::string_view ignored_key, verbose vrb) const;
        std::vector<CommonAntigensSera::common_t> common() const;
        std::optional<size_t> primary_by_secondary(size_t secondary_no) const;
        std::optional<size_t> secondary_by_primary(size_t primary_no) const;
        void clear() { number_of_common_ = 0; match_.clear(); }

        void keep_only(const PointIndexList& indexes)
        {
            number_of_common_ = 0;
            for (auto& en : match_) {
                if (en.use) {
                    if (!indexes.contains(en.primary_index))
                        en.use = false;
                    else
                        ++number_of_common_;
                }
            }
        }

        std::vector<AgSrEntry> primary_;
        std::vector<AgSrEntry> secondary_;
        std::vector<MatchEntry> match_;
        size_t number_of_common_ = 0;
        const size_t primary_base_, secondary_base_; // 0 for antigens, number_of_antigens for sera
        const size_t min_number_ = 0;   // for match_level_t::automatic threshold

     private:
        template <typename AgSr> static void make(std::vector<AgSrEntry>& target, const AgSr& source)
            {
                for (size_t index = 0; index < target.size(); ++index) {
                    target[index] = AgSrEntry(index, *source[index]);
                }
            }

        void match(match_level_t match_level, acmacs::debug dbg = acmacs::debug::no);
        score_t match(const AgSrEntry& primary, const AgSrEntry& secondary, match_level_t match_level, acmacs::debug dbg = acmacs::debug::no) const;
        score_t match_not_ignored(const AgSrEntry& primary, const AgSrEntry& secondary) const;

    }; // class ChartData<AgSrEntry>

    ChartData<AntigenEntry> antigens_;
    ChartData<SerumEntry> sera_;

}; // class Impl

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level, acmacs::debug dbg)
    : primary_(primary.number_of_antigens()), secondary_(secondary.number_of_antigens()),
      primary_base_{0}, secondary_base_{0},
      min_number_{std::min(primary_.size(), secondary_.size())}
{
    make(primary_, *primary.antigens());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *secondary.antigens());
    match(match_level, dbg);

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData(const acmacs::chart::Chart& primary)
    : primary_(primary.number_of_antigens()), secondary_(primary.number_of_antigens()), match_(primary.number_of_antigens()), primary_base_{0}, secondary_base_{0}
{
    make(primary_, *primary.antigens());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *primary.antigens());
    for (auto antigen_no : acmacs::range(match_.size())) {
        match_[antigen_no].primary_index = match_[antigen_no].secondary_index = antigen_no;
        match_[antigen_no].score = score_t::full_match;
        match_[antigen_no].use = true;
    }
    number_of_common_ = match_.size();

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level, acmacs::debug dbg)
    : primary_(primary.number_of_sera()), secondary_(secondary.number_of_sera()),
      primary_base_{primary.number_of_antigens()}, secondary_base_{secondary.number_of_antigens()},
      min_number_{std::min(primary_.size(), secondary_.size())}
{
    make(primary_, *primary.sera());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *secondary.sera());
    match(match_level, dbg);

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData(const acmacs::chart::Chart& primary)
    : primary_(primary.number_of_sera()), secondary_(primary.number_of_sera()),
      match_(primary.number_of_sera()), primary_base_{primary.number_of_antigens()}, secondary_base_{primary.number_of_antigens()}
{
    make(primary_, *primary.sera());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *primary.sera());
    for (auto serum_no : acmacs::range(match_.size())) {
        match_[serum_no].primary_index = match_[serum_no].secondary_index = serum_no;
        match_[serum_no].score = score_t::full_match;
        match_[serum_no].use = true;
    }
    number_of_common_ = match_.size();

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData

// ----------------------------------------------------------------------

template <typename AgSrEntry> void CommonAntigensSera::Impl::ChartData<AgSrEntry>::match(CommonAntigensSera::match_level_t match_level, acmacs::debug dbg)
{
    for (const auto& secondary: secondary_) {
        const auto [first, last] = std::equal_range(primary_.begin(), primary_.end(), secondary, AgSrEntry::less);
        for (auto p_e = first; p_e != last; ++p_e) {
            if (const auto score = match(*p_e, secondary, match_level, dbg); score != score_t::no_match)
                  //match_.emplace_back(p_e->index, secondary.index, score);
                match_.push_back({p_e->index, secondary.index, score});
        }
    }
    auto order = [](const auto& a, const auto& b) {
                     if (a.score != b.score)
                         return a.score > b.score;
                     else if (a.primary_index != b.primary_index)
                         return a.primary_index < b.primary_index;
                     return a.secondary_index < b.secondary_index;
                 };
    std::sort(match_.begin(), match_.end(), order);

    std::set<size_t> primary_used, secondary_used;
    auto use_entry = [this,&primary_used,&secondary_used](auto& match_entry) {
        if (primary_used.find(match_entry.primary_index) == primary_used.end() && secondary_used.find(match_entry.secondary_index) == secondary_used.end()) {
            match_entry.use = true;
            primary_used.insert(match_entry.primary_index);
            secondary_used.insert(match_entry.secondary_index);
            ++this->number_of_common_;
        }
    };

    const size_t automatic_level_threshold = std::max(3UL, min_number_ / 10);
    score_t previous_score{score_t::no_match};
    bool stop = false;
    for (auto& match_entry: match_) {
        switch (match_level) {
          case match_level_t::strict:
              if (match_entry.score == score_t::full_match)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::relaxed:
              if (match_entry.score >= score_t::egg)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::ignored:
              if (match_entry.score >= score_t::passage_serum_id_ignored)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::automatic:
              if (previous_score == match_entry.score || number_of_common_ < automatic_level_threshold) {
                  use_entry(match_entry);
                  previous_score = match_entry.score;
              }
              else
                  stop = true;
              break;
        }
        if (stop)
            break;
    }

} // CommonAntigensSera::Impl::ChartData::match

// ----------------------------------------------------------------------

template <typename AgSrEntry>
CommonAntigensSera::Impl::score_t CommonAntigensSera::Impl::ChartData<AgSrEntry>::match(const AgSrEntry& primary, const AgSrEntry& secondary,
                                                                                        acmacs::chart::CommonAntigensSera::match_level_t match_level, acmacs::debug dbg) const
{
    score_t result{score_t::no_match};
    if (dbg == acmacs::debug::yes) {
        fmt::print(stderr, "DEBUG: name \"{}\"  \"{}\" --> {}\n", primary.name, secondary.name, primary.name == secondary.name);
        fmt::print(stderr, "DEBUG: reassortant \"{}\"  \"{}\" --> {}\n", primary.reassortant, secondary.reassortant, primary.reassortant == secondary.reassortant);
        fmt::print(stderr, "DEBUG: annotations {}  {} --> {}\n", primary.annotations, secondary.annotations, primary.annotations == secondary.annotations);
        fmt::print(stderr, "DEBUG: distinct {} {}\n\n", primary.annotations.distinct(), secondary.annotations.distinct());
    }
    if (primary.name == secondary.name && primary.reassortant == secondary.reassortant && primary.annotations == secondary.annotations && !primary.annotations.distinct()) {
        switch (match_level) {
            case match_level_t::ignored:
                result = score_t::passage_serum_id_ignored;
                break;
            case match_level_t::strict:
            case match_level_t::relaxed:
            case match_level_t::automatic:
                result = match_not_ignored(primary, secondary);
                break;
        }
    }
    return result;

} // CommonAntigensSera::Impl::ChartData::match

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::score_t CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::match_not_ignored(const AntigenEntry& primary, const AntigenEntry& secondary) const
{
    auto result = score_t::passage_serum_id_ignored;
    if (primary.passage.empty() || secondary.passage.empty()) {
        if (primary.passage == secondary.passage && !primary.reassortant.empty() && !secondary.reassortant.empty()) // reassortant assumes egg passage
            result = score_t::egg;
    }
    else if (primary.passage == secondary.passage)
        result = score_t::full_match;
    else if (primary.passage.without_date() == secondary.passage.without_date())
        result = score_t::without_date;
    else if (primary.passage.is_egg() == secondary.passage.is_egg())
        result = score_t::egg;
    return result;

} // CommonAntigensSera::Impl::ChartData<AntigenEntry>::match_not_ignored

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::score_t CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::match_not_ignored(const SerumEntry& primary, const SerumEntry& secondary) const
{
    auto result = score_t::passage_serum_id_ignored;
    if (primary.serum_id == secondary.serum_id && !primary.serum_id.empty())
        result = score_t::full_match;
    else if (!primary.passage.empty() && !secondary.passage.empty() && primary.passage.is_egg() == secondary.passage.is_egg())
        result = score_t::egg;
    return result;

} // CommonAntigensSera::Impl::ChartData<AntigenEntry>::match_not_ignored

// ----------------------------------------------------------------------

template <typename AgSrEntry> std::string CommonAntigensSera::Impl::ChartData<AgSrEntry>::report(size_t indent, std::string_view prefix, std::string_view ignored_key, verbose vrb) const
{
    using namespace std::string_view_literals;
    const std::array score_names{"no-match"sv, ignored_key, "egg"sv, "no-date"sv, "full"sv};
    const auto score_names_max = std::accumulate(std::begin(score_names), std::end(score_names), 0ul, [](size_t mx, std::string_view nam) { return std::max(mx, nam.size()); });

    fmt::memory_buffer output;
    if (number_of_common_) {
        auto find_primary = [this](size_t index) -> const auto&
        {
            return *std::find_if(this->primary_.begin(), this->primary_.end(), [index](const auto& element) { return element.index == index; });
        };
        size_t primary_name_size = 0,
               // secondary_name_size = 0,
            max_number_primary = 0, max_number_secondary = 0;
        for (const auto& m : match_) {
            if (m.use) {
                primary_name_size = std::max(primary_name_size, find_primary(m.primary_index).full_name_length());
                // secondary_name_size = std::max(secondary_name_size, secondary_[m.secondary_index].full_name_length());
                max_number_primary = std::max(max_number_primary, m.primary_index);
                max_number_secondary = std::max(max_number_secondary, m.secondary_index);
            }
        }
        const auto num_digits_primary = max_number_primary ? static_cast<int>(std::log10(max_number_primary)) + 1 : 1;
        const auto num_digits_secondary = max_number_secondary ? static_cast<int>(std::log10(max_number_secondary)) + 1 : 1;
        // AD_DEBUG("report 1 score_names_max:{} num_digits_primary:{} max_number_primary:{} primary_name_size:{} max_number_secondary:{} num_digits_secondary:{}", score_names_max, num_digits_primary, max_number_primary, primary_name_size, max_number_secondary, num_digits_secondary);

        fmt::format_to(output, "{:{}c}common {}: {} (total primary: {} secondary: {})\n", ' ', indent, prefix, number_of_common_, primary_.size(), secondary_.size());
        std::vector<size_t> common_in_primary, common_in_secondary;
        for (const auto& m : match_) {
            if (m.use) {
                fmt::format_to(output, "{:{}c}{:<{}s} {:{}d} {:<{}s} | {:{}d} {}\n", ' ', indent, score_names[static_cast<size_t>(m.score)], score_names_max, m.primary_index, num_digits_primary,
                               find_primary(m.primary_index).full_name(), primary_name_size, m.secondary_index, num_digits_secondary, secondary_[m.secondary_index].full_name());
                common_in_primary.push_back(m.primary_index);
                common_in_secondary.push_back(m.secondary_index);
            }
        }
        if (vrb == verbose::yes) {
            fmt::format_to(output, "{:{}}common in primary {}: {}\n", ' ', indent, prefix, common_in_primary);
            fmt::format_to(output, "{:{}}common in secondary {}: {}\n", ' ', indent, prefix, common_in_secondary);
            std::vector<size_t> unique_in_primary(primary_.size() - common_in_primary.size()), unique_in_secondary(secondary_.size() - common_in_secondary.size());
            std::copy_if(acmacs::index_iterator(0UL), acmacs::index_iterator(primary_.size()), unique_in_primary.begin(),
                         [&common_in_primary](size_t index) { return std::find(std::begin(common_in_primary), std::end(common_in_primary), index) == std::end(common_in_primary); });
            std::copy_if(acmacs::index_iterator(0UL), acmacs::index_iterator(secondary_.size()), unique_in_secondary.begin(),
                         [&common_in_secondary](size_t index) { return std::find(std::begin(common_in_secondary), std::end(common_in_secondary), index) == std::end(common_in_secondary); });
            fmt::format_to(output, "{:{}}unique in primary {}: {}\n", ' ', indent, prefix, unique_in_primary);
            fmt::format_to(output, "{:{}}unique in secondary {}: {}\n", ' ', indent, prefix, unique_in_secondary);
        }
    }
    else {
        fmt::format_to(output, "{:{}}no common {}\n", ' ', indent, prefix);
    }
    return fmt::to_string(output);

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::report

// ----------------------------------------------------------------------

template <typename AgSrEntry> inline std::vector<CommonAntigensSera::common_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::common() const
{
    std::vector<CommonAntigensSera::common_t> result;
    for (const auto& m: match_) {
        if (m.use)
            result.emplace_back(m.primary_index, m.secondary_index);
    }
    return result;

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::common

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#if __GNUC__ == 8
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
// g++-8.2 issues strange warning about return {} statement in two template functions below
#endif

template <typename AgSrEntry> inline std::optional<size_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::primary_by_secondary(size_t secondary_no) const
{
    if (const auto found = std::find_if(match_.begin(), match_.end(), [secondary_no](const auto& match) { return match.use && match.secondary_index == secondary_no; }); found != match_.end())
        return found->primary_index;
    else
        return std::nullopt;

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::primary_by_secondary

// ----------------------------------------------------------------------

template <typename AgSrEntry> inline std::optional<size_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::secondary_by_primary(size_t primary_no) const
{
    if (const auto found = std::find_if(match_.begin(), match_.end(), [primary_no](const auto& match) { return match.use && match.primary_index == primary_no; }); found != match_.end())
        return found->secondary_index;
    else
        return std::nullopt;

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::secondary_by_primary

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

inline CommonAntigensSera::Impl::Impl(const Chart& primary, const Chart& secondary, match_level_t match_level, acmacs::debug dbg)
    : antigens_(primary, secondary, match_level, dbg), sera_(primary, secondary, match_level, dbg)
{
}

// ----------------------------------------------------------------------

inline CommonAntigensSera::Impl::Impl(const Chart& primary)
    : antigens_(primary), sera_(primary)
{
}

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigensSera::CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level, acmacs::debug dbg)
    : impl_(&primary == &secondary ? std::make_unique<Impl>(primary) : std::make_unique<Impl>(primary, secondary, match_level, dbg))
{
}

// ----------------------------------------------------------------------

// for procrustes between projections of the same chart
acmacs::chart::CommonAntigensSera::CommonAntigensSera(const Chart& primary)
    : impl_(std::make_unique<Impl>(primary))
{

} // acmacs::chart::CommonAntigensSera::CommonAntigensSera

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigensSera::CommonAntigensSera(CommonAntigensSera&&) = default;
acmacs::chart::CommonAntigensSera& acmacs::chart::CommonAntigensSera::operator=(CommonAntigensSera&&) = default;

// must be here to allow std::unique_ptr<Impl> with incomplete Impl in .hh
acmacs::chart::CommonAntigensSera::~CommonAntigensSera()
{
}

// ----------------------------------------------------------------------

std::string acmacs::chart::CommonAntigensSera::report(size_t indent, verbose vrb) const
{
    using namespace std::string_view_literals;
    return fmt::format("{}\n{}", impl_->antigens_.report(indent, "antigens"sv, "no-passage"sv, vrb), impl_->sera_.report(indent, "sera"sv, "no-serum-id"sv, vrb));

} // CommonAntigensSera::report

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigensSera::operator bool() const
{
    return impl_->antigens_.number_of_common_ > 0 || impl_->sera_.number_of_common_ > 0;

} // bool

// ----------------------------------------------------------------------

void acmacs::chart::CommonAntigensSera::keep_only(const PointIndexList& antigens, const PointIndexList& sera)
{
    if (antigens.empty())
        impl_->antigens_.clear();
    else
        impl_->antigens_.keep_only(antigens);
    if (sera.empty())
        impl_->sera_.clear();
    else
        impl_->sera_.keep_only(sera);

} // acmacs::chart::CommonAntigensSera::keep_only

// ----------------------------------------------------------------------

void acmacs::chart::CommonAntigensSera::antigens_only()   // remove sera from common lists
{
    impl_->sera_.clear();

} // acmacs::chart::CommonAntigensSera::antigens_only

// ----------------------------------------------------------------------

void acmacs::chart::CommonAntigensSera::sera_only()   // remove antigens from common lists
{
    impl_->antigens_.clear();

} // acmacs::chart::CommonAntigensSera::sera_only

// ----------------------------------------------------------------------

size_t acmacs::chart::CommonAntigensSera::common_antigens() const
{
    return impl_->antigens_.number_of_common_;

} // acmacs::chart::CommonAntigensSera::common_antigens

// ----------------------------------------------------------------------

size_t acmacs::chart::CommonAntigensSera::common_sera() const
{
    return impl_->sera_.number_of_common_;

} // acmacs::chart::CommonAntigensSera::common_sera

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::antigens() const
{
    return impl_->antigens_.common();

} // CommonAntigensSera::antigens

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::sera() const
{
    return impl_->sera_.common();

} // CommonAntigensSera::sera

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::sera_as_point_indexes() const
{
    auto result = impl_->sera_.common();
    std::transform(result.begin(), result.end(), result.begin(), [primary_base_=impl_->sera_.primary_base_,secondary_base=impl_->sera_.secondary_base_](const auto& entry) -> common_t { return {entry.primary + primary_base_, entry.secondary + secondary_base}; });
    return result;

} // acmacs::chart::CommonAntigensSera::sera_as_point_indexes

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::points(subset a_subset) const
{
    switch (a_subset) {
      case subset::all:
          return points();
      case subset::antigens:
          return impl_->antigens_.common();
      case subset::sera:
          return sera_as_point_indexes();
    }
    return {};

} // acmacs::chart::CommonAntigensSera::points

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::points() const
{
    auto result = impl_->antigens_.common();
    for (auto [primary, secondary]: impl_->sera_.common())
        result.emplace_back(primary + impl_->sera_.primary_base_, secondary + impl_->sera_.secondary_base_);
    return result;

} // CommonAntigensSera::points

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::points_for_primary_antigens(const Indexes& antigen_indexes) const
{
    auto result = impl_->antigens_.common();
    result.erase(std::remove_if(std::begin(result), std::end(result), [&antigen_indexes](const auto& entry) { return !antigen_indexes.contains(entry.primary); }), std::end(result));
    return result;

} // acmacs::chart::CommonAntigensSera::points_for_primary_antigens

// ----------------------------------------------------------------------

std::vector<acmacs::chart::CommonAntigensSera::common_t> acmacs::chart::CommonAntigensSera::points_for_primary_sera(const Indexes& serum_indexes) const
{
    auto result = impl_->sera_.common();
    result.erase(std::remove_if(std::begin(result), std::end(result), [&serum_indexes](const auto& entry) { return !serum_indexes.contains(entry.primary); }), std::end(result));
    std::transform(result.begin(), result.end(), result.begin(), [primary_base_=impl_->sera_.primary_base_,secondary_base=impl_->sera_.secondary_base_](const auto& entry) -> common_t { return {entry.primary + primary_base_, entry.secondary + secondary_base}; });
    return result;

} // acmacs::chart::CommonAntigensSera::points_for_primary_sera

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::CommonAntigensSera::antigen_primary_by_secondary(size_t secondary_no) const
{
    return impl_->antigens_.primary_by_secondary(secondary_no);

} // acmacs::chart::CommonAntigensSera::antigen_primary_by_secondary

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::CommonAntigensSera::antigen_secondary_by_primary(size_t primary_no) const
{
    return impl_->antigens_.secondary_by_primary(primary_no);

} // acmacs::chart::CommonAntigensSera::antigen_secondary_by_primary

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::CommonAntigensSera::serum_primary_by_secondary(size_t secondary_no) const
{
    return impl_->sera_.primary_by_secondary(secondary_no);

} // acmacs::chart::CommonAntigensSera::serum_primary_by_secondary

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::chart::CommonAntigensSera::serum_secondary_by_primary(size_t primary_no) const
{
    return impl_->sera_.secondary_by_primary(primary_no);

} // acmacs::chart::CommonAntigensSera::serum_secondary_by_primary

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigensSera::match_level_t acmacs::chart::CommonAntigensSera::match_level(std::string_view source)
{
    auto match_level{match_level_t::automatic};
    if (!source.empty()) {
        switch (source[0]) {
            case 's':
                match_level = match_level_t::strict;
                break;
            case 'r':
                match_level = match_level_t::relaxed;
                break;
            case 'i':
                match_level = match_level_t::ignored;
                break;
            case 'a':
                match_level = match_level_t::automatic;
                break;
            default:
              AD_WARNING(fmt::format("unrecognized match level: \"{}\", automatic assumed", source));
              break;
        }
    }
    return match_level;

} // acmacs::chart::CommonAntigensSera::match_level

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
