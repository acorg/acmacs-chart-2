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
    argument<str> chart{*this, arg_name{"chart"}};
    argument<str> output_prefix{*this, arg_name{"output_prefix"}};
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
        const auto layer_no_width{static_cast<int>(std::log10(titers->number_of_layers() - 1)) + 1};
        for (size_t layer_no{0}; layer_no < titers->number_of_layers(); ++layer_no) {
            auto [antigen_indexes, serum_indexes] = titers->antigens_sera_of_layer(layer_no);
            acmacs::chart::ChartNew output{antigen_indexes.size(), serum_indexes.size()};
            if (info->number_of_sources() > layer_no)
                output.info_modify()->replace_with(*info->source(layer_no));
            auto output_antigens = output.antigens_modify();
            for (size_t ind{0}; ind < antigen_indexes.size(); ++ind)
                output_antigens->at(ind).replace_with(*antigens->at(antigen_indexes[ind]));
            auto output_sera = output.sera_modify();
            for (size_t ind{0}; ind < serum_indexes.size(); ++ind)
                output_sera->at(ind).replace_with(*sera->at(serum_indexes[ind]));
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
