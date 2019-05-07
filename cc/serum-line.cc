#include "acmacs-base/range.hh"
#include "acmacs-base/statistics.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-chart-2/serum-line.hh"

// ----------------------------------------------------------------------

acmacs::chart::SerumLine::SerumLine(const Projection& projection)
{
    auto layout = projection.layout();
    if (layout->number_of_dimensions() != number_of_dimensions_t{2})
        throw std::runtime_error("invalid number of dimensions in projection: " + acmacs::to_string(layout->number_of_dimensions()) + ", only 2 is supported");

    const auto number_of_antigens = projection.chart().number_of_antigens();
    line_ = acmacs::statistics::simple_linear_regression(layout->begin_sera_dimension(number_of_antigens, number_of_dimensions_t{0}), layout->end_sera_dimension(number_of_antigens, number_of_dimensions_t{0}), layout->begin_sera_dimension(number_of_antigens, number_of_dimensions_t{1}));

    std::vector<double> distances;
    std::transform(layout->begin_sera(number_of_antigens), layout->end_sera(number_of_antigens), std::back_inserter(distances),
                   [this](const auto& coord) { return this->line_.distance_to(coord); });
    standard_deviation_ = acmacs::statistics::standard_deviation(distances.begin(), distances.end()).sd();

} // acmacs::chart::SerumLine::SerumLine

// ----------------------------------------------------------------------

acmacs::chart::SerumLine::AntigensRelativeToLine acmacs::chart::SerumLine::antigens_relative_to_line(const Projection& projection) const
{
    acmacs::chart::SerumLine::AntigensRelativeToLine result;
    auto layout = projection.layout();
    for (auto antigen_no : acmacs::range(projection.chart().number_of_antigens())) {
        const auto distance = line().distance_with_direction(layout->get(antigen_no));
        if (distance < 0)
            result.negative.insert(antigen_no);
        else
            result.positive.insert(antigen_no);
    }
    return result;

} // acmacs::chart::SerumLine::antigens_relative_to_line

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
