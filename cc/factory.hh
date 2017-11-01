#pragma once

#include <string>
#include <memory>
#include <stdexcept>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    class import_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    
    std::shared_ptr<Chart> factory(std::string aFilename, bool aVerify);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
