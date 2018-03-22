#pragma once

#include <random>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class LayoutRandomizer
    {
     public:
        LayoutRandomizer() : generator_(std::random_device()()) {}
        LayoutRandomizer(LayoutRandomizer&&) = default;
        virtual ~LayoutRandomizer() = default;

        virtual double operator()() = 0;

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
        LayoutRandomizerPlain(double radius) : distribution_(-radius, radius) {}
        LayoutRandomizerPlain(LayoutRandomizerPlain&&) = default;

        double operator()() override { return distribution_(generator()); }
        void radius(double radius) { distribution_ = std::uniform_real_distribution<>(-radius, radius); }

     private:
        std::uniform_real_distribution<> distribution_;

    }; // class LayoutRandomizerPlain

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
