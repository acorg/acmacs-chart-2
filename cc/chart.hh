#pragma once

#include <memory>

#include "acmacs-base/range.hh"
#include "acmacs-base/color.hh"
#include "acmacs-base/point-style.hh"
#include "acmacs-base/transformation.hh"
#include "acmacs-chart-2/base.hh"
#include "acmacs-chart-2/passage.hh"
#include "acmacs-chart-2/layout.hh"

// ----------------------------------------------------------------------

namespace rjson { class array; }

namespace acmacs::chart
{
    namespace internal
    {
        template <typename Parent, typename Reference> class iterator
        {
         public:
            using reference = Reference;

            constexpr inline iterator& operator++() { ++mIndex; return *this; }
            constexpr inline bool operator!=(const iterator& other) const { return &mParent != &other.mParent || mIndex != other.mIndex; }
            constexpr inline reference operator*() { return mParent[mIndex]; }

         private:
            inline iterator(const Parent& aParent, size_t aIndex) : mParent{aParent}, mIndex{aIndex} {}

            const Parent& mParent;
            size_t mIndex;

            friend Parent;
        };

    } // namespace internal

// ----------------------------------------------------------------------

    class data_not_available : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class invalid_titer : public std::runtime_error { public: using std::runtime_error::runtime_error; };

// ----------------------------------------------------------------------

    using Indexes = std::vector<size_t>;

    class Info
    {
     public:
        enum class Compute { No, Yes };

        virtual ~Info();

        virtual std::string make_info() const;

        virtual std::string name(Compute = Compute::No) const = 0;
        inline std::string name_non_empty() const { const auto result = name(Compute::Yes); return result.empty() ? std::string{"UNKNOWN"} : result; }
        virtual std::string virus(Compute = Compute::No) const = 0;
        inline std::string virus_not_influenza(Compute aCompute = Compute::No) const { const auto v = virus(aCompute); return string::lower(v) == "influenza" ? std::string{} : v; }
        virtual std::string virus_type(Compute = Compute::No) const = 0;
        virtual std::string subset(Compute = Compute::No) const = 0;
        virtual std::string assay(Compute = Compute::No) const = 0;
        virtual std::string lab(Compute = Compute::No) const = 0;
        virtual std::string rbc_species(Compute = Compute::No) const = 0;
        virtual std::string date(Compute aCompute = Compute::No) const = 0;
        virtual size_t number_of_sources() const = 0;
        virtual std::shared_ptr<Info> source(size_t aSourceNo) const = 0;

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

    enum class BLineage { Unknown, Victoria, Yamagata };

    class LabIds : public internal::string_list_data
    {
     public:
        using internal::string_list_data::string_list_data;

    }; // class LabIds

    class Annotations : public internal::string_list_data
    {
     public:
        using internal::string_list_data::string_list_data;

    }; // class Annotations

    class Clades : public internal::string_list_data
    {
     public:
        using internal::string_list_data::string_list_data;

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

    class Titer : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

        enum Type { Invalid, Regular, DontCare, LessTnan, MoreThan, Dodgy };

        inline Type type() const
            {
                if (data().empty())
                    return Invalid;
                switch (data()[0]) {
                  case '*':
                      return DontCare;
                  case '<':
                      return LessTnan;
                  case '>':
                      return MoreThan;
                  case '~':
                      return Dodgy;
                  case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                      return Regular;
                  default:
                      return Invalid;
                }
                return Invalid;
            }

        inline bool is_dont_care() const { return type() == DontCare; }
        inline bool is_regular() const { return type() == Regular; }

        double logged() const;
        std::string logged_as_string() const;
        double logged_for_column_bases() const;

    }; // class Titer

    class PointIndexList : public internal::index_list_data
    {
     public:
        using internal::index_list_data::index_list_data;

    }; // class PointIndexList

    class AvidityAdjusts : public internal::double_list_data
    {
     public:
        using internal::double_list_data::double_list_data;

        constexpr inline bool empty() const
            {
                return internal::double_list_data::empty() || std::all_of(begin(), end(), [](double val) -> bool { return float_equal(val, 1.0); });
            }

    }; // class AvidityAdjusts

