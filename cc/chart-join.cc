#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

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
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 3) {
            std::cerr << "if two charts have the same set of antigens and sera and the same titers, concatenate projections and sort them\n"
                      << "Usage: " << args.program() << " [options] <chart1> <chart2> <output-chart>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart1{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            acmacs::chart::ChartModify chart2{acmacs::chart::import_from_file(args[1], acmacs::chart::Verify::None, report)};

            if (chart1.number_of_antigens() != chart2.number_of_antigens())
                throw std::runtime_error{"charts have differenet number of antigens"};
            if (chart1.number_of_sera() != chart2.number_of_sera())
                throw std::runtime_error{"charts have differenet number of sera"};
            if (std::any_of(acmacs::index_iterator(0UL), acmacs::index_iterator(chart1.number_of_antigens()), [&chart1,&chart2](size_t ag_no) { return *chart1.antigen(ag_no) != *chart2.antigen(ag_no); }))
                throw std::runtime_error{"charts have differenet sets of antigens"};
            if (std::any_of(acmacs::index_iterator(0UL), acmacs::index_iterator(chart1.number_of_sera()), [&chart1,&chart2](size_t sr_no) { return *chart1.serum(sr_no) != *chart2.serum(sr_no); }))
                throw std::runtime_error{"charts have differenet sets of sera"};

            auto titers1 = chart1.titers(), titers2 = chart2.titers();
            if (titers1->number_of_layers() != titers2->number_of_layers())
                throw std::runtime_error{"titers of charts have differenet sets of layers"};
            const auto tim1 = titers1->titers_existing(), tim2 = titers2->titers_existing();
            for (auto ti1 = tim1.begin(), ti2 = tim2.begin(); ti1 != tim1.end() && ti2 != tim2.end(); ++ti1, ++ti2) {
                if (*ti1 != *ti2)
                    throw std::runtime_error{"titers of charts are differenet"};
            }

            auto& projections1 = chart1.projections_modify();
            auto& projections2 = chart2.projections_modify();
            for (size_t projection_no = 0; projection_no < projections2.size(); ++projection_no)
                projections1.new_by_cloning(*projections2.at(projection_no));
            projections1.sort();
            std::cout << chart1.make_info() << '\n';

            acmacs::chart::export_factory(chart1, args[2], args.program(), report);
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
