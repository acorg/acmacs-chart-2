#pragma once

#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class GridTest
    {
      public:
        using Chart = acmacs::chart::ChartModify;
        using Projection = acmacs::chart::ProjectionModifyP;
        using Coordinates = acmacs::Coordinates;
        using Stress = acmacs::chart::Stress<double>;

        GridTest(Chart& chart, size_t projection_no, double grid_step)
            : chart_(chart), projection_(chart.projection_modify(projection_no)), grid_step_(grid_step), original_layout_(*projection_->layout()), stress_(chart.make_stress<double>(projection_no)) {}

        struct Result
        {
            enum diagnosis_t { excluded, not_tested, normal, trapped, hemisphering };

            Result(size_t a_point_no) : point_no(a_point_no), diagnosis(not_tested) {}
            Result(size_t a_point_no, diagnosis_t a_diagnosis, const Coordinates& a_pos, double a_distance, double diff)
                : point_no(a_point_no), diagnosis(a_diagnosis), pos(a_pos), distance(a_distance), contribution_diff(diff) {}
            operator bool() const { return diagnosis == trapped || diagnosis == hemisphering; }
            std::string report() const;

            size_t point_no;
            diagnosis_t diagnosis;
            Coordinates pos;
            double distance;
            double contribution_diff;
        };

        struct Results : public std::vector<Result>
        {
            using std::vector<Result>::vector;
        };

        std::string point_name(size_t point_no) const;
        Result test_point(size_t point_no);
        void test_point(Result& result);
        Results test_all();
        std::string report(const Results& results) const;
        Projection make_new_projection_and_relax(const Results& results);

      private:
        Chart& chart_;
        Projection projection_;
        const double grid_step_;                             // acmacs-c2: 0.01
        const double hemisphering_distance_threshold_ = 1.0; // from acmacs-c2 hemi-local test: 1.0
        const double hemisphering_stress_threshold_ = 0.25;  // stress diff within threshold -> hemisphering, from acmacs-c2 hemi-local test: 0.25
        const acmacs::Layout original_layout_;
        const Stress stress_;
        static constexpr auto optimization_method_ = acmacs::chart::optimization_method::alglib_cg_pca;

        bool antigen(size_t point_no) const { return point_no < chart_.number_of_antigens(); }
        size_t antigen_serum_no(size_t point_no) const { return antigen(point_no) ? point_no : (point_no - chart_.number_of_antigens()); }
        // acmacs::Area area_for(size_t point_no) const;
        acmacs::Area area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const;

    }; // class GridTest

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
