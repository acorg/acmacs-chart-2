#pragma once

#include "acmacs-chart-2/base.hh"

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

        inline Type type() const
            {
                if (data().empty())
                    return Invalid;
                switch (data()[0]) {
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
                return Invalid;
            }

        inline bool is_dont_care() const { return type() == DontCare; }
        inline bool is_regular() const { return type() == Regular; }
        inline bool is_less_than() const { return type() == LessThan; }
        inline bool is_more_than() const { return type() == MoreThan; }

        inline bool operator<(const Titer& other) const { return value_for_sorting() < other.value_for_sorting(); }
        inline bool operator==(const Titer& other) const { return data() == other.data(); }
        inline bool operator!=(const Titer& other) const { return ! operator==(other); }

        double logged() const
            {
                constexpr auto log_titer = [](std::string source) -> double { return std::log2(std::stod(source) / 10.0); };

                switch (type()) {
                  case Invalid:
                      throw invalid_titer(data());
                  case Regular:
                      return log_titer(data());
                  case DontCare:
                      throw invalid_titer(data());
                  case LessThan:
                  case MoreThan:
                  case Dodgy:
                      return log_titer(data().substr(1));
                }
                throw invalid_titer(data()); // for gcc 7.2
            }

        double logged_with_thresholded() const
            {
                switch (type()) {
                  case Invalid:
                  case Regular:
                  case DontCare:
                  case Dodgy:
                      return logged();
                  case LessThan:
                      return logged() - 1;
                  case MoreThan:
                      return logged() + 1;
                }
                throw invalid_titer(data()); // for gcc 7.2
            }

        std::string logged_as_string() const
            {
                switch (type()) {
                  case Invalid:
                      throw invalid_titer(data());
                  case Regular:
                      return acmacs::to_string(logged());
                  case DontCare:
                      return data();
                  case LessThan:
                  case MoreThan:
                  case Dodgy:
                      return data()[0] + acmacs::to_string(logged());
                }
                throw invalid_titer(data()); // for gcc 7.2
            }

        double logged_for_column_bases() const
            {
                switch (type()) {
                  case Invalid:
                      throw invalid_titer(data());
                  case Regular:
                  case LessThan:
                      return logged();
                  case MoreThan:
                      return logged() + 1;
                  case DontCare:
                  case Dodgy:
                      return -1;
                }
                throw invalid_titer(data()); // for gcc 7.2
            }

        size_t value_for_sorting() const
            {
                switch (type()) {
                  case Invalid:
                  case DontCare:
                      return 0;
                  case Regular:
                      return std::stoul(data());
                  case LessThan:
                      return std::stoul(data().substr(1)) - 1;
                  case MoreThan:
                      return std::stoul(data().substr(1)) + 1;
                  case Dodgy:
                      return std::stoul(data().substr(1));
                }
                return 0;
            }

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
