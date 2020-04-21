#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// converts projection to fewer number of dimensions without relaxation

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t>    target_number_of_dimensions{*this, 'd',  dflt{2ul}, desc{"target number of dimensions, if 0 - apply full pca"}};
    option<size_t>    number_of_projections_to_convert{*this, 'p',  dflt{10ul}, desc{"number of the best projections to convert (0 - to conver all)"}};

    argument<str> input_chart{*this, arg_name{"input-chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        const acmacs::number_of_dimensions_t target_number_of_dimensions{*opt.target_number_of_dimensions};
        acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(opt.input_chart)};
        auto projections = chart.projections_modify();
        if (projections->size() == 0)
            throw std::runtime_error{"chart has no projections"};
        projections->remove_except(*opt.number_of_projections_to_convert);
        const acmacs::chart::optimization_options opt_opt;
        for (size_t p_no = 0; p_no < projections->size(); ++p_no) {
            auto projection = projections->at(p_no);
            auto stress = acmacs::chart::stress_factory(*projection, opt_opt.mult);
            auto layout = projection->layout_modified();
            if (*target_number_of_dimensions == 0) {
                // full pca
                const auto initial_stress = projection->stress();
                acmacs::chart::pca(stress, projection->number_of_dimensions(), layout->data(), layout->data() + layout->size());
                const auto resulting_stress = projection->calculate_stress(stress);
                AD_DEBUG("initial stress: {} resulting stress: {} diff:{}", initial_stress, resulting_stress, std::abs(initial_stress - resulting_stress));
                projection->transformation_reset();
            }
            else if (projection->number_of_dimensions() > target_number_of_dimensions) {
                acmacs::chart::dimension_annealing(opt_opt.method, stress, projection->number_of_dimensions(), target_number_of_dimensions, layout->data(), layout->data() + layout->size());
                layout->change_number_of_dimensions(target_number_of_dimensions);
                stress.change_number_of_dimensions(target_number_of_dimensions);
                const auto resulting_stress = projection->calculate_stress(stress);
                AD_DEBUG("stress: {}", resulting_stress);
                projection->transformation_reset();
            }
            else {
                AD_WARNING("projection {} has too few dimensions: {}", p_no, projection->number_of_dimensions());
            }
        }
        if (opt.output_chart)
            acmacs::chart::export_factory(chart, opt.output_chart, opt.program_name());

    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
