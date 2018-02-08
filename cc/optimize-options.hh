#pragma once

#include <stdexcept>
#include <vector>

#include "acmacs-base/to-string.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    enum class optimization_method { alglib_lbfgs_pca, alglib_cg_pca };
    enum class optimization_precision { rough, fine };
    enum class multiply_antigen_titer_until_column_adjust { no, yes };

    struct optimization_options
    {
        optimization_options() = default;
        optimization_options(optimization_method a_method, optimization_precision a_precision = optimization_precision::fine, double a_max_distance_multiplier = 1.0)
            : method{a_method}, precision{a_precision}, max_distance_multiplier{a_max_distance_multiplier} {}

        optimization_method method = optimization_method::alglib_cg_pca;
        optimization_precision precision = optimization_precision::fine;
        multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes;
        double max_distance_multiplier = 1.0; // for layout randomizations
    }; // struct optimization_options

    struct dimension_schedule
    {
        dimension_schedule(size_t target_number_of_dimensions = 2) : schedule{5, target_number_of_dimensions} {}
        dimension_schedule(std::initializer_list<size_t> arg) : schedule(arg) {}
        dimension_schedule(const std::vector<size_t>& arg) : schedule(arg) {}

        size_t size() const { return schedule.size(); }
        size_t initial() const { return schedule.front(); }
        size_t final() const { return schedule.back(); }

          // using const_iterator = std::vector<size_t>::const_iterator;
        auto begin() const { return schedule.begin(); }
        auto end() const { return schedule.end(); }

        std::vector<size_t> schedule;

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
