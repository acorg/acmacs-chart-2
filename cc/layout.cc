#include "acmacs-chart-2/layout.hh"
#include "acmacs-chart-2/bounding-ball.hh"

// ----------------------------------------------------------------------

acmacs::chart::Layout::~Layout()
{
}

// ----------------------------------------------------------------------

class TransformedLayout : public acmacs::chart::internal::LayoutData
{
 public:
    inline TransformedLayout(const acmacs::chart::Layout& aSource, const acmacs::Transformation& aTransformation)
        : acmacs::chart::internal::LayoutData(aSource.number_of_points())
        {
            for (size_t p_no = 0; p_no < aSource.number_of_points(); ++p_no)
                operator[](p_no) = aSource[p_no].transform(aTransformation);
        }

}; // class TransformedLayout

// ----------------------------------------------------------------------

acmacs::chart::Layout* acmacs::chart::Layout::transform(const acmacs::Transformation& aTransformation) const
{
    return new TransformedLayout(*this, aTransformation);

} // acmacs::chart::Layout::transform

// ----------------------------------------------------------------------

acmacs::BoundingBall* acmacs::chart::Layout::minimum_bounding_ball() const
{
    const size_t nd = number_of_dimensions();
    Indexes min(nd), max(nd);
    min_max_points(min, max);

    Coordinates min_point(nd), max_point(nd);
    for (size_t dim = 0; dim < nd; ++dim) {
        min_point[dim] = coordinate(min[dim], dim);
        max_point[dim] = coordinate(max[dim], dim);
    }

    BoundingBall* bb = new BoundingBall(min_point, max_point);
    bounding_ball_extend(*bb);
    return bb;

} // acmacs::chart::Layout::minimum_bounding_ball

// ----------------------------------------------------------------------

void acmacs::chart::Layout::min_max_points(Indexes& aMin, Indexes& aMax) const
{
    constexpr const size_t none = static_cast<size_t>(-1);
    std::fill(aMin.begin(), aMin.end(), none);
    std::fill(aMax.begin(), aMax.end(), none);
    for (size_t point_no = 0; point_no < number_of_points(); ++point_no) {
        const auto point = operator[](point_no);
        for (size_t dim = 0; dim < point.size(); ++dim) {
            const auto v = point[dim];
            if (std::isnan(v)) {
                  // Move aMin and aMax, if they refer to a point with NaN coordinates
                if (aMin[dim] == point_no)
                    aMin[dim] = point_no + 1;
                if (aMax[dim] == point_no)
                    aMax[dim] = point_no + 1;
            }
            else {
                if (aMin[dim] == none || v < coordinate(aMin[dim], dim))
                    aMin[dim] = point_no;
                if (aMax[dim] == none || v > coordinate(aMax[dim], dim))
                    aMax[dim] = point_no;
            }
        }
    }

} // acmacs::chart::Layout::min_max_points

// ----------------------------------------------------------------------

void acmacs::chart::Layout::bounding_ball_extend(BoundingBall& aBoundingBall) const
{
    for (size_t point_no = 0; point_no < number_of_points(); ++point_no)
        aBoundingBall.extend(operator[](point_no));
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
