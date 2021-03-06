#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "generate individual tables from the layers of the chart"; }
    argument<str> chart{*this, arg_name{"chart"}, mandatory};
    argument<str> output_prefix{*this, arg_name{"output_prefix"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.chart);
        auto titers = chart->titers();
        if (titers->number_of_layers() < 2)
            throw std::runtime_error("chart has no layers");
        auto info = chart->info();
        auto antigens = chart->antigens();
        auto sera = chart->sera();
        const auto layer_no_width{chart->number_of_digits_for_antigen_serum_index_formatting()};
        for (size_t layer_no{0}; layer_no < titers->number_of_layers(); ++layer_no) {
            auto [antigen_indexes, serum_indexes] = titers->antigens_sera_of_layer(layer_no);
            acmacs::chart::ChartNew output{antigen_indexes.size(), serum_indexes.size()};
            AD_INFO("Layer {}  antigens {}  sera {}", layer_no, antigen_indexes, serum_indexes);
            if (info->number_of_sources() > layer_no)
                output.info_modify().replace_with(*info->source(layer_no));
            auto& output_antigens = output.antigens_modify();
            for (size_t ag_ind{0}; ag_ind < antigen_indexes.size(); ++ag_ind)
                output_antigens.at(ag_ind).replace_with(*antigens->at(antigen_indexes[ag_ind]));
            auto& output_sera = output.sera_modify();
            for (size_t sr_ind{0}; sr_ind < serum_indexes.size(); ++sr_ind)
                output_sera.at(sr_ind).replace_with(*sera->at(serum_indexes[sr_ind]));
            auto& output_titers = output.titers_modify();
            AD_DEBUG("Layer {}", layer_no);
            for (size_t ag_ind{0}; ag_ind < antigen_indexes.size(); ++ag_ind) {
                for (size_t sr_ind{0}; sr_ind < serum_indexes.size(); ++sr_ind)
                    output_titers.titer(ag_ind, sr_ind, titers->titer_of_layer(layer_no, antigen_indexes[ag_ind], serum_indexes[sr_ind]));
            }
            const auto output_filename{fmt::format("{}.{:0{}d}.ace", opt.output_prefix, layer_no, layer_no_width)};
            AD_INFO("Generating {}: {}", output_filename, output.description());
            acmacs::chart::export_factory(output, output_filename, opt.program_name());
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