    class MinimumColumnBasis
    {
     public:
        inline MinimumColumnBasis(double aValue = 0) : mValue{aValue} {}
        inline MinimumColumnBasis(const MinimumColumnBasis&) = default;
        inline MinimumColumnBasis(std::string aValue) { from(aValue); }
        inline MinimumColumnBasis& operator=(double aValue) { mValue = aValue; return *this; }
        inline MinimumColumnBasis& operator=(const MinimumColumnBasis&) = default;
        inline MinimumColumnBasis& operator=(std::string aValue) { from(aValue); return *this; }

        inline constexpr bool is_none() const { return float_zero(mValue); }

        inline constexpr operator double() const noexcept { return mValue; }

        inline operator std::string() const noexcept
            {
                if (is_none())
                    return "none";
                else if (float_equal(mValue, 7.0))
                    return "1280";
                else
                    return acmacs::to_string(mValue);
            }

     private:
        double mValue;

        inline void from(std::string aValue)
            {
                if (aValue.empty() || aValue == "none")
                    mValue = 0;
                else if (aValue == "1280")
                    mValue = 7;
                else
                    throw std::runtime_error{"Unrecognized minimum_column_basis value: " + aValue};
            }

    }; // class MinimumColumnBasis

    class DrawingOrder : public internal::index_list_data
    {
     public:
        using internal::index_list_data::index_list_data;

        inline size_t index_of(size_t aValue) const { return static_cast<size_t>(std::find(begin(), end(), aValue) - begin()); }

    }; // class DrawingOrder

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
        virtual PointIndexList homologous_antigens() const = 0;

    }; // class Serum

// ----------------------------------------------------------------------

    class Antigens
    {
      public:
        virtual ~Antigens();

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Antigen> operator[](size_t aIndex) const = 0;
        using iterator = internal::iterator<Antigens, std::shared_ptr<Antigen>>;
        inline iterator begin() const { return {*this, 0}; }
        inline iterator end() const { return {*this, size()}; }

        inline Indexes all_indexes() const { return acmacs::filled_with_indexes(size()); }
        inline Indexes reference_indexes() const { return make_indexes([](const Antigen& ag) { return ag.reference(); }); }
        inline Indexes test_indexes() const { return make_indexes([](const Antigen& ag) { return !ag.reference(); }); }
        inline Indexes egg_indexes() const { return make_indexes([](const Antigen& ag) { return ag.passage().is_egg() || !ag.reassortant().empty(); }); }
        inline Indexes reassortant_indexes() const { return make_indexes([](const Antigen& ag) { return !ag.reassortant().empty(); }); }

     private:
        inline Indexes make_indexes(std::function<bool (const Antigen& ag)> test) const
            {
                Indexes result;
                for (size_t no = 0; no < size(); ++no)
                    if (test(*operator[](no)))
                        result.push_back(no);
                return result;
            }

    }; // class Antigens

// ----------------------------------------------------------------------

    class Sera
    {
      public:
        virtual ~Sera();

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Serum> operator[](size_t aIndex) const = 0;
        using iterator = internal::iterator<Sera, std::shared_ptr<Serum>>;
        inline iterator begin() const { return {*this, 0}; }
        inline iterator end() const { return {*this, size()}; }

        inline Indexes all_indexes() const
            {
                return acmacs::filled_with_indexes(size());
            }

    }; // class Sera

// ----------------------------------------------------------------------

    class Titers
    {
      public:
        virtual ~Titers();

        virtual Titer titer(size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual size_t number_of_layers() const = 0;
        virtual size_t number_of_antigens() const = 0;
        virtual size_t number_of_sera() const = 0;
        virtual size_t number_of_non_dont_cares() const = 0;
        virtual inline double percent_of_non_dont_cares() const { return static_cast<double>(number_of_non_dont_cares()) / (number_of_antigens() * number_of_sera()); }

          // support for fast exporting into ace, if source was ace or acd1
        virtual inline const rjson::array& rjson_list_list() const { throw data_not_available{"rjson_list_list titers are not available"}; }
        virtual inline const rjson::array& rjson_list_dict() const { throw data_not_available{"rjson_list_dict titers are not available"}; }
        virtual inline const rjson::array& rjson_layers() const { throw data_not_available{"rjson_list_dict titers are not available"}; }

    }; // class Titers

// ----------------------------------------------------------------------

