#include "acmacs-base/fmt.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

int main()
{
    fmt::print(" {}\n", acmacs::chart::format_help());
    return 0;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
