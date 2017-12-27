#pragma once

#include "acmacs-chart-2/base.hh"
#include "acmacs-chart-2/column-bases.hh"

namespace rjson { class array; }

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class data_not_available : public std::runtime_error { public: inline data_not_available(std::string msg) : std::runtime_error("data_not_available: " + msg) {} };
    class invalid_titer : public std::runtime_error { public: inline invalid_titer(std::string msg) : std::runtime_error("invalid_titer: " + msg) {} };

// ----------------------------------------------------------------------

    class Titer : public internal::string_data
    {
     public:
        using internal::string_data::string_data;

        enum Type { Invalid, Regular, DontCare, LessThan, MoreThan, Dodgy };

        Type type() const;

        bool is_dont_care() const { return type() == DontCare; }
        bool is_regular() const { return type() == Regular; }
        bool is_less_than() const { return type() == LessThan; }
        bool is_more_than() const { return type() == MoreThan; }

        bool operator<(const Titer& other) const { return value_for_sorting() < other.value_for_sorting(); }
        bool operator==(const Titer& other) const { return data() == other.data(); }
        bool operator!=(const Titer& other) const { return ! operator==(other); }

        double logged() const;
        double logged_with_thresholded() const;
        std::string logged_as_string() const;
        double logged_for_column_bases() const;
        size_t value_for_sorting() const;

          // static inline Titer from_logged(double aLogged, std::string aPrefix = "") { return aPrefix + std::to_string(std::lround(std::pow(2.0, aLogged) * 10.0)); }
        static inline Titer from_logged(double aLogged, std::string aPrefix = "") { return aPrefix + std::to_string(std::lround(std::exp2(aLogged) * 10.0)); }

    }; // class Titer

    inline std::ostream& operator<<(std::ostream& s, const Titer& aTiter) { return s << aTiter.data(); }

// ----------------------------------------------------------------------

    class Titers
    {
      public:
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
        virtual inline double percent_of_non_dont_cares() const { return static_cast<double>(number_of_non_dont_cares()) / (number_of_antigens() * number_of_sera()); }

          // support for fast exporting into ace, if source was ace or acd1
        virtual inline const rjson::array& rjson_list_list() const { throw data_not_available{"rjson_list_list titers are not available"}; }
        virtual inline const rjson::array& rjson_list_dict() const { throw data_not_available{"rjson_list_dict titers are not available"}; }
        virtual inline const rjson::array& rjson_layers() const { throw data_not_available{"rjson_list_dict titers are not available"}; }

        std::shared_ptr<ColumnBases> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis, size_t number_of_antigens, size_t number_of_sera) const;

    }; // class Titers

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    template <> inline std::string to_string(acmacs::chart::Titer aTiter)
    {
        return aTiter.data();
    }

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
