#pragma once

#include "acmacs-base/named-type.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Lab : public acmacs::named_string_t<struct chart_lab_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_lab_tag_t>::named_string_t;
    };

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
