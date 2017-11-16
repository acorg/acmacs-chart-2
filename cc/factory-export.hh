#pragma once

#include <string>
#include <memory>

#include "acmacs-chart-2/verify.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    void export_factory(std::shared_ptr<Chart> aChart, std::string aFilename, std::string aProgramName);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
