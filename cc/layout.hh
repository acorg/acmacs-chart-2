#pragma once

#include "acmacs-base/transformation.hh"
#include "acmacs-base/vector.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class BoundingBall;

} // namespace acmacs

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    using Indexes = std::vector<size_t>;

// ----------------------------------------------------------------------

    class Coordinates : public Vector
    {
     public:
        using Vector::Vector;

        inline Coordinates transform(const Transformation& aTransformation) const
            {
                const auto [x, y] = aTransformation.transform(operator[](0), operator[](1));
                return {x, y};
            }

        inline bool not_nan() const
            {
                return std::all_of(begin(), end(), [](double value) -> bool { return ! std::isnan(value); });
            }

    }; // class Coordinates

// ----------------------------------------------------------------------

    class Layout
    {
     public:
        virtual ~Layout();

        virtual size_t number_of_points() const noexcept = 0;
        virtual size_t number_of_dimensions() const noexcept = 0;
        virtual Coordinates operator[](size_t aPointNo) const = 0;
        virtual double coordinate(size_t aPointNo, size_t aDimensionNo) const = 0;

        Layout* transform(const Transformation& aTransformation) const;
        acmacs::BoundingBall* minimum_bounding_ball() const;
        void min_max_points(Indexes& aMin, Indexes& aMax) const;

     private:
        void bounding_ball_extend(BoundingBall& aBoundingBall) const;

    }; // class Layout

// ----------------------------------------------------------------------

    namespace internal
    {
        class LayoutData : public Layout
        {
         public:
            inline LayoutData(size_t aNumberOfPoints) : mData(aNumberOfPoints) {}

            inline size_t number_of_points() const noexcept override { return mData.size(); }
            inline size_t number_of_dimensions() const noexcept override { return mData.empty() ? 0 : mData[0].size(); }
            inline Coordinates operator[](size_t aPointNo) const override { return mData[aPointNo]; }
            inline double coordinate(size_t aPointNo, size_t aDimensionNo) const override { return mData[aPointNo][aDimensionNo]; }

            inline Coordinates& operator[](size_t aPointNo) { return mData[aPointNo]; }

         private:
            std::vector<Coordinates> mData;

        }; // class LayoutData

    } // namespace internal

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: