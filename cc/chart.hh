#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Antigens
    {
      public:
        virtual ~Antigens();

        virtual size_t size() const = 0;

    }; // class Antigens

// ----------------------------------------------------------------------

    class Sera
    {
      public:
        virtual ~Sera();

        virtual size_t size() const = 0;

    }; // class Sera

// ----------------------------------------------------------------------

    class Chart
    {
      public:
        virtual ~Chart();

          // read-only access
        virtual const Antigens& antigens() const = 0;
        virtual const Sera& sera() const = 0;

        inline size_t number_of_antigens() const { return antigens().size(); }
        inline size_t number_of_sera() const { return sera().size(); }

          // modifications

    }; // class Chart

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
