#pragma once

#include <stdexcept>
#include <vector>
#include <algorithm>

#include "acmacs-base/named-type.hh"
#include "acmacs-base/to-string.hh"
#include "acmacs-base/number-of-dimensions.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    enum class optimization_method { alglib_lbfgs_pca, alglib_cg_pca };
    enum class optimization_precision { rough, very_rough, fine };
    enum class multiply_antigen_titer_until_column_adjust { no, yes };
    enum class dodgy_titer_is_regular { no, yes };

    using number_of_optimizations_t = named_t<size_t, struct number_of_optimizations_tag>;

    struct optimization_options
    {
        optimization_options() = default;
        optimization_options(optimization_precision a_precision, double a_randomization_diameter_multiplier = 2.0)
            : precision{a_precision}, randomization_diameter_multiplier{a_randomization_diameter_multiplier} {}
        optimization_options(optimization_method a_method, optimization_precision a_precision = optimization_precision::fine, double a_randomization_diameter_multiplier = 2.0)
            : method{a_method}, precision{a_precision}, randomization_diameter_multiplier{a_randomization_diameter_multiplier} {}

        optimization_method method = optimization_method::alglib_cg_pca;
        optimization_precision precision = optimization_precision::fine;
        multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes;
        double randomization_diameter_multiplier = 2.0; // for layout randomizations
        int num_threads = 0;                            // 0 - omp_get_max_threads()

    }; // struct optimization_options

    struct dimension_schedule
    {
        dimension_schedule(number_of_dimensions_t target_number_of_dimensions = number_of_dimensions_t{2}) : schedule{5, target_number_of_dimensions} {}
        dimension_schedule(std::initializer_list<number_of_dimensions_t> arg) : schedule(arg) {}
        dimension_schedule(const std::vector<number_of_dimensions_t>& arg) : schedule(arg) {}
        dimension_schedule(const std::vector<size_t>& arg) : schedule(arg.size(), number_of_dimensions_t{0}) { std::transform(std::begin(arg), std::end(arg), std::begin(schedule), [](size_t src) { return number_of_dimensions_t{src}; }); }

        size_t size() const { return schedule.size(); }
        number_of_dimensions_t initial() const { return schedule.front(); }
        number_of_dimensions_t final() const { return schedule.back(); }

          // using const_iterator = std::vector<size_t>::const_iterator;
        auto begin() const { return schedule.begin(); }
        auto end() const { return schedule.end(); }

        std::vector<number_of_dimensions_t> schedule;

    }; // struct dimension_schedule

    inline optimization_method optimization_method_from_string(std::string_view source)
    {
        optimization_method method{optimization_method::alglib_cg_pca};
        if (source == "lbfgs")
            method = acmacs::chart::optimization_method::alglib_lbfgs_pca;
        else if (source == "cg")
            method = acmacs::chart::optimization_method::alglib_cg_pca;
        else
            throw std::runtime_error("unrecognized method: \"" + std::string(source) + "\", lbfgs or cg expected");
        return method;
    }

} // namespace acmacs::chart

namespace acmacs
{
    inline std::string to_string(const acmacs::chart::dimension_schedule& src) { return to_string(src.schedule); }

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
