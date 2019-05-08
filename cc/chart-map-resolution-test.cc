#include "acmacs-base/argv.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/map-resolution-test.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str>    number_of_dimensions{*this, 'd', dflt{"1,2,3,4,5"}, desc{"number of dimensions, comma separated values"}};
    option<size_t> number_of_optimizations{*this, 'n', dflt{100UL}, desc{"number of optimisations"}};
    option<size_t> number_of_random_replicates_for_each_proportion{*this, "random-replicates-for-each-proportion", dflt{25UL}};
    option<str>    proportions_to_dont_care{*this, "proportions-to-dont-care", dflt{"0.1, 0.2, 0.3"}, desc{"comma separated values"}};
    option<str>    minimum_column_basis{*this, 'm', dflt{"none"}, desc{"minimum column basis"}};
    option<bool>   fine_optimisation{*this, "fine-optimisation"};
    option<bool>   no_column_bases_from_master{*this, "no-column-bases-from-master", desc{"converting titers to dont-care may change column bases, do not force master chart column bases"}};
    option<bool>   relax_from_full_table{*this, "relax-from-full-table", desc{"additional projection in each replicate, first full table is relaxed, then titers dont-cared and the best projection relaxed again from already found starting coordinates."}};

    argument<str> source{*this, arg_name{"chart-to-test"}, mandatory};
};

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        acmacs::chart::map_resolution_test_data::Parameters parameters;
        parameters.number_of_dimensions = acmacs::string::split_into_uint<acmacs::number_of_dimensions_t>(*opt.number_of_dimensions, ",");
        parameters.number_of_optimizations = acmacs::chart::number_of_optimizations_t{opt.number_of_optimizations};
        parameters.number_of_random_replicates_for_each_proportion = opt.number_of_random_replicates_for_each_proportion;
        parameters.proportions_to_dont_care = acmacs::string::split_into_double(*opt.proportions_to_dont_care, ",");
        parameters.minimum_column_basis = *opt.minimum_column_basis;
        parameters.column_bases_from_master = *opt.no_column_bases_from_master ? acmacs::chart::map_resolution_test_data::column_bases_from_master::no : acmacs::chart::map_resolution_test_data::column_bases_from_master::yes;
        parameters.optimization_precision = *opt.fine_optimisation ? acmacs::chart::optimization_precision::fine : acmacs::chart::optimization_precision::rough;
        parameters.relax_from_full_table = *opt.relax_from_full_table ? acmacs::chart::map_resolution_test_data::relax_from_full_table::yes : acmacs::chart::map_resolution_test_data::relax_from_full_table::no;

        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.source, acmacs::chart::Verify::None, report_time::no)};
        const auto results = acmacs::chart::map_resolution_test(chart, parameters);

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
