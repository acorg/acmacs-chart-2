#pragma once

#include "acmacs-virus/log.hh"

// ----------------------------------------------------------------------

namespace acmacs::log
{
    enum {
        relax = 8,
        report_stresses
    };

    inline void register_enabler_acmacs_chart()
    {
        using namespace std::string_view_literals;
        register_enabler_acmacs_virus();
        register_enabler("relax"sv, relax);
        register_enabler("report-stresses"sv, report_stresses);
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
