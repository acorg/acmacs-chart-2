#include "acmacs-base/omp.hh"
#include "acmacs-chart-2/grid-test.hh"

// ----------------------------------------------------------------------

#ifndef __clang__
// workaround for a bug in gcc 7.2
static std::vector<acmacs::chart::GridTest::Result> _gcc72_bug;
#endif

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::point_name(size_t point_no) const
{
    if (antigen(point_no)) {
        return "AG " + acmacs::to_string(point_no) + ' ' + (*chart_.antigens())[point_no]->full_name();
    }
    else {
        const auto serum_no = antigen_serum_no(point_no);
        return "SR " + acmacs::to_string(serum_no) + ' ' + (*chart_.sera())[serum_no]->full_name();
    }

} // acmacs::chart::GridTest::point_name

// ----------------------------------------------------------------------

acmacs::Area acmacs::chart::GridTest::area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const
{
    acmacs::Area result(original_layout_.get(table_distances_for_point.regular.empty() ? table_distances_for_point.less_than.front().another_point : table_distances_for_point.regular.front().another_point));
    auto extend = [&result,this](const auto& entry) {
        const auto coord = this->original_layout_.get(entry.another_point);
        const auto radius = entry.distance; // + 1;
        result.extend(coord - radius);
        result.extend(coord + radius);
    };
    std::for_each(table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), extend);
    std::for_each(table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), extend);
    return result;

} // acmacs::chart::GridTest::area_for

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Result acmacs::chart::GridTest::test_point(size_t point_no)
{
    Result result(point_no);
    test_point(result);
    return result;

} // acmacs::chart::GridTest::test_point

// ----------------------------------------------------------------------

void acmacs::chart::GridTest::test_point(Result& result)
{
    if (result.diagnosis == Result::not_tested) {
        result.diagnosis = Result::normal;

        acmacs::Layout layout(original_layout_);
        const auto table_distances_for_point = stress_.table_distances_for(result.point_no);
        const auto target_contribution = stress_.contribution(result.point_no, table_distances_for_point, layout.data());
        const auto original_pos = original_layout_.get(result.point_no);
        auto best_contribution = target_contribution;
        Coordinates best_coord, hemisphering_coord;
        const auto hemisphering_stress_threshold_rough = hemisphering_stress_threshold_ * 2;
        auto hemisphering_contribution = target_contribution + hemisphering_stress_threshold_rough;
        const auto area = area_for(table_distances_for_point);
        for (auto it = area.begin(grid_step_), last = area.end(); it != last; ++it) {
            layout.set(result.point_no, *it);
            const auto contribution = stress_.contribution(result.point_no, table_distances_for_point, layout.data());
            if (contribution < best_contribution) {
                best_contribution = contribution;
                best_coord = *it;
            }
            else if (best_coord.empty() && contribution < hemisphering_contribution && original_pos.distance(*it) > hemisphering_distance_threshold_) {
                hemisphering_contribution = contribution;
                hemisphering_coord = *it;
            }
        }
        if (!best_coord.empty()) {
            layout.set(result.point_no, best_coord);
            const auto status = acmacs::chart::optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
            result.pos = layout.get(result.point_no);
            result.distance = original_pos.distance(result.pos);
            result.contribution_diff = status.final_stress - projection_->stress();
            result.diagnosis = std::abs(result.contribution_diff) > hemisphering_stress_threshold_ ? Result::trapped : Result::hemisphering;
        }
        else if (!hemisphering_coord.empty()) {
            // relax to find real contribution
            layout.set(result.point_no, hemisphering_coord);
            auto status = acmacs::chart::optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
            result.pos = layout.get(result.point_no);
            result.distance = original_pos.distance(result.pos);
            if (result.distance > hemisphering_distance_threshold_ && result.distance < (hemisphering_distance_threshold_ * 1.2)) {
                status = acmacs::chart::optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::fine);
                result.pos = layout.get(result.point_no);
                result.distance = original_pos.distance(result.pos);
            }
            result.contribution_diff = status.final_stress - projection_->stress();
            if (result.distance > hemisphering_distance_threshold_) {
                // if (const auto real_contribution_diff = stress_.contribution(result.point_no, table_distances_for_point, layout.data()) - target_contribution;
                //     real_contribution_diff < hemisphering_stress_threshold_) {
                if (std::abs(result.contribution_diff) < hemisphering_stress_threshold_) {
                    // result.contribution_diff = real_contribution_diff;
                    result.diagnosis = Result::hemisphering;
                }
            }
        }
    }

} // acmacs::chart::GridTest::test_point

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Results acmacs::chart::GridTest::test_all_prepare()
{
      // std::cerr << "stress: " << stress_.value(original_layout_.data()) << '\n';
    Results result(acmacs::index_iterator(0UL), acmacs::index_iterator(chart_.number_of_points()));
    for (auto unmovable : projection_->unmovable())
        result[unmovable].diagnosis = Result::excluded;
    for (auto disconnected : projection_->disconnected())
        result[disconnected].diagnosis = Result::excluded;
    result.erase(std::remove_if(result.begin(), result.end(), [](const auto& entry) { return entry.diagnosis == Result::excluded; }), result.end());
    return result;

} // acmacs::chart::GridTest::test_all_prepare

