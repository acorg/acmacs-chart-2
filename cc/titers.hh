#pragma once

#include <memory>

#include "acmacs-base/rjson-forward.hh"
#include "acmacs-chart-2/base.hh"
#include "acmacs-chart-2/column-bases.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class data_not_available : public std::runtime_error { public: data_not_available(std::string msg) : std::runtime_error("data_not_available: " + msg) {} };
    class invalid_titer : public std::runtime_error { public: invalid_titer(std::string msg) : std::runtime_error("invalid_titer: " + msg) {} };

// ----------------------------------------------------------------------

    class Titer : public detail::string_data
    {
     public:
        using detail::string_data::string_data;
        Titer() : detail::string_data::string_data("*") {}

        enum Type { Invalid, Regular, DontCare, LessThan, MoreThan, Dodgy };

        Type type() const
        {
            if (empty())
                return Invalid;
            switch (front()) {
                case '*':
                    return DontCare;
                case '<':
                    return LessThan;
                case '>':
                    return MoreThan;
                case '~':
                    return Dodgy;
                case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                    return Regular;
                default:
                    return Invalid;
            }
        }

        bool is_invalid() const { return type() == Invalid; }
        bool is_dont_care() const { return type() == DontCare; }
        bool is_regular() const { return type() == Regular; }
        bool is_less_than() const { return type() == LessThan; }
        bool is_more_than() const { return type() == MoreThan; }

        bool operator<(const Titer& other) const { return value_for_sorting() < other.value_for_sorting(); }
        // bool operator==(const Titer& other) const { return data() == other.data(); }
        // bool operator!=(const Titer& other) const { return ! operator==(other); }

        double logged() const;
        double logged_with_thresholded() const;
        std::string logged_as_string() const;
        double logged_for_column_bases() const;
        size_t value() const;
        size_t value_for_sorting() const;
        Titer multiplied_by(double value) const; // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *

          // static inline Titer from_logged(double aLogged, std::string aPrefix = "") { return aPrefix + std::to_string(std::lround(std::pow(2.0, aLogged) * 10.0)); }
        static inline Titer from_logged(double aLogged, const char* aPrefix = "") { return aPrefix + std::to_string(std::lround(std::exp2(aLogged) * 10.0)); }

    }; // class Titer

    // inline std::ostream& operator<<(std::ostream& s, const Titer& aTiter) { return s << aTiter; }

// ----------------------------------------------------------------------

    class Titers;

    class TiterIterator
    {
     public:
        struct Data
        {
            constexpr operator const Titer& () const { return titer; }
            constexpr bool operator==(const Data& rhs) const { return antigen == rhs.antigen && serum == rhs.serum; }
            Titer titer;
            size_t antigen, serum;
        };

        class Implementation
        {
         public:
            Implementation() = default;
            Implementation(const Titer& titer, size_t antigen, size_t serum) : data_{titer, antigen, serum} {}
            virtual ~Implementation() = default;
            constexpr bool operator==(const Implementation& rhs) const { return data_ == rhs.data_; }
            constexpr const Data& operator*() const { return data_; }
            constexpr const Data* ptr() const { return &data_; }
            virtual void operator++() = 0;

         protected:
            Data data_;
        };

        TiterIterator(Implementation* implementation) : data_{implementation} {}
        bool operator==(const TiterIterator& rhs) const { return *data_ == *rhs.data_; }
        bool operator!=(const TiterIterator& rhs) const { return !operator==(rhs); }
        const Data& operator*() const { return **data_; }
        const Data* operator->() const { return data_->ptr(); }
        const TiterIterator& operator++() { data_->operator++(); return *this; }

     private:
        std::unique_ptr<Implementation> data_;

    }; // class TiterIterator

// ----------------------------------------------------------------------

    template <typename Float> class TableDistances;
    class PointIndexList;
    class AvidityAdjusts;
    struct StressParameters;

    class Titers
    {
     public:
        static constexpr double dense_sparse_boundary = 0.7;

        virtual ~Titers() {}
        Titers() = default;
        Titers(const Titers&) = delete;

        virtual Titer titer(size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual Titer titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const = 0;
        virtual std::vector<Titer> titers_for_layers(size_t aAntigenNo, size_t aSerumNo) const = 0; // returns list of non-dont-care titers in layers, may throw data_not_available
        virtual size_t number_of_layers() const = 0;
        virtual size_t number_of_antigens() const = 0;
        virtual size_t number_of_sera() const = 0;
        virtual size_t number_of_non_dont_cares() const = 0;
        virtual double percent_of_non_dont_cares() const { return static_cast<double>(number_of_non_dont_cares()) / (number_of_antigens() * number_of_sera()); }
        virtual bool is_dense() const noexcept;

          // support for fast exporting into ace, if source was ace or acd1
        virtual const rjson::v1::array& rjson_list_list() const { throw data_not_available{"rjson_list_list titers are not available"}; }
        virtual const rjson::v1::array& rjson_list_dict() const { throw data_not_available{"rjson_list_dict titers are not available"}; }
        virtual const rjson::v1::array& rjson_layers() const { throw data_not_available{"rjson_list_dict titers are not available"}; }

        std::shared_ptr<ColumnBases> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis) const;

        TableDistances<double> table_distances(const ColumnBases& column_bases, const StressParameters& parameters);
        virtual void update(TableDistances<float>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters) const;
        virtual void update(TableDistances<double>& table_distances, const ColumnBases& column_bases, const StressParameters& parameters) const;
        virtual double max_distance(const ColumnBases& column_bases);

        virtual TiterIterator begin() const;
        virtual TiterIterator end() const;

        PointIndexList having_titers_with(size_t point_no) const;
          // returns list of points having less than threshold numeric titers
        PointIndexList having_too_few_numeric_titers(size_t threshold = 3) const;

    }; // class Titers

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    // inline std::string to_string(acmacs::chart::Titer aTiter)
    // {
    //     return aTiter.data();
    // }

    inline std::string to_string(acmacs::chart::TiterIterator::Data data)
    {
        return "ag:" + std::to_string(data.antigen) + " sr:" + std::to_string(data.serum) + " t:" + data.titer;
    }

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
