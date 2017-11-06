#pragma once

#include <stdexcept>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    enum class Verify { None, All };

    class import_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
