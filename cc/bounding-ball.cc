#include "acmacs-chart-2/bounding-ball.hh"

// ----------------------------------------------------------------------

void acmacs::BoundingBall::extend(const acmacs::PointCoordinates& aPoint)
{
    const auto distance2_to_center = distance2FromCenter(aPoint);
    if (distance2_to_center > radius2()) {
        const auto dist = std::sqrt(distance2_to_center);
        mDiameter = mDiameter * 0.5 + dist;
        const auto difference = dist - mDiameter * 0.5;
        auto p = aPoint.begin();
        for (auto c = mCenter.begin(); c != mCenter.end(); ++c, ++p)
            *c = (mDiameter * 0.5 * (*c) + difference * (*p)) / dist;
    }

} // acmacs::BoundingBall::extend

// ----------------------------------------------------------------------

void acmacs::BoundingBall::extend(const acmacs::BoundingBall& aBoundingBall)
{
    auto new_center = center();
    new_center += aBoundingBall.center();
    new_center *= 0.5;
    set(new_center, diameter() + distance(center(), aBoundingBall.center()));

} // acmacs::BoundingBall::extend

// ----------------------------------------------------------------------

acmacs::BoundingBall acmacs::minimum_bounding_ball(const Layout& aLayout)
{
    const auto area = aLayout.area();
    BoundingBall bb(area.min, area.max);
    for (size_t point_no = 0; point_no < aLayout.number_of_points(); ++point_no)
        bb.extend(aLayout[point_no]);
    return bb;

} // acmacs::minimum_bounding_ball

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
