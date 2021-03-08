#pragma once

#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace detail
    {
        template <typename AgSr> struct Selected
        {
            // Selected() = default;
            Selected(std::shared_ptr<AgSr> a_ag_sr) : ag_sr{a_ag_sr}, indexes{a_ag_sr->all_indexes()} {}
            // call func for each antigen/serum and select ag/sr if func returns true
            template <typename F> Selected(std::shared_ptr<AgSr> a_ag_sr, F&& func) : ag_sr{a_ag_sr}, indexes{a_ag_sr->indexes(std::forward<F>(func))} {}

            bool empty() const { return indexes.empty(); }
            size_t size() const { return indexes.size(); }

            // substitutions in format: {no0} {no1} {AG_SR} {name} {full_name}
            std::string report(std::string_view format = "{no0},") const
            {
                fmt::memory_buffer output;
                for (const auto no0 : indexes) {
                    fmt::format_to(output, "{}", ag_sr->at(no0)->format(format));
                }
                return fmt::to_string(output);
            }

            std::shared_ptr<AgSr> ag_sr;
            acmacs::chart::Indexes indexes;
        };
    } // namespace detail

    struct SelectedAntigens : public detail::Selected<acmacs::chart::Antigens>
    {
        using detail::Selected<acmacs::chart::Antigens>::Selected;
    };

    struct SelectedSera : public detail::Selected<acmacs::chart::Sera>
    {
        using detail::Selected<acmacs::chart::Sera>::Selected;
    };

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
