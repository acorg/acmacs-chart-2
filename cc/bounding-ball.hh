#pragma once

#include "acmacs-base/log.hh"
#include "acmacs-base/stream.hh"
#include "acmacs-base/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class BoundingBall
    {
     public:
        inline BoundingBall() = default;
          // aP1 and aP2 are opposite corners of some area, draw a minimal circle through them
        inline BoundingBall(const Coordinates& aP1, const Coordinates& aP2)
            : mCenter(aP1.size()), mDiameter(0)
            {
                Vector v{aP1};
                v.subtract(aP2);
                mDiameter = v.eucledian_norm();
                for (Vector::size_type dim = 0; dim < aP1.size(); ++dim)
                    mCenter[dim] = (aP1[dim] + aP2[dim]) * 0.5;
            }

        inline const Vector& center() const { return mCenter; }
        inline double diameter() const { return mDiameter; }

        inline void set(const Vector& aCenter, double aDiameter)
            {
                mCenter = aCenter;
                mDiameter = aDiameter;
            }

          // The same as *this=BoundingBall(aP1, aP2, aDistance) but avoids assignment
        inline void move(const Vector& aP1, const Vector& aP2, double aDistance)
            {
                mDiameter = aDistance;
                mCenter.resize(aP1.size());
                for (Vector::size_type dim = 0; dim < aP1.size(); ++dim)
                    mCenter[dim] = (aP1[dim] + aP2[dim]) * 0.5;
            }

        inline void moveCenter(const Vector& aBy)
            {
                mCenter.add(aBy);
            }

          // Extends bounding ball (change center and diameter) to make sure the bounding ball includes the passed point
        void extend(const Coordinates& aPoint);

          // Extends bounding ball to make sure it includes all points of the second boundig ball
        void extend(const BoundingBall& aBoundingBall);

        inline double top() const { return mCenter[1] - mDiameter / 2.0; }
        inline double bottom() const { return mCenter[1] + mDiameter / 2.0; }
        inline double left() const { return mCenter[0] - mDiameter / 2.0; }
        inline double right() const { return mCenter[0] + mDiameter / 2.0; }

     private:
        Vector mCenter;
        double mDiameter;

          // Returns distance^2 from the ball center to point
        inline double distance2FromCenter(const Coordinates& aPoint) const
            {
                Vector v(aPoint);
                v.subtract(mCenter);
                return v.inner_product();
            }

        inline double radius2() const
            {
                return mDiameter * mDiameter * 0.25;
            }

    }; // class BoundingBall

    BoundingBall* minimum_bounding_ball(const LayoutInterface& aLayout);

} // namespace acmacs

// ----------------------------------------------------------------------

inline std::ostream& operator << (std::ostream& out, const acmacs::BoundingBall& a) { return out << "BoundingBall(" << a.center() << ", " << a.diameter() << ")"; }

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
