#pragma once

#include <memory>
#include <cmath>
#include <optional>

#include "acmacs-base/timeit.hh"
#include "acmacs-base/range.hh"
#include "acmacs-base/color.hh"
#include "acmacs-base/point-style.hh"
#include "acmacs-base/layout.hh"
#include "acmacs-base/debug.hh"
#include "acmacs-base/lab.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-chart-2/titers.hh"
#include "acmacs-chart-2/stress.hh"
#include "acmacs-chart-2/optimize.hh"
#include "acmacs-chart-2/serum-circle.hh"
#include "acmacs-chart-2/blobs.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    using Indexes = PointIndexList;

    class invalid_data : public std::runtime_error
    {
      public:
        invalid_data(std::string_view msg) : std::runtime_error{fmt::format("invalid_data: {}", msg)} {}
    };
    class chart_is_read_only : public std::runtime_error
    {
      public:
        chart_is_read_only(std::string_view msg) : std::runtime_error{fmt::format("chart_is_read_only: ", msg)} {}
    };
    class serum_coverage_error : public std::runtime_error
    {
      public:
        serum_coverage_error(std::string_view msg) : std::runtime_error{fmt::format("serum_coverage: ", msg)} {}
    };

    enum class find_homologous {
        strict,         // passage must match
        relaxed_strict, // if serum has no homologous antigen, relax passage matching for it but use only not previous matched with other sera antigens
        relaxed,        // if serum has no homologous antigen, relax passage matching for it and use any antigen
        all             // find all possible antigens with relaxed passage matching, use for serum circles
    };

    enum class reassortant_as_egg { no, yes };

    class Chart;

    // ----------------------------------------------------------------------

    class Virus : public acmacs::named_string_t<struct chart_virus_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_virus_tag_t>::named_string_t;
    };

    class Assay : public acmacs::named_string_t<struct chart_assay_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_assay_tag_t>::named_string_t;

        std::string hi_or_neut() const { return (empty() || get() == "HI") ? "hi" : "neut"; }
    };

    class RbcSpecies : public acmacs::named_string_t<struct chart_rbc_species_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_rbc_species_tag_t>::named_string_t;
    };

    class TableDate : public acmacs::named_string_t<struct chart_table_date_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_table_date_tag_t>::named_string_t;
    };

    class Date : public acmacs::named_string_t<struct chart_date_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_date_tag_t>::named_string_t;

        bool within_range(std::string_view first_date, std::string_view after_last_date) const
        {
            return !empty() && (first_date.empty() || *this >= first_date) && (after_last_date.empty() || *this < after_last_date);
        }

        // void check() const;

    }; // class Date

    class BLineage
    {
      public:
        enum Lineage { Unknown, Victoria, Yamagata };

        BLineage() = default;
        BLineage(Lineage lineage) : mLineage{lineage} {}
        BLineage(const BLineage&) = default;
        BLineage(std::string_view lineage) : mLineage{from(lineage)} {}
        // BLineage(const std::string& lineage) : BLineage{std::string_view{lineage}} {}
        BLineage(char lineage) : mLineage{from(lineage)} {}
        BLineage& operator=(Lineage lineage)
        {
            mLineage = lineage;
            return *this;
        }
        BLineage& operator=(const BLineage&) = default;
        BLineage& operator=(std::string_view lineage)
        {
            mLineage = from(lineage);
            return *this;
        }
        bool operator==(BLineage lineage) const { return mLineage == lineage.mLineage; }
        bool operator!=(BLineage lineage) const { return !operator==(lineage); }
        bool operator==(Lineage lineage) const { return mLineage == lineage; }
        bool operator!=(Lineage lineage) const { return !operator==(lineage); }

        bool operator==(std::string_view rhs) const
        {
            if (rhs.empty())
                return mLineage == Unknown;
            switch (rhs.front()) {
              case 'V':
              case 'v':
                  return mLineage == Victoria;
              case 'Y':
              case 'y':
                  return mLineage == Yamagata;
            }
            return mLineage == Unknown;
        }

        bool operator!=(std::string_view rhs) const { return !operator==(rhs); }

        std::string to_string() const
        {
            using namespace std::string_literals;
            switch (mLineage) {
                case Victoria:
                    return "VICTORIA"s;
                case Yamagata:
                    return "YAMAGATA"s;
                case Unknown:
                    return {};
            }
#ifndef __clang__
            return "UNKNOWN"s;
#endif
        }

        operator acmacs::virus::lineage_t() const
        {
            return acmacs::virus::lineage_t{to_string()};
        }

        operator Lineage() const { return mLineage; }

      private:
        Lineage mLineage{Unknown};

        static Lineage from(std::string_view aSource);
        static Lineage from(char aSource);

    }; // class BLineage

    class Continent : public acmacs::named_string_t<struct chart_continent_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_continent_tag_t>::named_string_t;

    }; // class Continent

    class LabIds : public acmacs::named_vector_t<std::string, struct chart_LabIds_tag_t>
    {
      public:
        using acmacs::named_vector_t<std::string, struct chart_LabIds_tag_t>::named_vector_t;

        LabIds(const rjson::value& src) : acmacs::named_vector_t<std::string, struct chart_LabIds_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        std::string join() const { return string::join(acmacs::string::join_space, begin(), end()); }

    }; // class LabIds

    class Annotations : public acmacs::named_vector_t<std::string, struct chart_Annotations_tag_t>
    {
      public:
        using acmacs::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t;
        Annotations(const rjson::value& src) : acmacs::named_vector_t<std::string, struct chart_Annotations_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        bool distinct() const { return exists("DISTINCT"); }
        std::string join() const { return string::join(acmacs::string::join_space, begin(), end()); }
        size_t total_length() const { return std::accumulate(begin(), end(), size_t{0}, [](size_t sum, const auto& element) { return sum + element.size(); }); }

        // returns if annotations of antigen and serum matches (e.g. ignores CONC for serum), used for homologous pairs finding
        static bool match_antigen_serum(const Annotations& antigen, const Annotations& serum);

    }; // class Annotations

    class Clades : public acmacs::named_vector_t<std::string, struct chart_Clades_tag_t>
    {
      public:
        using acmacs::named_vector_t<std::string, struct chart_Clades_tag_t>::named_vector_t;
        Clades(const rjson::value& src) : acmacs::named_vector_t<std::string, struct chart_Clades_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

    }; // class Clades

    class SerumId : public acmacs::named_string_t<struct chart_SerumId_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_SerumId_tag_t>::named_string_t;

    }; // class SerumId

    class SerumSpecies : public acmacs::named_string_t<struct chart_SerumSpecies_tag_t>
    {
      public:
        using acmacs::named_string_t<struct chart_SerumSpecies_tag_t>::named_string_t;

    }; // class SerumSpecies

    class DrawingOrder : public acmacs::named_vector_t<size_t, struct chart_DrawingOrder_tag_t>
    {
      public:
        using acmacs::named_vector_t<size_t, struct chart_DrawingOrder_tag_t>::named_vector_t;
        DrawingOrder(const rjson::value& src) : acmacs::named_vector_t<size_t, struct chart_DrawingOrder_tag_t>::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        size_t index_of(size_t aValue) const { return static_cast<size_t>(std::find(begin(), end(), aValue) - begin()); }

        void raise(size_t aIndex)
        {
            if (const auto p = std::find(begin(), end(), aIndex); p != end())
                std::rotate(p, p + 1, end());
        }

        void raise(const std::vector<size_t>& aIndexes)
        {
            std::for_each(aIndexes.begin(), aIndexes.end(), [this](size_t index) { this->raise(index); });
        }

        void lower(size_t aIndex)
        {
            if (const auto p = std::find(rbegin(), rend(), aIndex); p != rend())
                std::rotate(p, p + 1, rend());
        }

        void lower(const std::vector<size_t>& aIndexes)
        {
            std::for_each(aIndexes.begin(), aIndexes.end(), [this](size_t index) { this->lower(index); });
        }

        void fill_if_empty(size_t aSize)
        {
            if (get().empty())
                acmacs::fill_with_indexes(get(), aSize);
        }

        void insert(size_t before)
        {
            std::for_each(begin(), end(), [before](size_t& point_no) {
                if (point_no >= before)
                    ++point_no;
            });
            push_back(before);
        }

        void remove_points(const ReverseSortedIndexes& to_remove, size_t base_index = 0)
        {
            for (const auto index : to_remove) {
                const auto real_index = index + base_index;
                remove(real_index);
                // if (const auto found = std::find(begin(), end(), real_index); found != end())
                //     erase(found);
                std::for_each(begin(), end(), [real_index](size_t& point_no) {
                    if (point_no > real_index)
                        --point_no;
                });
            }
        }

    }; // class DrawingOrder

    // ----------------------------------------------------------------------

    class Info
    {
      public:
        enum class Compute { No, Yes };
        enum class FixLab { no, yes, reverse };

        virtual ~Info() = default;
        Info() = default;
        Info(const Info&) = delete;

        virtual std::string make_info() const;
        std::string make_name() const;

        virtual std::string name(Compute = Compute::No) const = 0;
        std::string name_non_empty() const
        {
            const auto result = name(Compute::Yes);
            return result.empty() ? std::string{"UNKNOWN"} : result;
        }
        virtual Virus virus(Compute = Compute::No) const = 0;
        Virus virus_not_influenza(Compute aCompute = Compute::No) const
        {
            const auto vir = virus(aCompute);
            if (::string::lower(*vir) == "influenza")
              return {};
            else
              return vir;
        }
        virtual acmacs::virus::type_subtype_t virus_type(Compute = Compute::Yes) const = 0;
        virtual std::string subset(Compute = Compute::No) const = 0;
        virtual Assay assay(Compute = Compute::Yes) const = 0;
        virtual Lab lab(Compute = Compute::Yes, FixLab fix = FixLab::yes) const = 0;
        virtual RbcSpecies rbc_species(Compute = Compute::Yes) const = 0;
        virtual TableDate date(Compute aCompute = Compute::No) const = 0;
        virtual size_t number_of_sources() const = 0;
        virtual std::shared_ptr<Info> source(size_t aSourceNo) const = 0;
        size_t max_source_name() const;

      protected:
        Lab fix_lab_name(Lab source, FixLab fix) const;

    }; // class Info

    // ----------------------------------------------------------------------

    class Antigen
    {
      public:
        virtual ~Antigen() = default;
        Antigen() = default;
        Antigen(const Antigen&) = delete;
        bool operator==(const Antigen& rhs) const { return full_name() == rhs.full_name(); }
        bool operator!=(const Antigen& rhs) const { return !operator==(rhs); }

        virtual acmacs::virus::name_t name() const = 0;
        virtual Date date() const = 0;
        virtual acmacs::virus::Passage passage() const = 0;
        virtual BLineage lineage() const = 0;
        virtual acmacs::virus::Reassortant reassortant() const = 0;
        virtual LabIds lab_ids() const = 0;
        virtual Clades clades() const = 0;
        virtual Annotations annotations() const = 0;
        virtual bool reference() const = 0;
        virtual Continent continent() const { return {}; }

        std::string full_name() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), string::join(acmacs::string::join_space, annotations()), passage()); }
        std::string full_name_without_passage() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), string::join(acmacs::string::join_space, annotations())); }
        std::string full_name_with_passage() const { return full_name(); }
        std::string full_name_with_fields() const;
        std::string full_name_for_seqdb_matching() const
        {
            return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), passage(), string::join(acmacs::string::join_space, annotations()));
        } // annotations may part of the passage in seqdb (NIMR ISOLATE 1)
        std::string abbreviated_name() const { return acmacs::string::join(acmacs::string::join_space, name_abbreviated(), reassortant(), string::join(acmacs::string::join_space, annotations())); }
        std::string abbreviated_name_with_passage_type() const { return acmacs::string::join(acmacs::string::join_dash, name_abbreviated(), reassortant(), string::join(acmacs::string::join_space, annotations()), passage_type()); }
        std::string abbreviated_location_with_passage_type() const { return acmacs::string::join(acmacs::string::join_space, location_abbreviated(), passage_type()); }
        std::string designation() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), string::join(acmacs::string::join_space, annotations()), passage()); }

        std::string name_abbreviated() const;
        std::string name_without_subtype() const;
        std::string location() const;
        std::string location_abbreviated() const;
        std::string abbreviated_location_year() const;

        std::string_view passage_type(reassortant_as_egg rae = reassortant_as_egg::yes) const
        {
            using namespace std::string_view_literals;
            if (passage().is_egg()) {
                if (reassortant().empty() || rae == reassortant_as_egg::yes)
                    return "egg"sv;
                else
                    return "reassortant"sv;
            }
            else
                return "cell"sv;
        }

        bool is_egg(reassortant_as_egg rae) const { return rae == reassortant_as_egg::yes ? (!reassortant().empty() || passage().is_egg()) : (reassortant().empty() && passage().is_egg()); }
        bool is_cell() const { return !is_egg(reassortant_as_egg::yes); }
        bool distinct() const { return annotations().distinct(); }

    }; // class Antigen

    // ----------------------------------------------------------------------

    class Serum
    {
      public:
        virtual ~Serum() = default;
        Serum() = default;
        Serum(const Serum&) = delete;
        bool operator==(const Serum& rhs) const { return full_name() == rhs.full_name(); }
        bool operator!=(const Serum& rhs) const { return !operator==(rhs); }

        virtual acmacs::virus::name_t name() const = 0;
        virtual acmacs::virus::Passage passage() const = 0;
        virtual BLineage lineage() const = 0;
        virtual acmacs::virus::Reassortant reassortant() const = 0;
        virtual Annotations annotations() const = 0;
        virtual SerumId serum_id() const = 0;
        virtual SerumSpecies serum_species() const = 0;
        virtual PointIndexList homologous_antigens() const = 0;
        virtual void set_homologous(const std::vector<size_t>&, acmacs::debug) const {}

        std::string full_name() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), acmacs::string::join(acmacs::string::join_space, annotations()), serum_id()); }
        std::string full_name_without_passage() const { return full_name(); }
        std::string full_name_with_passage() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), acmacs::string::join(acmacs::string::join_space, annotations()), serum_id(), passage()); }
        std::string full_name_with_fields() const;
        std::string abbreviated_name() const { return acmacs::string::join(acmacs::string::join_space, name_abbreviated(), reassortant(), acmacs::string::join(acmacs::string::join_space, annotations())); }
        std::string abbreviated_name_with_serum_id() const { return acmacs::string::join(acmacs::string::join_space, name_abbreviated(), reassortant(), serum_id(), acmacs::string::join(acmacs::string::join_space, annotations())); }
        std::string designation() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), acmacs::string::join(acmacs::string::join_space, annotations()), serum_id()); }
        std::string designation_without_serum_id() const { return acmacs::string::join(acmacs::string::join_space, name(), reassortant(), acmacs::string::join(acmacs::string::join_space, annotations())); }

        std::string name_abbreviated() const;
        std::string name_without_subtype() const;
        std::string location() const;
        std::string location_abbreviated() const;
        std::string abbreviated_location_year() const;

        std::string_view passage_type(reassortant_as_egg rae = reassortant_as_egg::yes) const
        {
            using namespace std::string_view_literals;
            if (is_egg(reassortant_as_egg::yes)) { // note NIID passage cannot be used
                if (reassortant().empty() || rae == reassortant_as_egg::yes)
                    return "egg"sv;
                else
                    return "reassortant"sv;
            }
            else
                return "cell"sv;
        }

        bool is_egg(reassortant_as_egg rae) const
        {
            const auto egg = passage().is_egg() || serum_id().find("EGG") != std::string::npos;
            return rae == reassortant_as_egg::yes ? (!reassortant().empty() || egg) : (reassortant().empty() && egg);
        }
        bool is_cell() const { return !is_egg(reassortant_as_egg::yes); }

    }; // class Serum

    // ----------------------------------------------------------------------

    using duplicates_t = std::vector<std::vector<size_t>>;

    class Antigens
    {
      public:
        virtual ~Antigens() = default;
        Antigens() = default;
        Antigens(const Antigens&) = delete;

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Antigen> operator[](size_t aIndex) const = 0;
        std::shared_ptr<Antigen> at(size_t aIndex) const { return operator[](aIndex); }
        using iterator = acmacs::iterator<Antigens, std::shared_ptr<Antigen>>;
        iterator begin() const { return {*this, 0}; }
        iterator end() const { return {*this, size()}; }

        Indexes all_indexes() const { return Indexes{acmacs::filled_with_indexes(size())}; }
        Indexes reference_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return ag.reference(); });
        }
        Indexes test_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return !ag.reference(); });
        }
        Indexes egg_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return ag.passage().is_egg() || !ag.reassortant().empty(); });
        }
        Indexes reassortant_indexes() const
        {
            return make_indexes([](const Antigen& ag) { return !ag.reassortant().empty(); });
        }

        void filter_reference(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return !entry.reference(); });
        }
        void filter_test(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return entry.reference(); });
        }

        void filter_egg(Indexes& aIndexes, reassortant_as_egg rae = reassortant_as_egg::yes) const
        {
            remove(aIndexes, [rae](const auto& entry) -> bool { return !entry.is_egg(rae); });
        }
        void filter_cell(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return !entry.is_cell(); });
        }
        void filter_reassortant(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return entry.reassortant().empty(); });
        }
        void filter_passage(Indexes& aIndexes, std::string_view passage) const
        {
            remove(aIndexes, [passage](const auto& entry) -> bool { return entry.passage().find(passage) == std::string::npos; });
        }
        void filter_passage(Indexes& aIndexes, const std::regex& passage) const
        {
            remove(aIndexes, [&passage](const auto& entry) -> bool { return !entry.passage().search(passage); });
        }

        void filter_date_range(Indexes& aIndexes, std::string_view first_date, std::string_view after_last_date) const
        {
            remove(aIndexes, [first_date, after_last_date](const auto& entry) { return !entry.date().within_range(first_date, after_last_date); });
        }
        void filter_date_not_in_range(Indexes& aIndexes, std::string_view first_date, std::string_view after_last_date) const
        {
            remove(aIndexes, [first_date, after_last_date](const auto& entry) { return entry.date().within_range(first_date, after_last_date); });
        }
        void filter_country(Indexes& aIndexes, std::string_view aCountry) const;
        void filter_continent(Indexes& aIndexes, std::string_view aContinent) const;
        void filter_found_in(Indexes& aIndexes, const Antigens& aNother) const
        {
            remove(aIndexes, [&](const auto& entry) -> bool { return !aNother.find_by_full_name(entry.full_name()); });
        }
        void filter_not_found_in(Indexes& aIndexes, const Antigens& aNother) const
        {
            remove(aIndexes, [&](const auto& entry) -> bool { return aNother.find_by_full_name(entry.full_name()).has_value(); });
        }

        virtual std::optional<size_t> find_by_full_name(std::string_view aFullName) const;
        virtual Indexes find_by_name(std::string_view aName) const;
        virtual Indexes find_by_name(const std::regex& aName) const; // regex search in full name

        // max_full_name() - see global template below

        duplicates_t find_duplicates() const;

        enum class include_reference { no, yes };
        std::vector<Date> all_dates(include_reference inc_ref) const; // list of unique dates of the antigens of a chart

        void remove(Indexes& aIndexes, std::function<bool(const Antigen&)> aFilter) const
        {
            aIndexes.get().erase(std::remove_if(aIndexes.begin(), aIndexes.end(), [&aFilter, this](auto index) -> bool { return aFilter(*(*this)[index]); }), aIndexes.end());
        }

      private:
        Indexes make_indexes(std::function<bool(const Antigen& ag)> test) const
        {
            Indexes result;
            for (size_t no = 0; no < size(); ++no)
                if (test(*operator[](no)))
                    result.insert(no);
            return result;
        }

    }; // class Antigens

    // ----------------------------------------------------------------------

    class Sera
    {
      public:
        virtual ~Sera() = default;
        Sera() = default;
        Sera(const Sera&) = delete;

        virtual size_t size() const = 0;
        virtual std::shared_ptr<Serum> operator[](size_t aIndex) const = 0;
        std::shared_ptr<Serum> at(size_t aIndex) const { return operator[](aIndex); }
        using iterator = acmacs::iterator<Sera, std::shared_ptr<Serum>>;
        iterator begin() const { return {*this, 0}; }
        iterator end() const { return {*this, size()}; }

        Indexes all_indexes() const { return Indexes{acmacs::filled_with_indexes(size())}; }

        void filter_serum_id(Indexes& aIndexes, std::string_view aSerumId) const
        {
            remove(aIndexes, [&aSerumId](const auto& entry) -> bool { return entry.serum_id() != SerumId{aSerumId}; });
        }

        void filter_serum_id(Indexes& aIndexes, const std::regex& re_serum_id) const
        {
            remove(aIndexes, [&re_serum_id](const auto& entry) -> bool { return !std::regex_search(*entry.serum_id(), re_serum_id); });
        }

        void filter_country(Indexes& aIndexes, std::string_view aCountry) const;
        void filter_continent(Indexes& aIndexes, std::string_view aContinent) const;
        void filter_found_in(Indexes& aIndexes, const Sera& aNother) const
        {
            remove(aIndexes, [&](const auto& entry) -> bool { return !aNother.find_by_full_name(entry.full_name()); });
        }
        void filter_not_found_in(Indexes& aIndexes, const Sera& aNother) const
        {
            remove(aIndexes, [&](const auto& entry) -> bool { return aNother.find_by_full_name(entry.full_name()).has_value(); });
        }
        void filter_egg(Indexes& aIndexes, reassortant_as_egg rae = reassortant_as_egg::yes) const
        {
            remove(aIndexes, [rae](const auto& entry) -> bool { return !entry.is_egg(rae); });
        }
        void filter_cell(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return !entry.is_cell(); });
        }
        void filter_passage(Indexes& aIndexes, std::string_view passage) const
        {
            remove(aIndexes, [passage](const auto& entry) -> bool { return entry.passage().find(passage) == std::string::npos; });
        }
        void filter_passage(Indexes& aIndexes, const std::regex& passage) const
        {
            remove(aIndexes, [&passage](const auto& entry) -> bool { return !entry.passage().search(passage); });
        }

        void filter_reassortant(Indexes& aIndexes) const
        {
            remove(aIndexes, [](const auto& entry) -> bool { return entry.reassortant().empty(); });
        }

        virtual std::optional<size_t> find_by_full_name(std::string_view aFullName) const;
        virtual Indexes find_by_name(std::string_view aName) const;
        virtual Indexes find_by_name(const std::regex& aName) const; // regex search in full name
        // max_full_name() - see global template below

        void set_homologous(find_homologous options, const Antigens& aAntigens, acmacs::debug dbg = acmacs::debug::no);

        duplicates_t find_duplicates() const;

        void remove(Indexes& aIndexes, std::function<bool(const Serum&)> aFilter) const
        {
            aIndexes.get().erase(std::remove_if(aIndexes.begin(), aIndexes.end(), [&aFilter, this](auto index) -> bool { return aFilter(*(*this)[index]); }), aIndexes.end());
        }

      private:
        using homologous_canditate_t = Indexes;                              // indexes of antigens
        using homologous_canditates_t = std::vector<homologous_canditate_t>; // for each serum
        homologous_canditates_t find_homologous_canditates(const Antigens& aAntigens, acmacs::debug dbg) const;

    }; // class Sera

    // ----------------------------------------------------------------------

    enum class RecalculateStress { no, if_necessary, yes };
    constexpr const double InvalidStress{-1.0};

    class Projection
    {
      public:
        virtual ~Projection() = default;
        Projection(const Chart& chart) : chart_(chart) {}
        Projection(const Projection&) = delete;

        virtual size_t projection_no() const
        {
            if (!projection_no_)
                throw invalid_data("no projection_no");
            return *projection_no_;
        }
        virtual std::string make_info() const;
        virtual std::optional<double> stored_stress() const = 0;
        double stress(RecalculateStress recalculate = RecalculateStress::if_necessary) const;
        double stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const;
        virtual std::string comment() const = 0;
        virtual number_of_dimensions_t number_of_dimensions() const = 0;
        virtual size_t number_of_points() const = 0;
        virtual std::shared_ptr<Layout> layout() const = 0;
        virtual std::shared_ptr<Layout> transformed_layout() const { return layout()->transform(transformation()); }
        virtual MinimumColumnBasis minimum_column_basis() const = 0;
        virtual std::shared_ptr<ColumnBases> forced_column_bases() const = 0; // returns nullptr if not forced
        virtual acmacs::Transformation transformation() const = 0;
        virtual enum dodgy_titer_is_regular dodgy_titer_is_regular() const = 0;
        virtual double stress_diff_to_stop() const = 0;
        virtual UnmovablePoints unmovable() const = 0;
        virtual DisconnectedPoints disconnected() const = 0;
        virtual UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension() const = 0;
        virtual AvidityAdjusts avidity_adjusts() const = 0; // antigens_sera_titers_multipliers, double for each point
                                                            // antigens_sera_gradient_multipliers, double for each point

        double calculate_stress(const Stress& stress) const { return stress.value(*layout()); }
        std::vector<double> calculate_gradient(const Stress& stress) const { return stress.gradient(*layout()); }

        double calculate_stress(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;
        std::vector<double> calculate_gradient(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;

        Blobs blobs(double stress_diff, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;
        Blobs blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;

        Chart& chart() { return const_cast<Chart&>(chart_); }
        const Chart& chart() const { return chart_; }
        void set_projection_no(size_t projection_no) { projection_no_ = projection_no; }

        ErrorLines error_lines() const { return acmacs::chart::error_lines(*this); }

      protected:
        virtual double recalculate_stress() const { return calculate_stress(); }

      private:
        const Chart& chart_;
        std::optional<size_t> projection_no_;

    }; // class Projection

    // ----------------------------------------------------------------------

    class Projections
    {
      public:
        virtual ~Projections() = default;
        Projections(const Chart& chart) : chart_(chart) {}
        Projections(const Projections&) = delete;

        virtual bool empty() const = 0;
        virtual size_t size() const = 0;
        virtual std::shared_ptr<Projection> operator[](size_t aIndex) const = 0;
        virtual std::shared_ptr<Projection> best() const { return operator[](0); }
        using iterator = acmacs::iterator<Projections, std::shared_ptr<Projection>>;
        iterator begin() const { return {*this, 0}; }
        iterator end() const { return {*this, size()}; }
        // virtual size_t projection_no(const Projection* projection) const;

        virtual std::string make_info(size_t max_number_of_projections_to_show = 20) const;

        // Chart& chart() { return const_cast<Chart&>(chart_); }
        const Chart& chart() const { return chart_; }

      private:
        const Chart& chart_;

    }; // class Projections

    // ----------------------------------------------------------------------

    class PlotSpec : public PointStyles
    {
      public:
        virtual DrawingOrder drawing_order() const = 0;
        virtual Color error_line_positive_color() const = 0;
        virtual Color error_line_negative_color() const = 0;
        virtual std::vector<PointStyle> all_styles() const = 0;
        PointStylesCompacted compacted() const override;

    }; // class PlotSpec

    // ----------------------------------------------------------------------

    class Chart
    {
      protected:
        enum class PointType { TestAntigen, ReferenceAntigen, Serum };
        PointStyle default_style(PointType aPointType) const;

      public:
        enum class use_cache { no, yes };

        virtual ~Chart() = default;
        Chart() = default;
        Chart(const Chart&) = delete;
        Chart(Chart&&) = default;

        virtual std::shared_ptr<Info> info() const = 0;
        virtual std::shared_ptr<Antigens> antigens() const = 0;
        virtual std::shared_ptr<Sera> sera() const = 0;
        virtual std::shared_ptr<Titers> titers() const = 0;
        virtual std::shared_ptr<ColumnBases> forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const = 0; // returns nullptr if column bases not forced
        virtual std::shared_ptr<ColumnBases> computed_column_bases(MinimumColumnBasis aMinimumColumnBasis, use_cache a_use_cache = use_cache::no) const;
        double column_basis(size_t serum_no, size_t projection_no = 0) const;
        std::shared_ptr<ColumnBases> column_bases(MinimumColumnBasis aMinimumColumnBasis) const;
        virtual std::shared_ptr<Projections> projections() const = 0;
        std::shared_ptr<Projection> projection(size_t aProjectionNo) const { return (*projections())[aProjectionNo]; }
        virtual std::shared_ptr<PlotSpec> plot_spec() const = 0;
        virtual bool is_merge() const = 0;

        virtual size_t number_of_antigens() const { return antigens()->size(); }
        virtual size_t number_of_sera() const { return sera()->size(); }
        size_t number_of_points() const { return number_of_antigens() + number_of_sera(); }
        virtual size_t number_of_projections() const { return projections()->size(); }

        virtual const rjson::value& extension_field(std::string_view /*field_name*/) const { return rjson::ConstNull; }
        virtual const rjson::value& extension_fields() const { return rjson::ConstNull; }

        std::shared_ptr<Antigen> antigen(size_t aAntigenNo) const { return antigens()->operator[](aAntigenNo); }
        std::shared_ptr<Serum> serum(size_t aSerumNo) const { return sera()->operator[](aSerumNo); }
        acmacs::virus::lineage_t lineage() const;

        std::string make_info(size_t max_number_of_projections_to_show = 20) const;
        std::string make_name(std::optional<size_t> aProjectionNo = {}) const;
        std::string description() const;

        PointStyle default_style(size_t aPointNo) const
        {
            auto ags = antigens();
            return default_style(aPointNo < ags->size() ? ((*ags)[aPointNo]->reference() ? PointType::ReferenceAntigen : PointType::TestAntigen) : PointType::Serum);
        }
        std::vector<acmacs::PointStyle> default_all_styles() const;

        SerumCircle serum_circle_radius_empirical(const Indexes& antigens, Titer aHomologousTiter, size_t aSerumNo, size_t aProjectionNo, double fold = 2, acmacs::verbose verbose = acmacs::verbose::no) const // aFold=2 for 4fold, 3 - for 8fold
        {
            return serum_circle_empirical(antigens, aHomologousTiter, aSerumNo, *projection(aProjectionNo)->layout(), column_basis(aSerumNo, aProjectionNo), *titers(), fold, verbose);
        }
        SerumCircle serum_circle_radius_empirical(size_t aAntigenNo, size_t aSerumNo, size_t aProjectionNo, double fold = 2, acmacs::verbose verbose = acmacs::verbose::no) const
        {
            return serum_circle_empirical(aAntigenNo, aSerumNo, *projection(aProjectionNo)->layout(), column_basis(aSerumNo, aProjectionNo), *titers(), fold, verbose);
        }
        SerumCircle serum_circle_radius_empirical(const Indexes& antigens, size_t aSerumNo, size_t aProjectionNo, double fold = 2, acmacs::verbose verbose = acmacs::verbose::no) const
        {
            return serum_circle_empirical(antigens, aSerumNo, *projection(aProjectionNo)->layout(), column_basis(aSerumNo, aProjectionNo), *titers(), fold, verbose);
        }
        SerumCircle serum_circle_radius_theoretical(const Indexes& antigens, Titer aHomologousTiter, size_t aSerumNo, size_t aProjectionNo, double fold = 2) const
        {
            return serum_circle_theoretical(antigens, aHomologousTiter, aSerumNo, column_basis(aSerumNo, aProjectionNo), fold);
        }
        SerumCircle serum_circle_radius_theoretical(size_t aAntigenNo, size_t aSerumNo, size_t aProjectionNo, double fold = 2) const
        {
            return serum_circle_theoretical(aAntigenNo, aSerumNo, column_basis(aSerumNo, aProjectionNo), *titers(), fold);
        }
        SerumCircle serum_circle_radius_theoretical(const Indexes& antigens, size_t aSerumNo, size_t aProjectionNo, double fold = 2) const
        {
            return serum_circle_theoretical(antigens, aSerumNo, column_basis(aSerumNo, aProjectionNo), *titers(), fold);
        }
        // aWithin4Fold: indices of antigens within 4fold from homologous titer
        // aOutside4Fold: indices of antigens with titers against aSerumNo outside 4fold distance from homologous titer
        void serum_coverage(size_t aAntigenNo, size_t aSerumNo, Indexes& aWithinFold, Indexes& aOutsideFold, double aFold = 2) const; // aFold=2 for 4fold, 3 - for 8fold
        void serum_coverage(Titer aHomologousTiter, size_t aSerumNo, Indexes& aWithinFold, Indexes& aOutsideFold, double aFold = 2) const; // aFold=2 for 4fold, 3 - for 8fold

        void set_homologous(find_homologous options, std::shared_ptr<Sera> aSera = nullptr, acmacs::debug dbg = acmacs::debug::no) const;

        Stress make_stress(const Projection& projection, multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const
        {
            return stress_factory(projection, mult);
        }

        Stress make_stress(size_t aProjectionNo) const { return make_stress(*projection(aProjectionNo)); }

        void show_table(std::ostream& output, std::optional<size_t> layer_no = {}) const;

      private:
        mutable std::map<MinimumColumnBasis, std::shared_ptr<ColumnBases>> computed_column_bases_; // cache, computing might be slow for big charts

    }; // class Chart

    using ChartP = std::shared_ptr<Chart>;
    using AntigenP = std::shared_ptr<Antigen>;
    using SerumP = std::shared_ptr<Serum>;
    using AntigensP = std::shared_ptr<Antigens>;
    using SeraP = std::shared_ptr<Sera>;
    using InfoP = std::shared_ptr<Info>;
    using TitersP = std::shared_ptr<Titers>;
    using ColumnBasesP = std::shared_ptr<ColumnBases>;
    using ProjectionP = std::shared_ptr<Projection>;
    using ProjectionsP = std::shared_ptr<Projections>;
    using PlotSpecP = std::shared_ptr<PlotSpec>;

    inline double Projection::calculate_stress(multiply_antigen_titer_until_column_adjust mult) const { return calculate_stress(stress_factory(*this, mult)); }

    inline std::vector<double> Projection::calculate_gradient(multiply_antigen_titer_until_column_adjust mult) const
    {
        return calculate_gradient(stress_factory(*this, mult));
    }

    template <typename AgSr, typename = std::enable_if_t<std::is_same_v<AgSr, Antigens> || std::is_same_v<AgSr, Sera>>>
        inline bool equal(const AgSr& a1, const AgSr& a2, bool verbose = false)
    {
        if (a1.size() != a2.size()) {
            if (verbose)
                fmt::print(stderr, "WARNING: number of ag/sr different: {} vs {}\n", a1.size(), a2.size());
            return false;
        }
        for (auto i1 = a1.begin(), i2 = a2.begin(); i1 != a1.end(); ++i1, ++i2) {
            if (**i1 != **i2) {
                if (verbose)
                    fmt::print(stderr, "WARNING: ag/sr different: {} vs {}\n", (*i1)->full_name(), (*i2)->full_name());
                return false;
            }
        }
        return true;
    }

    // returns if sets of antigens, sera are the same in both charts and titers are the same.
    // charts may have different sets of projections and different plot specs
    bool same_tables(const Chart& c1, const Chart& c2, bool verbose = false);

    template <typename AgSr> inline size_t max_full_name(const AgSr& ag_sr)
    {
        return std::accumulate(std::begin(ag_sr), std::end(ag_sr), 0ul, [](size_t max_name, const auto& en) { return std::max(max_name, en->full_name().size()); });
    }

    template <typename AgSr> inline size_t max_full_name(const AgSr& ag_sr, const acmacs::chart::PointIndexList& indexes)
    {
        return std::accumulate(std::begin(indexes), std::end(indexes), 0ul, [&ag_sr](size_t max_name, size_t index) { return std::max(max_name, ag_sr.at(index)->full_name().size()); });
    }

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const acmacs::chart::duplicates_t& dups)
    {
        if (dups.empty())
            return {};
        std::string result{"DUPS:["};
        for (const auto& entry : dups)
            result += to_string(entry) + ", ";
        result.replace(result.size() - 2, 2, "]");
        return result;
    }
}

// ----------------------------------------------------------------------

#ifndef __clang__

template <> struct std::iterator_traits<acmacs::chart::Antigens::iterator>
{
    using reference = typename acmacs::chart::Antigens::iterator::reference;
    using iterator_category = typename acmacs::chart::Antigens::iterator::iterator_category;
    using difference_type = typename acmacs::chart::Antigens::iterator::difference_type;
};

template <> struct std::iterator_traits<acmacs::chart::Sera::iterator>
{
    using reference = typename acmacs::chart::Sera::iterator::reference;
    using iterator_category = typename acmacs::chart::Sera::iterator::iterator_category;
    using difference_type = typename acmacs::chart::Sera::iterator::difference_type;
};

template <> struct std::iterator_traits<acmacs::chart::Projections::iterator>
{
    using reference = typename acmacs::chart::Projections::iterator::reference;
    using iterator_category = typename acmacs::chart::Projections::iterator::iterator_category;
    using difference_type = typename acmacs::chart::Projections::iterator::difference_type;
};

#endif

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::chart::BLineage> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::BLineage& lineage, FormatCtx& ctx) { return format_to(ctx.out(), "{}", lineage.to_string()); }
};


template <> struct fmt::formatter<acmacs::chart::Antigen> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::Antigen& antigen, FormatCtx& ctx)
    {
        format_to(ctx.out(), "{}", antigen.full_name());
        if (const auto date = antigen.date(); !date.empty())
            format_to(ctx.out(), " [{}]", date);
        if (const auto lab_ids = antigen.lab_ids(); !lab_ids->empty())
            format_to(ctx.out(), " {}", lab_ids.join());
        if (const auto lineage = antigen.lineage(); lineage != acmacs::chart::BLineage::Unknown)
            format_to(ctx.out(), " {}", lineage);
        return ctx.out();
    }
};

template <> struct fmt::formatter<acmacs::chart::Serum> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::chart::Serum& serum, FormatCtx& ctx)
    {
        format_to(ctx.out(), "{}", serum.full_name());
        if (const auto lineage = serum.lineage(); lineage != acmacs::chart::BLineage::Unknown)
            format_to(ctx.out(), " {}", lineage);
        if (const auto serum_species = serum.serum_species(); !serum_species.empty())
            format_to(ctx.out(), " {}", serum_species);
        return ctx.out();
    }
};


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
