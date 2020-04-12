#include "acmacs-base/string.hh"
#include "acmacs-chart-2/randomizer.hh"
#include "acmacs-chart-2/chart-modify.hh"

static std::shared_ptr<acmacs::chart::LayoutRandomizer> randomizer_plain_from_sample_optimization_internal(acmacs::chart::ProjectionModifyNew&& projection, const acmacs::chart::Stress& stress,
                                                                                                           double diameter_multiplier, acmacs::chart::LayoutRandomizer::seed_t seed);

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::LayoutRandomizerPlain> acmacs::chart::randomizer_plain_with_table_max_distance(const Projection& projection, LayoutRandomizer::seed_t seed)
{
    auto cb = projection.forced_column_bases();
    if (!cb)
        cb = projection.chart().column_bases(projection.minimum_column_basis());
    const auto max_distance = projection.chart().titers()->max_distance(*cb);
    // std::cerr << "max_distance: " << max_distance << '\n';
    return std::make_shared<LayoutRandomizerPlain>(max_distance, seed);

} // acmacs::chart::randomizer_plain_with_table_max_distance

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::LayoutRandomizer> acmacs::chart::randomizer_plain_with_current_layout_area(const ProjectionModify& projection, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    const auto mm = projection.layout_modified()->minmax();
    auto sq = [](double v) { return v*v; };
    const auto diameter = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); }));
    return std::make_shared<LayoutRandomizerPlain>(diameter * diameter_multiplier, seed);

} // acmacs::chart::randomizer_plain_with_current_layout_area

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::LayoutRandomizer> acmacs::chart::randomizer_border_with_current_layout_area(const ProjectionModify& projection, double diameter_multiplier, const LineSide& line_side, LayoutRandomizer::seed_t seed)
{
    const auto mm = projection.layout_modified()->minmax();
    auto sq = [](double v) { return v*v; };
    const auto diameter = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); }));
    return std::make_shared<LayoutRandomizerWithLineBorder>(diameter * diameter_multiplier, line_side, seed);

} // acmacs::chart::randomizer_border_with_current_layout_area

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::LayoutRandomizer> randomizer_plain_from_sample_optimization_internal(acmacs::chart::ProjectionModifyNew&& projection, const acmacs::chart::Stress& stress,
                                                                                                    double diameter_multiplier, acmacs::chart::LayoutRandomizer::seed_t seed)
{
    auto rnd = randomizer_plain_with_table_max_distance(projection, seed);
    projection.randomize_layout(rnd);
    acmacs::chart::optimize(acmacs::chart::optimization_method::alglib_cg_pca, stress, projection.layout_modified()->data(), projection.layout_modified()->size(),
                            acmacs::chart::optimization_precision::very_rough);
    auto sq = [](double v) { return v * v; };
    const auto mm = projection.layout_modified()->minmax();
    const auto diameter = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); }));
    if (std::isnan(diameter) || float_zero(diameter)) {
        // std::cerr << "WARNING: randomizer_plain_from_sample_optimization_internal: diameter is " << diameter << '\n';
        throw std::runtime_error(acmacs::string::concat("randomizer_plain_from_sample_optimization_internal: diameter is ", diameter));
    }
    rnd->diameter(diameter * diameter_multiplier);
    return rnd;

} // randomizer_plain_from_sample_optimization_internal

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::LayoutRandomizer> acmacs::chart::randomizer_plain_from_sample_optimization(const Projection& projection, const Stress& stress, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    return randomizer_plain_from_sample_optimization_internal(ProjectionModifyNew(projection.chart(), projection.number_of_dimensions(), projection.minimum_column_basis()), stress, diameter_multiplier, seed);

} // acmacs::chart::randomizer_plain_from_sample_optimization

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::chart::LayoutRandomizer> acmacs::chart::randomizer_plain_from_sample_optimization(const Chart& chart, const Stress& stress, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    return randomizer_plain_from_sample_optimization_internal(ProjectionModifyNew(chart, number_of_dimensions, minimum_column_basis), stress, diameter_multiplier, seed);

} // acmacs::chart::randomizer_plain_from_sample_optimization

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
