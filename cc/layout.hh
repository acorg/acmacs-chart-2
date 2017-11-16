#pragma once

#include "acmacs-base/vector.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    using Coordinates = Vector;

    class Layout
    {
     public:
        virtual ~Layout();

        virtual size_t number_of_points() const = 0;
        virtual size_t number_of_dimensions() const = 0;
        virtual Coordinates operator[](size_t aPointNo) const = 0;
        virtual double coordinate(size_t aPointNo, size_t aDimensionNo) const = 0;

    }; // class Layout

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
