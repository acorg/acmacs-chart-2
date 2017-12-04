#pragma once

#include "acmacs-base/stream.hh"
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
                return !empty() && std::all_of(begin(), end(), [](double value) -> bool { return ! std::isnan(value); });
            }

    }; // class Coordinates

    inline std::ostream& operator<<(std::ostream& s, const Coordinates& c)
    {
        stream_internal::write_to_stream(s, c, "[", "]", ", ");
        return s;
    }

// ----------------------------------------------------------------------

    class Layout
    {
     public:
        virtual ~Layout();

        virtual size_t number_of_points() const noexcept = 0;
        virtual size_t number_of_dimensions() const noexcept = 0;
        virtual Coordinates operator[](size_t aPointNo) const = 0;
        virtual double coordinate(size_t aPointNo, size_t aDimensionNo) const = 0;
        virtual void set(size_t aPointNo, const Coordinates& aCoordinates) = 0;

        Layout* transform(const Transformation& aTransformation) const;
        acmacs::BoundingBall* minimum_bounding_ball() const;
        void min_max_points(Indexes& aMin, Indexes& aMax) const;

        inline double distance(size_t p1, size_t p2, double no_distance = std::numeric_limits<double>::quiet_NaN()) const
        {
            auto c1 = operator[](p1);
            auto c2 = operator[](p2);
            return (c1.not_nan() && c2.not_nan()) ? c1.distance(c2) : no_distance;
        }

     private:
        void bounding_ball_extend(BoundingBall& aBoundingBall) const;

    }; // class Layout

    inline std::ostream& operator<<(std::ostream& s, const Layout& c)
    {
        s << "Layout [";
        for (size_t no = 0; no < c.number_of_points(); ++no)
            s << c[no] << ' ';
        s << ']';
        return s;
    }

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
            inline void set(size_t aPointNo, const Coordinates& aCoordinates) override { mData[aPointNo] = aCoordinates; }

            inline Coordinates& operator[](size_t aPointNo) { return mData[aPointNo]; }

         private:
            std::vector<Coordinates> mData;

        }; // class LayoutData

    } // namespace internal

} // namespace acmacs::chart

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
