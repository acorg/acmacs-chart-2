#pragma once

#include <random>
#include <memory>
#include <algorithm>
#include <optional>

#include "acmacs-base/line.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class mt19937_2002
    {
      public:
        mt19937_2002(unsigned long seed) { init(seed); }

        unsigned long operator()();

      private:
        constexpr static size_t N{624};
        std::array<unsigned long, N> mt_;
        size_t mti_{N + 1};

        void init(unsigned long seed);
    };

    class LayoutRandomizer
    {
     public:
        using seed_t = std::optional<std::uint_fast32_t>;

        LayoutRandomizer(seed_t seed = std::nullopt) : generator_(seed ? *seed : std::random_device{}()) {}
        // LayoutRandomizer(LayoutRandomizer&&) = default;
        virtual ~LayoutRandomizer() = default;

        virtual PointCoordinates get(number_of_dimensions_t number_of_dimensions)
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
        // mt19937_2002 generator_;

    }; // class LayoutRandomizer

// ----------------------------------------------------------------------

    class LayoutRandomizerPlain : public LayoutRandomizer
    {
     public:
        LayoutRandomizerPlain(double diameter, seed_t seed = std::nullopt)
            : LayoutRandomizer(seed), diameter_{diameter}, distribution_(-diameter / 2, diameter / 2) {}
          // LayoutRandomizerPlain(LayoutRandomizerPlain&&) = default;

        void diameter(double diameter) { diameter_ = diameter; distribution_ = std::uniform_real_distribution<>(-diameter / 2, diameter / 2); }
        double diameter() const { return diameter_; } // std::abs(distribution_.a() - distribution_.b()); }

        using LayoutRandomizer::get;

     protected:
        double get() override {
            std::lock_guard<std::mutex> guard(generator_access_);
            return distribution_(generator());
        }
        // double get() override {  // c2
        //     std::lock_guard<std::mutex> guard(generator_access_);
        //     return (static_cast<double>(generator()()) * (1.0 / 4294967296.0) - 0.5) * diameter_;
        // }

      private:
        double diameter_;
        std::mutex generator_access_;
        std::uniform_real_distribution<> distribution_;

    }; // class LayoutRandomizerPlain

// ----------------------------------------------------------------------

      // random values are placed at the same side of a line (for map degradation resolver)
    class LayoutRandomizerWithLineBorder : public LayoutRandomizerPlain
    {
     public:
        LayoutRandomizerWithLineBorder(double diameter, const LineSide& line_side, seed_t seed = std::nullopt)
            : LayoutRandomizerPlain(diameter, seed), line_side_(line_side) {}

        PointCoordinates get(number_of_dimensions_t number_of_dimensions) override { return line().fix(LayoutRandomizerPlain::get(number_of_dimensions)); }

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

    std::shared_ptr<LayoutRandomizerPlain> randomizer_plain_with_table_max_distance(const Projection& projection, LayoutRandomizer::seed_t seed = std::nullopt);

      // makes randomizer with table max distance, generates random layout, performs very rough optimization,
      // resets randomization diameter with the resulting projection layout size
    std::shared_ptr<LayoutRandomizer> randomizer_plain_from_sample_optimization(const Chart& chart, const Stress& stress, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, double diameter_multiplier, LayoutRandomizer::seed_t seed = std::nullopt);
    std::shared_ptr<LayoutRandomizer> randomizer_plain_from_sample_optimization(const Projection& projection, const Stress& stress, double diameter_multiplier, LayoutRandomizer::seed_t seed = std::nullopt);

    std::shared_ptr<LayoutRandomizer> randomizer_plain_with_current_layout_area(const ProjectionModify& projection, double diameter_multiplier, LayoutRandomizer::seed_t seed = std::nullopt);
    std::shared_ptr<LayoutRandomizer> randomizer_border_with_current_layout_area(const ProjectionModify& projection, double diameter_multiplier, const LineSide& line_side, LayoutRandomizer::seed_t seed = std::nullopt);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
