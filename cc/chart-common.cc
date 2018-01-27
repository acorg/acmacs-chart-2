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
        struct AntigenEntry
        {
            AntigenEntry() = default;
            AntigenEntry(AntigenEntry&&) = default;
            AntigenEntry(size_t a_index, const Antigen& antigen)
                : index(a_index), name(antigen.name()), passage(antigen.passage()), reassortant(antigen.reassortant()), annotations(antigen.annotations()) {}
            AntigenEntry& operator=(AntigenEntry&&) = default;

            bool operator<(const AntigenEntry& rhs) const
                {
                    if (auto n_c = name.compare(rhs.name); n_c != 0)
                        return n_c;
                    if (auto r_c = reassortant.compare(rhs.reassortant); r_c != 0)
                        return r_c;
                    if (auto a_c = string::compare(annotations.join(), rhs.annotations.join()); a_c != 0)
                        return a_c;
                    if (auto p_c = passage.compare(rhs.passage); p_c != 0)
                        return p_c;
                    return false;
                }

            size_t index;
            Name name;
            Passage passage;
            Reassortant reassortant;
            Annotations annotations;

        }; // class Antigen

        class PrimaryEntry : public AntigenEntry
        {
         public:
            using AntigenEntry::AntigenEntry;
            using AntigenEntry::operator<;


        }; // class PrimaryEntry

        class SecondaryEntry : public AntigenEntry
        {
         public:
            using AntigenEntry::AntigenEntry;


        }; // class SecondaryEntry

        CommonAntigens(const Chart& primary, const Chart& secondary);


     private:
        std::vector<PrimaryEntry> primary_;
        std::vector<SecondaryEntry> secondary_;

        using score_t = size_t;
        enum class match_level_t { strict, relaxed, ignored, automatic };
        score_t match(const AntigenEntry& primary, const AntigenEntry& secondary, match_level_t match_level) const;

    }; // class CommonAntigens

} // namespace acmacs::chart

acmacs::chart::CommonAntigens::CommonAntigens(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary)
    : primary_(primary.number_of_antigens()), secondary_(secondary.number_of_antigens())
{
    for (size_t index = 0; index < primary.number_of_antigens(); ++index)
        primary_[index] = PrimaryEntry(index, *(*primary.antigens())[index]);
    std::sort(primary_.begin(), primary_.end());

    for (size_t index = 0; index < secondary.number_of_antigens(); ++index)
        secondary_[index] = SecondaryEntry(index, *(*secondary.antigens())[index]);

} // acmacs::chart::CommonAntigens::CommonAntigens

// ----------------------------------------------------------------------

acmacs::chart::CommonAntigens::score_t acmacs::chart::CommonAntigens::match(const AntigenEntry& primary, const AntigenEntry& secondary, match_level_t match_level) const
{
    constexpr const score_t NO_MATCH = 0, PASSAGE_IGNORED = 1, EGG = 2, WITHOUT_DATE = 3, FULL_MATCH = 4;
    score_t result{NO_MATCH};
    if (primary.name == secondary.name && primary.reassortant == secondary.reassortant && primary.annotations == secondary.annotations && !primary.annotations.distinct()) {
        switch (match_level) {
          case match_level_t::ignored:
              result = PASSAGE_IGNORED;
              break;
          case match_level_t::strict:
          case match_level_t::relaxed:
          case match_level_t::automatic:
              if (primary.passage.empty() || secondary.passage.empty()) {
                  if (primary.passage == secondary.passage && !primary.reassortant.empty() && !secondary.reassortant.empty()) // reassortant assumes egg passage
                      result = EGG;
                  else
                      result = PASSAGE_IGNORED;
              }
              else if (primary.passage == secondary.passage)
                  result = FULL_MATCH;
              else if (primary.passage.without_date() == secondary.passage.without_date())
                  result = WITHOUT_DATE;
              else if (primary.passage.is_egg() == secondary.passage.is_egg())
                  result = EGG;
              else
                  result = PASSAGE_IGNORED;
              break;
        }
    }
    return result;

} // acmacs::chart::CommonAntigens::match

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
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
            auto chart1 = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto chart2 = acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report);
            acmacs::chart::CommonAntigens(*chart1, *chart2);
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
