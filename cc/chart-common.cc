#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class CommonAntigens
    {
     public:
        enum class match_level_t { strict, relaxed, ignored, automatic };
        enum class score_t : size_t { no_match = 0, passage_ignored = 1, egg = 2, without_date = 3, full_match = 4 };

        CommonAntigens(const Chart& primary, const Chart& secondary, match_level_t match_level);

        void report() const;

     private:
        struct AntigenEntry
        {
            AntigenEntry() = default;
            AntigenEntry(AntigenEntry&&) = default;
            AntigenEntry(size_t a_index, const Antigen& antigen)
                : index(a_index), name(antigen.name()), passage(antigen.passage()), reassortant(antigen.reassortant()), annotations(antigen.annotations()) {}
            AntigenEntry& operator=(AntigenEntry&&) = default;

            std::string full_name() const { return ::string::join(" ", {name, reassortant, ::string::join(" ", annotations), passage}); }
            bool operator<(const AntigenEntry& rhs) const { return compare(*this, rhs) < 0; }

            static int compare_without_passage(const AntigenEntry& lhs, const AntigenEntry& rhs)
                {
                    if (auto n_c = lhs.name.compare(rhs.name); n_c != 0)
                        return n_c;
                    if (auto r_c = lhs.reassortant.compare(rhs.reassortant); r_c != 0)
                        return r_c;
                    return string::compare(lhs.annotations.join(), rhs.annotations.join());
                }

            static int compare(const AntigenEntry& lhs, const AntigenEntry& rhs)
                {
                    if (auto np_c = compare_without_passage(lhs, rhs); np_c != 0)
                        return np_c;
                    return lhs.passage.compare(rhs.passage);
                }

            static bool less_without_passage(const AntigenEntry& lhs, const AntigenEntry& rhs)
                {
                    return compare_without_passage(lhs, rhs) < 0;
                }

            size_t index;
            Name name;
            Passage passage;
            Reassortant reassortant;
            Annotations annotations;

        }; // class AntigenEntry

        // class PrimaryEntry : public AntigenEntry
        // {
        //  public:
        //     using AntigenEntry::AntigenEntry;
        //     using AntigenEntry::operator<;


        // }; // class PrimaryEntry

        // class SecondaryEntry : public AntigenEntry
        // {
        //  public:
        //     using AntigenEntry::AntigenEntry;


        // }; // class SecondaryEntry

        // using score_t = size_t;
        // constexpr static const score_t NO_MATCH = 0, PASSAGE_IGNORED = 1, EGG = 2, WITHOUT_DATE = 3, FULL_MATCH = 4;

        struct MatchEntry
        {
            size_t primary_index;
            size_t secondary_index;
            score_t score;
            bool use = false;

        }; // class MatchEntry

        std::vector<AntigenEntry> primary_;
        std::vector<AntigenEntry> secondary_;
        std::vector<MatchEntry> match_;
        size_t number_of_common_ = 0;
        const size_t max_antigens_;   // for report formatting
        const size_t min_antigens_;   // for match_level_t::automatic threshold

        score_t match(const AntigenEntry& primary, const AntigenEntry& secondary, match_level_t match_level) const;
        void match(match_level_t match_level);

    }; // class CommonAntigens

} // namespace acmacs::chart

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigens::CommonAntigens(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary, match_level_t match_level)
    : primary_(primary.number_of_antigens()), secondary_(secondary.number_of_antigens()),
      max_antigens_{std::max(primary_.size(), secondary_.size())}, min_antigens_{std::min(primary_.size(), secondary_.size())}
{
    for (size_t index = 0; index < primary.number_of_antigens(); ++index)
        primary_[index] = AntigenEntry(index, *(*primary.antigens())[index]);
    std::sort(primary_.begin(), primary_.end());

    // for (const auto& pp: primary_)
    //     std::cout << pp.index << ' ' << pp.name << "  ::  " << pp.reassortant << "  ::  " << pp.annotations << ' ' << pp.passage << '\n';

    for (size_t index = 0; index < secondary.number_of_antigens(); ++index)
        secondary_[index] = AntigenEntry(index, *(*secondary.antigens())[index]);

    match(match_level);

} // acmacs::chart::CommonAntigens::CommonAntigens

// ----------------------------------------------------------------------

