#pragma once

#include <string>
#include <memory>

#include "acmacs-chart/verify.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    std::shared_ptr<Chart> factory(std::string aFilename, Verify aVerify);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
