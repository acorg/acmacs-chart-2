#pragma once

#include <random>
#include <memory>
#include <algorithm>

#include "acmacs-base/line.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class LayoutRandomizer
    {
     public:
        LayoutRandomizer() : generator_(std::random_device()()) {}
        // LayoutRandomizer(LayoutRandomizer&&) = default;
        virtual ~LayoutRandomizer() = default;

        virtual PointCoordinates get(size_t number_of_dimensions)
            {
                PointCoordinates result(number_of_dimensions);
                std::generate(result.begin(), result.end(), [this]() { return this->get(); });
                return result;
            }

     protected:
        virtual double get() = 0;
        auto& generator() { return generator_; }

     private:
        // std::random_device rd_;
        std::mt19937 generator_;

    }; // class LayoutRandomizer

// ----------------------------------------------------------------------

    class LayoutRandomizerPlain : public LayoutRandomizer
    {
     public:
        LayoutRandomizerPlain(double diameter) : distribution_(-diameter / 2, diameter / 2) {}
          // LayoutRandomizerPlain(LayoutRandomizerPlain&&) = default;

        void diameter(double diameter) { distribution_ = std::uniform_real_distribution<>(-diameter / 2, diameter / 2); }
        double diameter() const { return std::abs(distribution_.a() - distribution_.b()); }

        using LayoutRandomizer::get;

     protected:
        double get() override { return distribution_(generator()); }

     private:
        std::uniform_real_distribution<> distribution_;

    }; // class LayoutRandomizerPlain

// ----------------------------------------------------------------------

      // random values are placed at the same side of a line (for map degradation resolver)
    class LayoutRandomizerWithLineBorder : public LayoutRandomizerPlain
    {
     public:
        LayoutRandomizerWithLineBorder(double diameter, const LineSide& line_side) : LayoutRandomizerPlain(diameter), line_side_(line_side) {}

        PointCoordinates get(size_t number_of_dimensions) override { return line().fix(LayoutRandomizerPlain::get(number_of_dimensions)); }

        LineSide& line() { return line_side_; }
        const LineSide& line() const { return line_side_; }

     protected:
        using LayoutRandomizerPlain::get;

     private:
        LineSide line_side_;

    }; // class LayoutRandomizerPlain

// ----------------------------------------------------------------------

    class Chart;
    class Projection;
    class ProjectionModify;
    class Stress;

    std::shared_ptr<LayoutRandomizerPlain> randomizer_plain_with_table_max_distance(const Projection& projection);

      // makes randomizer with table max distance, generates random layout, performs very rough optimization,
      // resets randomization diameter with the resulting projection layout size
    std::shared_ptr<LayoutRandomizer> randomizer_plain_from_sample_optimization(const Chart& chart, const Stress& stress, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, double diameter_multiplier);
    std::shared_ptr<LayoutRandomizer> randomizer_plain_from_sample_optimization(const Projection& projection, const Stress& stress, double diameter_multiplier);

    std::shared_ptr<LayoutRandomizer> randomizer_plain_with_current_layout_area(const ProjectionModify& projection, double diameter_multiplier);
    std::shared_ptr<LayoutRandomizer> randomizer_border_with_current_layout_area(const ProjectionModify& projection, double diameter_multiplier, const LineSide& line_side);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
