#include <iostream>

#include "acmacs-base/argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/string.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<bool> show_lab{*this, "lab", desc{"just show lab"}};
    option<bool> show_virus_type{*this, "virus-type", desc{"just show virus type and lineage"}};
    option<bool> show_assay{*this, "assay", desc{"just show assay"}};
    option<bool> show_number_of_antigens{*this, "antigens", desc{"just show number of antigens"}};
    option<bool> show_number_of_sera{*this, "sera", desc{"just show number of sera"}};
    option<bool> column_bases{*this, "column-bases"};
    option<bool> list_tables{*this, "list-tables"};
    option<bool> homologous{*this, "homologous", desc{"report homologous antigens for sera"}};
    option<bool> verify{*this, "verify"};
    option<bool> report_time{*this, "time", desc{"report time of loading chart"}};

    argument<str_array> charts{*this, arg_name{"chart-file"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const auto report = do_report_time(opt.report_time);
        size_t total_number_of_antigens = 0;
        for (size_t file_no = 0; file_no < opt.charts->size(); ++file_no) {
            auto chart = acmacs::chart::import_from_file((*opt.charts)[file_no], opt.verify ? acmacs::chart::Verify::All : acmacs::chart::Verify::None, report);
            auto info = chart->info();

            std::vector<std::string> fields;
            if (opt.show_lab)
                fields.emplace_back(info->lab(acmacs::chart::Info::Compute::Yes));
            if (opt.show_virus_type) {
                const auto virus = info->virus(acmacs::chart::Info::Compute::Yes);
                if (::string::lower(*virus) == "influenza") {
                    const auto virus_type = info->virus_type(acmacs::chart::Info::Compute::Yes);
                    if (virus_type == acmacs::chart::VirusType{"B"}) {
                        if (const auto lineage = chart->lineage(); !lineage.empty())
                            fields.push_back(string::concat(virus_type, '/', string::capitalize(lineage.substr(0, 3))));
                        else
                            fields.emplace_back(virus_type);
                    }
                    else if (!virus_type.empty())
                        fields.emplace_back(virus_type);
                }
                else {
                    fields.push_back(*virus);
                    if (const auto virus_type = info->virus_type(acmacs::chart::Info::Compute::Yes); !virus_type.empty())
                        fields.emplace_back(virus_type);
                }
            }
            if (opt.show_assay) {
                if (const auto assay = info->assay(acmacs::chart::Info::Compute::Yes); !assay.empty())
                    fields.emplace_back(assay);
            }
            if (opt.show_number_of_antigens) {
                const auto na = chart->number_of_antigens();
                fields.push_back(std::to_string(na));
                total_number_of_antigens += na;
            }
            if (opt.show_number_of_sera)
                fields.push_back(std::to_string(chart->number_of_sera()));
            if (opt.homologous) {
                chart->set_homologous(acmacs::chart::find_homologous::all, nullptr, acmacs::debug::yes);
                auto antigens = chart->antigens();
                auto sera = chart->sera();
                const auto num_digits = static_cast<int>(std::log10(sera->size())) + 1;
                for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
                    std::cout << "SR " << std::setw(num_digits) << sr_no << ' ' << serum->full_name() << " -- " << serum->homologous_antigens() << '\n';
                }
                fields.emplace_back();
            }

            if (!fields.empty()) {
                std::cout << string::join(" ", fields) << '\n';
            }
            else {
                std::cout << chart->make_info() << '\n';
                if (const auto having_too_few_numeric_titers = chart->titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers.empty())
                    std::cout << "Points having too few numeric titers: " << having_too_few_numeric_titers.size() << ' ' << having_too_few_numeric_titers << '\n';
                if (opt.column_bases) {
                    // Timeit ti("column bases computing ");
                    auto cb = chart->computed_column_bases(acmacs::chart::MinimumColumnBasis{});
                    fmt::print("computed column bases: {}\n", *cb);
                    for (auto projection_no : acmacs::range(chart->number_of_projections())) {
                        if (auto fcb = chart->projection(projection_no)->forced_column_bases(); fcb)
                            fmt::print("forced column bases for projection {}: {}\n", projection_no, *fcb);
                    }
                }
                if (opt.list_tables && info->number_of_sources() > 0) {
                    std::cout << "\nTables:\n";
                    for (auto src_no : acmacs::range(info->number_of_sources()))
                        std::cout << std::setw(3) << src_no << ' ' << info->source(src_no)->make_name() << '\n';
                }
            }
            if (file_no < (opt.charts->size() - 1))
                std::cout << '\n';
        }
        if (total_number_of_antigens > 0)
            std::cerr << "Total number of antigens: " << total_number_of_antigens << '\n';
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
