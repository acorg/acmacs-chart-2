#pragma once

#include <memory>
#include "acmacs-base/float.hh"
#include "acmacs-base/string.hh"
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
        // ColumnBases(const ColumnBases&) = delete;

        virtual double column_basis(size_t aSerumNo) const = 0;
        virtual size_t size() const = 0;
        
    }; // class ColumnBases

// ----------------------------------------------------------------------

    class ColumnBasesData : public ColumnBases
    {
      public:
        ColumnBasesData(size_t number_of_sera) : data_(number_of_sera, 0) {}
        ColumnBasesData(const ColumnBases& aSource) : data_(aSource.size()) { for (size_t serum_no = 0; serum_no < data_.size(); ++serum_no) data_[serum_no] = aSource.column_basis(serum_no); }

        virtual double column_basis(size_t aSerumNo) const { return data_.at(aSerumNo); }
        virtual size_t size() const { return data_.size(); }

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

    inline std::string to_string(const acmacs::chart::MinimumColumnBasis& aMinimumColumnBasis)
    {
        return static_cast<std::string>(aMinimumColumnBasis);
    }

} // namespace acmacs

namespace acmacs::chart
{
    inline std::ostream& operator<<(std::ostream& out, const ColumnBases& cb)
    {
        return out << acmacs::to_string(cb);
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