void acmacs::chart::CommonAntigens::match(match_level_t match_level)
{
    for (const auto& secondary: secondary_) {
        const auto [first, last] = std::equal_range(primary_.begin(), primary_.end(), secondary, AntigenEntry::less_without_passage);
        for (auto p_e = first; p_e != last; ++p_e) {
            if (const auto score = match(*p_e, secondary, match_level); score != score_t::no_match)
                  //match_.emplace_back(p_e->index, secondary.index, score);
                match_.push_back({p_e->index, secondary.index, score});
        }
    }
    std::sort(match_.begin(), match_.end(), [](const auto& a, const auto& b) { return a.score == b.score ? a.primary_index < b.primary_index : a.score > b.score; });

    std::set<size_t> primary_used, secondary_used;
    auto use_entry = [this,&primary_used,&secondary_used](auto& match_entry) {
        if (primary_used.find(match_entry.primary_index) == primary_used.end() && secondary_used.find(match_entry.secondary_index) == secondary_used.end()) {
            match_entry.use = true;
            primary_used.insert(match_entry.primary_index);
            secondary_used.insert(match_entry.secondary_index);
            ++this->number_of_common_;
        }
    };

    const size_t automatic_level_threshold = std::max(3UL, min_antigens_ / 10);
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
              if (match_entry.score >= score_t::passage_ignored)
                  use_entry(match_entry);
              else
                  stop = true;
              break;
          case match_level_t::automatic:
              if (previous_score == match_entry.score || number_of_common_ < automatic_level_threshold) {
                  // std::cout << match_entry.primary_index << ' ' << match_entry.secondary_index << ' ' << static_cast<size_t>(match_entry.score) << ' ' << static_cast<size_t>(previous_score) << number_of_common_ << ' ' << automatic_level_threshold << '\n';
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

} // acmacs::chart::CommonAntigens::match

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigens::score_t acmacs::chart::CommonAntigens::match(const AntigenEntry& primary, const AntigenEntry& secondary, match_level_t match_level) const
{
    score_t result{score_t::no_match};
    if (primary.name == secondary.name && primary.reassortant == secondary.reassortant && primary.annotations == secondary.annotations && !primary.annotations.distinct()) {
        switch (match_level) {
          case match_level_t::ignored:
              result = score_t::passage_ignored;
              break;
          case match_level_t::strict:
          case match_level_t::relaxed:
          case match_level_t::automatic:
              if (primary.passage.empty() || secondary.passage.empty()) {
                  if (primary.passage == secondary.passage && !primary.reassortant.empty() && !secondary.reassortant.empty()) // reassortant assumes egg passage
                      result = score_t::egg;
                  else
                      result = score_t::passage_ignored;
              }
              else if (primary.passage == secondary.passage)
                  result = score_t::full_match;
              else if (primary.passage.without_date() == secondary.passage.without_date())
                  result = score_t::without_date;
              else if (primary.passage.is_egg() == secondary.passage.is_egg())
                  result = score_t::egg;
              else
                  result = score_t::passage_ignored;
              break;
        }
    }
    return result;

} // acmacs::chart::CommonAntigens::match

// ----------------------------------------------------------------------

void acmacs::chart::CommonAntigens::report() const
{
    auto& stream = std::cout;
    const auto num_digits = static_cast<int>(std::log10(max_antigens_)) + 1;
    const char* const score_names[] = {"no-match", "no-passage", "egg", "no-date", "full"};

    stream << "common: " << number_of_common_ << '\n';
    auto find_primary = [this](size_t index) -> const AntigenEntry& { return *std::find_if(this->primary_.begin(), this->primary_.end(), [index](const auto& element) { return element.index == index; }); };
    for (const auto& m: match_) {
        if (m.use)
            stream << std::setw(10) << std::left << score_names[static_cast<size_t>(m.score)]
                   << " [" << std::setw(num_digits) << std::right << m.primary_index << ' ' << std::setw(70) << std::left << find_primary(m.primary_index).full_name()
                   << "] [" << std::setw(num_digits) << std::right << m.secondary_index << ' ' << std::setw(70) << std::left << secondary_[m.secondary_index].full_name() << "]\n";
    }

} // acmacs::chart::CommonAntigens::report

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--match", "auto", "match level: \"strict\", \"relaxed\", \"ignored\", \"auto\""},
                {"--time", false, "test speed"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 2) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file> <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            auto match_level{acmacs::chart::CommonAntigens::match_level_t::automatic};
            if (const std::string match_level_s = args["--match"].str(); !match_level_s.empty()) {
                switch (match_level_s[0]) {
                  case 's': match_level = acmacs::chart::CommonAntigens::match_level_t::strict; break;
                  case 'r': match_level = acmacs::chart::CommonAntigens::match_level_t::relaxed; break;
                  case 'i': match_level = acmacs::chart::CommonAntigens::match_level_t::ignored; break;
                  case 'a': match_level = acmacs::chart::CommonAntigens::match_level_t::automatic; break;
                  default:
                      std::cerr << "Unrecognized --match argument, automatic assumed" << '\n';
                      break;
                }
            }
            auto chart1 = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto chart2 = acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report);
            acmacs::chart::CommonAntigens common(*chart1, *chart2, match_level);
            common.report();
        }
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