// ----------------------------------------------------------------------

// acmacs::chart::GridTest::Results acmacs::chart::GridTest::test_all()
// {
//     auto result = test_all_prepare();
//     for (size_t entry_no = 0; entry_no < result.size(); ++entry_no) {
//         test_point(result[entry_no]);
//     }
//     return result;

// } // acmacs::chart::GridTest::test_all

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Results acmacs::chart::GridTest::test_all_parallel(int threads)
{
    auto result = test_all_prepare();

    const int num_threads = threads <= 0 ? omp_get_max_threads() : threads;
    const int slot_size = chart_.number_of_antigens() < 1000 ? 4 : 1;
#pragma omp parallel for default(none) shared(result) num_threads(num_threads) schedule(static, slot_size)
    for (size_t entry_no = 0; entry_no < result.size(); ++entry_no) {
        test_point(result[entry_no]);
    }

    return result;

} // acmacs::chart::GridTest::test_all_parallel

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Projection acmacs::chart::GridTest::make_new_projection_and_relax(const Results& results, bool verbose)
{
    auto projection = chart_.projections_modify()->new_by_cloning(*projection_);
    auto layout = projection->layout_modified();
    for (const auto& result : results) {
        if (result && result.contribution_diff < 0) {
            layout->set(result.point_no, result.pos);
        }
    }
    const auto status = acmacs::chart::optimize(optimization_method_, stress_, layout->data(), layout->data() + layout->size(), acmacs::chart::optimization_precision::fine);
    if (verbose)
        std::cout << "stress: " << projection_->stress() << " --> " << status.final_stress << '\n';
    return std::move(projection);

} // acmacs::chart::GridTest::make_new_projection_and_relax

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::Results::report() const
{
    size_t trapped = 0, hemi = 0;
    std::for_each(begin(), end(), [&](const auto& r) { if (r.diagnosis == Result::trapped) ++trapped; else if (r.diagnosis == Result::hemisphering) ++hemi; });
    if (trapped || hemi)
        return "trapped:" + std::to_string(trapped) + " hemisphering:" + std::to_string(hemi);
    else
        return "nothing found";

} // acmacs::chart::GridTest::Results::report

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::Result::report(const Chart& chart) const
{
    std::string diag;
    switch (diagnosis) {
      case excluded:
          diag = "excl";
          break;
      case not_tested:
          diag = "nott";
          break;
      case normal:
          diag = "norm";
          break;
      case trapped:
          diag = "trap";
          break;
      case hemisphering:
          diag = "hemi";
          break;
    }
    std::string name;
    if (const auto num_ags = chart.number_of_antigens(); point_no < num_ags)
        name = string::concat("[AG ", point_no, ' ', chart.antigens()->at(point_no)->full_name(), ']');
    else
        name = string::concat("[SR ", point_no - num_ags, ' ', chart.sera()->at(point_no - num_ags)->full_name(), ']');
    return string::concat(diag, ' ', name, " diff:", acmacs::to_string(contribution_diff, 4), " dist:", acmacs::to_string(distance, 4));

} // acmacs::chart::GridTest::Result::report

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::Result::diagnosis_str() const
{
    switch (diagnosis) {
      case excluded:
          return "excluded";
      case not_tested:
          return "not_tested";
      case normal:
          return "normal";
      case trapped:
          return "trapped";
      case hemisphering:
          return "hemisphering";
    }
    return "not_tested";

} // acmacs::chart::GridTest::Result::diagnosis_str

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
