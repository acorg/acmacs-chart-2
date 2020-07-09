#pragma once

#include "acmacs-chart-2/stress.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    // optimization engines do not like NaNs in layout, set disconnected point coordinates to 0 before calling an engine and restore them afterwards
    class DisconnectedPointsHandler
    {
      public:
        DisconnectedPointsHandler(const Stress& stress, double* arg_first) : stress_{stress}, arg_first_{arg_first}
        {
            stress_.set_coordinates_of_disconnected(arg_first_, 0.0, stress_.number_of_dimensions());
        }

        ~DisconnectedPointsHandler()
        {
            stress_.set_coordinates_of_disconnected(arg_first_, std::numeric_limits<double>::quiet_NaN(), stress_.number_of_dimensions());
        }

      private:
        const Stress& stress_;
        double* arg_first_;
    };
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
