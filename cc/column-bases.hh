#pragma once

#include <memory>
#include "acmacs-base/float.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-base/indexes.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class MinimumColumnBasis
    {
     public:
        MinimumColumnBasis(double value = 0) : value_{value} {}
        MinimumColumnBasis(const MinimumColumnBasis&) = default;
        MinimumColumnBasis(std::string value) { from(value); }
        MinimumColumnBasis(std::string_view value) { from(value); }
        MinimumColumnBasis(const char* value) { from(value); }
        MinimumColumnBasis& operator=(double value) { value_ = value; return *this; }
        MinimumColumnBasis& operator=(const MinimumColumnBasis&) = default;
        MinimumColumnBasis& operator=(std::string value) { from(value); return *this; }
        bool operator==(const MinimumColumnBasis& rhs) const { return float_equal(value_, rhs.value_); }
        bool operator!=(const MinimumColumnBasis& rhs) const { return !operator==(rhs); }

        constexpr bool is_none() const { return float_zero(value_); }

        constexpr operator double() const noexcept { return value_; }
        double apply(double column_basis) const noexcept { return std::max(column_basis, value_); }

        operator std::string() const noexcept;

     private:
        double value_;

        void from(std::string_view value);

    }; // class MinimumColumnBasis

// ----------------------------------------------------------------------

    class ColumnBases
    {
      public:
        virtual ~ColumnBases() = default;
        ColumnBases() = default;
        ColumnBases(const ColumnBases&) = default;

        virtual double column_basis(size_t aSerumNo) const = 0;
        virtual size_t size() const = 0;

        virtual std::vector<double> data() const
        {
            std::vector<double> result(size());
            for (size_t i = 0; i < size(); ++i)
                result[i] = column_basis(i);
            return result;
        }

    }; // class ColumnBases

// ----------------------------------------------------------------------

    class ColumnBasesData : public ColumnBases
    {
      public:
        ColumnBasesData(size_t number_of_sera, double aMinimumColumnBasis = 0.0) : data_(number_of_sera, aMinimumColumnBasis) {}
        ColumnBasesData(const ColumnBases& aSource) : data_(aSource.size()) { for (size_t serum_no = 0; serum_no < data_.size(); ++serum_no) data_[serum_no] = aSource.column_basis(serum_no); }

        double column_basis(size_t aSerumNo) const override { return data_.at(aSerumNo); }
        size_t size() const override { return data_.size(); }
        std::vector<double> data() const override { return data_; }
        std::vector<double>& data() { return data_; }

        void set(size_t aSerumNo, double column_basis) { data_.at(aSerumNo) = column_basis; }
        void remove(const ReverseSortedIndexes& indexes, ReverseSortedIndexes::difference_type base_index = 0) { acmacs::remove(indexes, data_, base_index); }
        void insert(size_t before, double value) { data_.insert(data_.begin() + static_cast<decltype(data_)::difference_type>(before), value); }

     private:
        std::vector<double> data_;

    }; // class ColumnBases

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    std::string to_string(const acmacs::chart::ColumnBases& aColumnBases);

    inline std::string to_string(std::shared_ptr<acmacs::chart::ColumnBases> aColumnBases)
    {
        if (aColumnBases)
            return to_string(*aColumnBases);
        else
            return "<none>";
    }

    inline std::string to_string(acmacs::chart::MinimumColumnBasis aMinimumColumnBasis)
    {
        return static_cast<std::string>(aMinimumColumnBasis);
    }

} // namespace acmacs

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::chart::ColumnBases> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const acmacs::chart::ColumnBases& cb, FormatCtx& ctx) { return fmt::formatter<std::string>::format(acmacs::to_string(cb), ctx); }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
