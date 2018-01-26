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
        class PrimaryEntry
        {
         public:
            PrimaryEntry() = default;
            PrimaryEntry(size_t index, const Antigen& antigen)
                : index_(index), name_(antigen.name()), passage_(antigen.passage()), reassortant_(antigen.reassortant()), annotations_(antigen.annotations()) {}
            PrimaryEntry& operator=(PrimaryEntry&&) = default;

         private:
            size_t index_;
            Name name_;
            Passage passage_;
            Reassortant reassortant_;
            Annotations annotations_;

        }; // class PrimaryEntry

        CommonAntigens(const Chart& primary, const Chart& secondary);

     private:
        std::vector<PrimaryEntry> primary_;

    }; // class CommonAntigens

} // namespace acmacs::chart

acmacs::chart::CommonAntigens::CommonAntigens(const acmacs::chart::Chart& primary, const acmacs::chart::Chart& secondary)
    : primary_(primary.number_of_antigens())
{
    for (size_t index = 0; index < primary.number_of_antigens(); ++index)
        primary_[index] = PrimaryEntry(index, *(*primary.antigens())[index]);

} // acmacs::chart::CommonAntigens::CommonAntigens

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
