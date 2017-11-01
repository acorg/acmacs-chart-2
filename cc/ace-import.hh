#pragma once

#include "acmacs-base/rjson.hh"
#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class AceChart : public Chart
    {
      public:
        inline AceChart(rjson::value&& aSrc) : mData{std::move(aSrc)} {}

        std::shared_ptr<Info> info() const override;
        std::shared_ptr<Antigens> antigens() const override;
        std::shared_ptr<Sera> sera() const override;
        std::shared_ptr<Projections> projections() const override;

        void verify_data() const;

     private:
        rjson::value mData;

    }; // class Chart

    inline bool is_ace(const std::string_view& aData)
    {
        return aData.size() > 35 && aData.front() == '{' && aData.find("\"acmacs-ace-v1\"") != std::string_view::npos;
    }

    std::shared_ptr<Chart> ace_import(const std::string_view& aData, bool aVerify);

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
