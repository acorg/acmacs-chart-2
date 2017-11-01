#pragma once

#include "base.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Passage : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Passage

// ----------------------------------------------------------------------

    class Reassortant : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Reassortant

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