    class ColumnBases
    {
      public:
        virtual ~ColumnBases();

        virtual bool exists() const = 0;
        inline operator bool() const { return exists(); }
        virtual double column_basis(size_t aSerumNo) const = 0;
        virtual size_t size() const = 0;

    }; // class ColumnBases

// ----------------------------------------------------------------------

    class Projection
    {
      public:
        virtual ~Projection();

        virtual std::string make_info() const;
        virtual double stress() const = 0;
        virtual std::string comment() const = 0;
        virtual std::shared_ptr<Layout> layout() const = 0;
        virtual MinimumColumnBasis minimum_column_basis() const = 0;
        virtual std::shared_ptr<ColumnBases> forced_column_bases() const = 0;
        virtual acmacs::Transformation transformation() const = 0;
        virtual bool dodgy_titer_is_regular() const = 0;
        virtual double stress_diff_to_stop() const = 0;
        virtual PointIndexList unmovable() const = 0;
        virtual PointIndexList disconnected() const = 0;
        virtual PointIndexList unmovable_in_the_last_dimension() const = 0;
        virtual AvidityAdjusts avidity_adjusts() const = 0; // antigens_sera_titers_multipliers, double for each point
          // antigens_sera_gradient_multipliers, double for each point

    }; // class Projection

// ----------------------------------------------------------------------

    class Projections
    {
      public:
        virtual ~Projections();

        virtual bool empty() const = 0;
        virtual size_t size() const = 0;
        virtual std::shared_ptr<Projection> operator[](size_t aIndex) const = 0;
        using iterator = internal::iterator<Projections, std::shared_ptr<Projection>>;
        inline iterator begin() const { return {*this, 0}; }
        inline iterator end() const { return {*this, size()}; }

        virtual std::string make_info() const;

    }; // class Projections

// ----------------------------------------------------------------------

    class PlotSpec
    {
      public:
        virtual ~PlotSpec();

        virtual bool empty() const = 0;
        virtual DrawingOrder drawing_order() const = 0;
        virtual Color error_line_positive_color() const = 0;
        virtual Color error_line_negative_color() const = 0;
        virtual PointStyle style(size_t aPointNo) const = 0;
        virtual std::vector<PointStyle> all_styles() const = 0;

    }; // class PlotSpec

// ----------------------------------------------------------------------

    class Chart
    {
      public:
        virtual ~Chart();

        virtual std::shared_ptr<Info> info() const = 0;
        virtual std::shared_ptr<Antigens> antigens() const = 0;
        virtual std::shared_ptr<Sera> sera() const = 0;
        virtual std::shared_ptr<Titers> titers() const = 0;
        virtual std::shared_ptr<ColumnBases> forced_column_bases() const = 0;
        virtual std::shared_ptr<ColumnBases> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis) const;
        virtual std::shared_ptr<Projections> projections() const = 0;
        inline std::shared_ptr<Projection> projection(size_t aProjectionNo) const { return (*projections())[aProjectionNo]; }
        virtual std::shared_ptr<PlotSpec> plot_spec() const = 0;

        virtual inline size_t number_of_antigens() const { return antigens()->size(); }
        virtual inline size_t number_of_sera() const { return sera()->size(); }
        inline size_t number_of_points() const { return number_of_antigens() + number_of_sera(); }
        virtual inline size_t number_of_projections() const { return projections()->size(); }

        std::string make_info() const;

    }; // class Chart

} // namespace acmacs::chart

// ----------------------------------------------------------------------

template <> struct std::iterator_traits<acmacs::chart::Antigens::iterator> { using reference = typename acmacs::chart::Antigens::iterator::reference; };
template <> struct std::iterator_traits<acmacs::chart::Sera::iterator> { using reference = typename acmacs::chart::Sera::iterator::reference; };
template <> struct std::iterator_traits<acmacs::chart::Projections::iterator> { using reference = typename acmacs::chart::Projections::iterator::reference; };

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
