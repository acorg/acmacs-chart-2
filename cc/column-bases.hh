#pragma once

#include <memory>
#include "acmacs-base/float.hh"
#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class MinimumColumnBasis
    {
     public:
        inline MinimumColumnBasis(double aValue = 0) : mValue{aValue} {}
        inline MinimumColumnBasis(const MinimumColumnBasis&) = default;
        inline MinimumColumnBasis(std::string aValue) { from(aValue); }
        inline MinimumColumnBasis& operator=(double aValue) { mValue = aValue; return *this; }
        inline MinimumColumnBasis& operator=(const MinimumColumnBasis&) = default;
        inline MinimumColumnBasis& operator=(std::string aValue) { from(aValue); return *this; }

        inline constexpr bool is_none() const { return float_zero(mValue); }

        inline constexpr operator double() const noexcept { return mValue; }

        inline operator std::string() const noexcept
            {
                if (is_none())
                    return "none";
                else if (float_equal(mValue, 7.0))
                    return "1280";
                else
                    return acmacs::to_string(mValue);
            }

     private:
        double mValue;

        inline void from(std::string aValue)
            {
                if (aValue.empty() || aValue == "none")
                    mValue = 0;
                else if (aValue == "1280")
                    mValue = 7;
                else
                    throw std::runtime_error{"Unrecognized minimum_column_basis value: " + aValue};
            }

    }; // class MinimumColumnBasis

// ----------------------------------------------------------------------

    class ColumnBases
    {
      public:
        virtual ~ColumnBases();
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
