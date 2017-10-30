#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Info
    {
      public:
        virtual ~Info();

    }; // class Info

// ----------------------------------------------------------------------

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

    class Projections
    {
      public:
        virtual ~Projections();

        virtual size_t size() const = 0;

    }; // class Projections

// ----------------------------------------------------------------------

    class Chart
    {
      public:
        virtual ~Chart();

          // read-only access
        virtual const Info& info() const = 0;
        virtual const Antigens& antigens() const = 0;
        virtual const Sera& sera() const = 0;
          // titers
          // forced column bases for new projections
        virtual const Projections& projections() const = 0;
          // plot spec

        inline size_t number_of_antigens() const { return antigens().size(); }
        inline size_t number_of_sera() const { return sera().size(); }
        inline size_t number_of_projections() const { return projections().size(); }

          // --------------------------------------------------
          // modifications

    }; // class Chart

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
