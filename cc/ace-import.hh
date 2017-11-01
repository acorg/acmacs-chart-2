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

// ----------------------------------------------------------------------

    class AceInfo : public Info
    {
      public:
        inline AceInfo(const rjson::value& aData) : mData{aData} {}

        std::string make_info() const override;

        inline std::string name() const { return make_field("N", " + "); }
        inline std::string virus() const { return make_field("v", "+"); }
        inline std::string virus_type() const { return make_field("V", "+"); }
        inline std::string subset() const { return make_field("s", "+"); }
        inline std::string assay() const { return make_field("A", "+"); }
        inline std::string lab() const { return make_field("l", "+"); }
        inline std::string rbc_species() const { return make_field("r", "+"); }
        inline std::string date() const;
        inline size_t number_of_sources() const { return mData.get_or_empty_array("S").size(); }

     private:
        const rjson::value& mData;

        std::string make_field(const char* aField, const char* aSeparator) const;

    }; // class AceInfo

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
