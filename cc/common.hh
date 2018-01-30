#pragma once

#include <memory>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;

    class CommonAntigensSera
    {
     public:
        enum class match_level_t { strict, relaxed, ignored, automatic };

        CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level);
        ~CommonAntigensSera();

        void report() const;

     private:
        class Impl;
        std::unique_ptr<Impl> impl_;

    }; // class CommonAntigensSera

} // namespace acmacs::chart

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
