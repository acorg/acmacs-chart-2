#include "acmacs-chart-2/layout.hh"

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


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
