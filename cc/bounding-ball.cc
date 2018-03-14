#include "acmacs-chart-2/bounding-ball.hh"

// ----------------------------------------------------------------------

void acmacs::BoundingBall::extend(const acmacs::Coordinates& aPoint)
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
    new_center += aBoundingBall.center();
    new_center *= 0.5;
    set(new_center, diameter() + center().distance(aBoundingBall.center()));

} // acmacs::BoundingBall::extend

// ----------------------------------------------------------------------

acmacs::BoundingBall* acmacs::minimum_bounding_ball(const LayoutInterface& aLayout)
{
    const auto area = aLayout.area();
    BoundingBall* bb = new BoundingBall(area.min, area.max);
    for (size_t point_no = 0; point_no < aLayout.number_of_points(); ++point_no)
        bb->extend(aLayout.get(point_no));
    return bb;

} // acmacs::minimum_bounding_ball

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
