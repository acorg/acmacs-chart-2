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

// ----------------------------------------------------------------------

    class AceAntigen : public Antigen
    {
      public:
        inline AceAntigen(const rjson::object& aData) : mData{aData} {}

        inline Name name() const override { return mData["N"]; }
        inline Date date() const override { return mData["D"]; }
        inline Passage passage() const override { return mData["P"]; }
        BLineage lineage() const override;
        inline Reassortant reassortant() const override { return mData["R"]; }
        inline LabIds lab_ids() const override { return mData["l"]; }
        inline Clades clades() const override { return mData["c"]; }
        inline Annotations annotations() const override { return mData["a"]; }
        inline bool reference() const override { return static_cast<std::string>(mData["S"]).find("R") != std::string::npos; }

     private:
        const rjson::object& mData;

    }; // class AceAntigen

// ----------------------------------------------------------------------

    class AceSerum : public Serum
    {
      public:
        inline AceSerum(const rjson::object& aData) : mData{aData} {}

        inline Name name() const override { return mData["N"]; }
        inline Passage passage() const override { return mData["P"]; }
        BLineage lineage() const override;
        inline Reassortant reassortant() const override { return mData["R"]; }
        inline Annotations annotations() const override { return mData["a"]; }
        inline SerumId serum_id() const override { return mData["I"]; }
        inline SerumSpecies serum_species() const override { return mData["s"]; }

     private:
        const rjson::object& mData;

    }; // class AceSerum

// ----------------------------------------------------------------------

    class AceAntigens : public Antigens
    {
      public:
        inline AceAntigens(const rjson::array& aData) : mData{aData} {}

        inline size_t size() const override { return mData.size(); }
        inline std::shared_ptr<Antigen> operator[](size_t aIndex) const override { return std::make_shared<AceAntigen>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceAntigens

// ----------------------------------------------------------------------

    class AceSera : public Sera
    {
      public:
        inline AceSera(const rjson::array& aData) : mData{aData} {}

        inline size_t size() const override { return mData.size(); }
        inline std::shared_ptr<Serum> operator[](size_t aIndex) const override { return std::make_shared<AceSerum>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceSera

// ----------------------------------------------------------------------

    class AceProjection : public Projection
    {
      public:
        inline AceProjection(const rjson::object& aData) : mData{aData} {}

        inline double stress() const override { return mData.get_or_default("s", 0.0); }
        size_t number_of_dimensions() const override;

     private:
        const rjson::object& mData;

    }; // class AceProjections

// ----------------------------------------------------------------------

    class AceProjections : public Projections
    {
      public:
        inline AceProjections(const rjson::array& aData) : mData{aData} {}

        inline bool empty() const override { return mData.empty(); }
        inline size_t size() const override { return mData.size(); }
        inline std::shared_ptr<Projection> operator[](size_t aIndex) const override { return std::make_shared<AceProjection>(mData[aIndex]); }

     private:
        const rjson::array& mData;

    }; // class AceProjections

// ----------------------------------------------------------------------

} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
