#include "acmacs-chart-2/bounding-ball.hh"

// ----------------------------------------------------------------------

void acmacs::BoundingBall::extend(const acmacs::chart::Coordinates& aPoint)
{
    const double distance2_to_center = distance2FromCenter(aPoint);
    if (distance2_to_center > radius2()) {
        const double dist = std::sqrt(distance2_to_center);
        mDiameter = mDiameter * 0.5 + dist;
        const double difference = dist - mDiameter * 0.5;
        Vector::const_iterator p = aPoint.begin();
        for (Vector::iterator c = mCenter.begin(); c != mCenter.end(); ++c, ++p)
            *c = (mDiameter * 0.5 * (*c) + difference * (*p)) / dist;
    }

} // acmacs::BoundingBall::extend

// ----------------------------------------------------------------------

void acmacs::BoundingBall::extend(const acmacs::BoundingBall& aBoundingBall)
{
    Vector new_center(center());
    new_center.add(aBoundingBall.center());
    new_center.multiply_by(0.5);
    set(new_center, diameter() + center().distance(aBoundingBall.center()));

} // acmacs::BoundingBall::extend

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
