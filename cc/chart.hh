#pragma once

#include <memory>

#include "base.hh"
#include "passage.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Info
    {
      public:
        virtual ~Info();

    }; // class Info

// ----------------------------------------------------------------------

    class Name : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Name

    class Date : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class Date

    enum class BLineage { Victoria, Yamagata };

    class LabIds
    {
    }; // class LabIds

    class Annotations
    {
    }; // class Annotations

    class Clades
    {
    }; // class Clades

    class SerumId : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class SerumId

    class SerumSpecies : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

    }; // class SerumSpecies

// ----------------------------------------------------------------------

    class Antigen
    {
      public:
        virtual ~Antigen();

        virtual Name name() const = 0;
        virtual Date date() const = 0;
        virtual Passage passage() const = 0;
        virtual BLineage lineage() const = 0;
        virtual Reassortant reassortant() const = 0;
        virtual LabIds lab_ids() const = 0;
        virtual Clades clades() const = 0;
        virtual Annotations annotations() const = 0;
        virtual bool reference() const = 0;

    }; // class Antigen

// ----------------------------------------------------------------------

    class Serum
    {
      public:
        virtual ~Serum();

        virtual Name name() const = 0;
        virtual Passage passage() const = 0;
        virtual BLineage lineage() const = 0;
        virtual Reassortant reassortant() const = 0;
        virtual Annotations annotations() const = 0;
        virtual SerumId serum_id() const = 0;
        virtual SerumSpecies serum_species() const = 0;

    }; // class Serum

// ----------------------------------------------------------------------

    class Antigens
    {
      public:
        virtual ~Antigens();

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Antigen> operator[](size_t aIndex) const = 0;

    }; // class Antigens

// ----------------------------------------------------------------------

    class Sera
    {
      public:
        virtual ~Sera();

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Serum> operator[](size_t aIndex) const = 0;

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

        virtual std::shared_ptr<Info> info() const = 0;
        virtual std::shared_ptr<Antigens> antigens() const = 0;
        virtual std::shared_ptr<Sera> sera() const = 0;
          // titers
          // forced column bases for new projections
        virtual std::shared_ptr<Projections> projections() const = 0;
          // plot spec

        inline size_t number_of_antigens() const { return antigens()->size(); }
        inline size_t number_of_sera() const { return sera()->size(); }
        inline size_t number_of_projections() const { return projections()->size(); }

    }; // class Chart

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
