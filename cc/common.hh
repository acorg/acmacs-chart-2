#pragma once

#include <memory>
#include <vector>

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
        operator bool() const;

        struct common_t
        {
            common_t(size_t p, size_t s) : primary(p), secondary(s) {}
            size_t primary;
            size_t secondary;
        };

        std::vector<common_t> antigens() const;
        std::vector<common_t> sera() const; // returns serum indexes (NOT point indexes)!
        std::vector<common_t> points() const;

     private:
        class Impl;
        std::unique_ptr<Impl> impl_;

    }; // class CommonAntigensSera

} // namespace acmacs::chart

/// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
