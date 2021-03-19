#pragma once

#include "acmacs-base/string-join.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/name-format.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    namespace detail
    {
        template <typename AgSr, typename Chrt> struct Selected
        {
            using AntigensSeraType = AgSr;

            // Selected() = default;
            Selected(std::shared_ptr<Chrt> a_chart) : chart{a_chart}, indexes{ag_sr()->all_indexes()} {}
            // call func for each antigen/serum and select ag/sr if func returns true
            template <typename F> Selected(std::shared_ptr<Chrt> a_chart, F&& func) : chart{a_chart}, indexes{ag_sr()->indexes(std::forward<F>(func))} {}

            std::shared_ptr<AgSr> ag_sr() const;

            bool empty() const { return indexes.empty(); }
            size_t size() const { return indexes.size(); }

            // substitutions in format: {no0} {no1} {AG_SR} {name} {full_name}
            std::string report(std::string_view format = "{no0},") const
            {
                return acmacs::string::join(acmacs::string::join_concat, std::begin(indexes), std::end(indexes),
                                            [format, this](const auto no0) { return format_antigen_serum<AgSr>(format, *chart, no0, collapse_spaces_t::yes); });
            }

            void for_each(const std::function<void(size_t, std::shared_ptr<typename AgSr::AntigenSerumType>)>& modifier)
            {
                for (const auto index : indexes)
                    modifier(index, ag_sr()->ptr_at(index));
            }

            std::shared_ptr<Chrt> chart;
            acmacs::chart::Indexes indexes;
        };

        template <> inline std::shared_ptr<Antigens> Selected<Antigens, Chart>::ag_sr() const { return chart->antigens(); }
        template <> inline std::shared_ptr<AntigensModify> Selected<AntigensModify, ChartModify>::ag_sr() const { return chart->antigens_modify_ptr(); }
        template <> inline std::shared_ptr<Sera> Selected<Sera, Chart>::ag_sr() const { return chart->sera(); }
        template <> inline std::shared_ptr<SeraModify> Selected<SeraModify, ChartModify>::ag_sr() const { return chart->sera_modify_ptr(); }

    } // namespace detail

    struct SelectedAntigens : public detail::Selected<Antigens, Chart>
    {
        using detail::Selected<Antigens, Chart>::Selected;
    };

    struct SelectedAntigensModify : public detail::Selected<AntigensModify, ChartModify>
    {
        using detail::Selected<AntigensModify, ChartModify>::Selected;
    };

    struct SelectedSera : public detail::Selected<Sera, Chart>
    {
        using detail::Selected<Sera, Chart>::Selected;
    };

    struct SelectedSeraModify : public detail::Selected<SeraModify, ChartModify>
    {
        using detail::Selected<SeraModify, ChartModify>::Selected;
    };

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
