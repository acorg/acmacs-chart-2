#pragma once

#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class GridTest
    {
      public:
        using Chart = acmacs::chart::ChartModify;
        using Projection = acmacs::chart::ProjectionModifyP;

        GridTest(Chart& chart, size_t projection_no, double grid_step)
            : chart_(chart), projection_(chart.projection_modify(projection_no)), grid_step_(grid_step), original_layout_(*projection_->layout()), stress_(chart.make_stress(projection_no)) {}
        void reset(Projection projection) { projection_ = projection; original_layout_ = *projection_->layout(); stress_ = chart_.make_stress(projection_->projection_no()); }

        struct Result
        {
            enum diagnosis_t { excluded, not_tested, normal, trapped, hemisphering };

            Result(size_t a_point_no, number_of_dimensions_t number_of_dimensions) : point_no(a_point_no), diagnosis(not_tested), pos(number_of_dimensions) {}
            Result(size_t a_point_no, diagnosis_t a_diagnosis, const PointCoordinates& a_pos, double a_distance, double diff)
                : point_no(a_point_no), diagnosis(a_diagnosis), pos(a_pos), distance(a_distance), contribution_diff(diff) {}
            operator bool() const { return diagnosis == trapped || diagnosis == hemisphering; }
            std::string report(const Chart& chart) const;
            std::string diagnosis_str() const;

            size_t point_no;
            diagnosis_t diagnosis;
            PointCoordinates pos;
            double distance;
            double contribution_diff;
        };

        class Results : public std::vector<Result>
        {
         public:
            Results() = default;
            Results(const acmacs::chart::Projection& projection);
            Results(const std::vector<size_t>& points, const acmacs::chart::Projection& projection);

            std::string report() const;
            std::string export_to_json(const Chart& chart) const;
            auto count_trapped_hemisphering() const { return std::count_if(begin(), end(), [](const auto& r) { return r.diagnosis == Result::trapped || r.diagnosis == Result::hemisphering; }); }
            number_of_dimensions_t num_dimensions() const { return front().pos.number_of_dimensions(); }

          private:
            void exclude_disconnected(const acmacs::chart::Projection& projection);
            Result* find(size_t point_no)
            {
                if (const auto found = std::find_if(begin(), end(), [point_no](const auto& en) { return en.point_no == point_no; }); found != end())
                    return &*found;
                else
                    return nullptr;
            }
        };

        std::string point_name(size_t point_no) const;
        Result test(size_t point_no);
        Results test(const std::vector<size_t>& points, int threads = 0);
        Results test_all(int threads = 0);
        Projection make_new_projection_and_relax(const Results& results, bool verbose);

      private:
        Chart& chart_;
        Projection projection_;
        const double grid_step_;                             // acmacs-c2: 0.01
        const double hemisphering_distance_threshold_ = 1.0; // from acmacs-c2 hemi-local test: 1.0
        const double hemisphering_stress_threshold_ = 0.25;  // stress diff within threshold -> hemisphering, from acmacs-c2 hemi-local test: 0.25
        acmacs::Layout original_layout_;
        Stress stress_;
        static constexpr auto optimization_method_ = acmacs::chart::optimization_method::alglib_cg_pca;

        void test(Result& result);
        bool antigen(size_t point_no) const { return point_no < chart_.number_of_antigens(); }
        size_t antigen_serum_no(size_t point_no) const { return antigen(point_no) ? point_no : (point_no - chart_.number_of_antigens()); }
        // acmacs::Area area_for(size_t point_no) const;
        acmacs::Area area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const;

    }; // class GridTest::chart

} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
