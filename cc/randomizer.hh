#pragma once

#include <random>
#include <memory>

#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class LayoutRandomizer
    {
     public:
        LayoutRandomizer() : generator_(std::random_device()()) {}
        LayoutRandomizer(LayoutRandomizer&&) = default;
        virtual ~LayoutRandomizer() = default;

        virtual double get() = 0;

     protected:
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
        LayoutRandomizerPlain(LayoutRandomizerPlain&&) = default;

        double get() override { return distribution_(generator()); }
        void diameter(double diameter) { distribution_ = std::uniform_real_distribution<>(-diameter / 2, diameter / 2); }
        double diameter() const { return std::abs(distribution_.a() - distribution_.b()); }

     private:
        std::uniform_real_distribution<> distribution_;

    }; // class LayoutRandomizerPlain

// ----------------------------------------------------------------------

    class Chart;
    class Projection;
    template <typename Float> class Stress;

    std::shared_ptr<LayoutRandomizerPlain> randomizer_plain_with_table_max_distance(const Projection& projection);

      // makes randomizer with table max distance, generates random layout, performs very rough optimization,
      // resets randomization diameter with the resulting projection layout size
    std::shared_ptr<LayoutRandomizer> randomizer_plain_from_sample_optimization(const Chart& chart, const Stress<double>& stress, size_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, double diameter_multiplier);
    std::shared_ptr<LayoutRandomizer> randomizer_plain_from_sample_optimization(const Projection& projection, const Stress<double>& stress, double diameter_multiplier);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
