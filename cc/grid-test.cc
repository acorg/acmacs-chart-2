#include "acmacs-base/omp.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-base/enumerate.hh"
#include "acmacs-base/to-json.hh"
#include "acmacs-base/data-formatter.hh"
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
    size_t another_point;
    if (!table_distances_for_point.regular.empty())
        another_point = table_distances_for_point.regular.front().another_point;
    else if (!table_distances_for_point.less_than.empty())
        another_point = table_distances_for_point.less_than.front().another_point;
    else
        throw std::runtime_error("acmacs::chart::GridTest::area_for: table_distances_for_point has neither regulr nor less_than entries");
    acmacs::Area result(original_layout_.at(another_point));
    auto extend = [&result,this](const auto& entry) {
        const auto coord = this->original_layout_.at(entry.another_point);
        const auto radius = entry.distance; // + 1;
        result.extend(coord - radius);
        result.extend(coord + radius);
    };
    std::for_each(table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), extend);
    std::for_each(table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), extend);
    return result;

} // acmacs::chart::GridTest::area_for

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Result acmacs::chart::GridTest::test(size_t point_no)
{
    Result result(point_no, original_layout_.number_of_dimensions());
    test(result);
    return result;

} // acmacs::chart::GridTest::test_point

// ----------------------------------------------------------------------

void acmacs::chart::GridTest::test(Result& result)
{
    if (result.diagnosis == Result::not_tested) {
        const auto table_distances_for_point = stress_.table_distances_for(result.point_no);
        if (table_distances_for_point.empty()) { // no table distances, cannot test
            result.diagnosis = Result::excluded;
            return;
        }

        result.diagnosis = Result::normal;

        acmacs::Layout layout(original_layout_);
        const auto target_contribution = stress_.contribution(result.point_no, table_distances_for_point, layout.data());
        const auto original_pos = original_layout_.at(result.point_no);
        auto best_contribution = target_contribution;
        PointCoordinates best_coord(original_pos.number_of_dimensions()),
                hemisphering_coord(original_pos.number_of_dimensions());
        const auto hemisphering_stress_threshold_rough = hemisphering_stress_threshold_ * 2;
        auto hemisphering_contribution = target_contribution + hemisphering_stress_threshold_rough;
        const auto area = area_for(table_distances_for_point);
        for (auto it = area.begin(grid_step_), last = area.end(); it != last; ++it) {
            layout.update(result.point_no, *it);
            const auto contribution = stress_.contribution(result.point_no, table_distances_for_point, layout.data());
            if (contribution < best_contribution) {
                best_contribution = contribution;
                best_coord = *it;
            }
            else if (!best_coord.exists() && contribution < hemisphering_contribution && distance(original_pos, *it) > hemisphering_distance_threshold_) {
                hemisphering_contribution = contribution;
                hemisphering_coord = *it;
            }
        }
        if (best_coord.exists()) {
            layout.update(result.point_no, best_coord);
            const auto status = acmacs::chart::optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
            result.pos = layout.at(result.point_no);
            result.distance = distance(original_pos, result.pos);
            result.contribution_diff = status.final_stress - projection_->stress();
            result.diagnosis = std::abs(result.contribution_diff) > hemisphering_stress_threshold_ ? Result::trapped : Result::hemisphering;
        }
        else if (hemisphering_coord.exists()) {
            // relax to find real contribution
            layout.update(result.point_no, hemisphering_coord);
            auto status = acmacs::chart::optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::rough);
            result.pos = layout.at(result.point_no);
            result.distance = distance(original_pos, result.pos);
            if (result.distance > hemisphering_distance_threshold_ && result.distance < (hemisphering_distance_threshold_ * 1.2)) {
                status = acmacs::chart::optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), acmacs::chart::optimization_precision::fine);
                result.pos = layout.at(result.point_no);
                result.distance = distance(original_pos, result.pos);
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

} // acmacs::chart::GridTest::test

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Results acmacs::chart::GridTest::test(const std::vector<size_t>& points, [[maybe_unused]] int threads)
{
    Results results(points, *projection_);

#pragma omp parallel for default(none) shared(results) num_threads(threads <= 0 ? omp_get_max_threads() : threads) schedule(static, chart_.number_of_antigens() < 1000 ? 4 : 1)
    for (size_t entry_no = 0; entry_no < results.size(); ++entry_no) {
        test(results[entry_no]);
    }

    return results;

} // acmacs::chart::GridTest::test

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Results acmacs::chart::GridTest::test_all([[maybe_unused]] int threads)
{
    Results results(*projection_);

#pragma omp parallel for default(none) shared(results) num_threads(threads <= 0 ? omp_get_max_threads() : threads) schedule(static, chart_.number_of_antigens() < 1000 ? 4 : 1)
    for (size_t entry_no = 0; entry_no < results.size(); ++entry_no) {
        test(results[entry_no]);
    }

    return results;

} // acmacs::chart::GridTest::test_all

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Projection acmacs::chart::GridTest::make_new_projection_and_relax(const Results& results, bool verbose)
{
    auto projection = chart_.projections_modify()->new_by_cloning(*projection_);
    auto layout = projection->layout_modified();
    for (const auto& result : results) {
        if (result && result.contribution_diff < 0) {
            layout->update(result.point_no, result.pos);
        }
    }
    const auto status = acmacs::chart::optimize(optimization_method_, stress_, layout->data(), layout->data() + layout->size(), acmacs::chart::optimization_precision::fine);
    if (verbose)
        std::cout << "stress: " << projection_->stress() << " --> " << status.final_stress << '\n';
    return projection;

} // acmacs::chart::GridTest::make_new_projection_and_relax

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::Results::report() const
{
    size_t trapped = 0, hemi = 0;
    std::for_each(begin(), end(), [&](const auto& r) { if (r.diagnosis == Result::trapped) ++trapped; else if (r.diagnosis == Result::hemisphering) ++hemi; });
    if (trapped || hemi)
        return fmt::format("trapped:{} hemisphering:{}", trapped, hemi);
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
        name = fmt::format("[AG {:5d} {}]", point_no, chart.antigens()->at(point_no)->full_name());
    else
        name = fmt::format("[SR {:5d} {}]", point_no - num_ags, chart.sera()->at(point_no - num_ags)->full_name());
    return fmt::format("{} {} diff:{:.4f} dist:{:.4f}", diag, name, contribution_diff, distance);

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

acmacs::chart::GridTest::Results::Results(const acmacs::chart::Projection& projection)
    : std::vector<Result>(projection.number_of_points(), Result(0, projection.number_of_dimensions()))
{
    size_t point_no = 0;
    for (auto& res : *this)
        res.point_no = point_no++;
    exclude_disconnected(projection);

} // acmacs::chart::GridTest::Results::Results

// ----------------------------------------------------------------------

acmacs::chart::GridTest::Results::Results(const std::vector<size_t>& points, const acmacs::chart::Projection& projection)
    : std::vector<Result>(points.size(), Result(0, projection.number_of_dimensions()))
{
    for (auto [index, point_no] : acmacs::enumerate(points))
        at(index).point_no = point_no;
    exclude_disconnected(projection);

} // acmacs::chart::GridTest::Results::Results

// ----------------------------------------------------------------------

void acmacs::chart::GridTest::Results::exclude_disconnected(const acmacs::chart::Projection& projection)
{
    const auto exclude = [this](size_t point_no) {
        if (auto* found = find(point_no); found)
            found->diagnosis = Result::excluded;
    };

    for (auto unmovable : projection.unmovable())
        exclude(unmovable);
    for (auto disconnected : projection.disconnected())
        exclude(disconnected);
    erase(std::remove_if(begin(), end(), [](const auto& entry) { return entry.diagnosis == Result::excluded; }), end());

} // acmacs::chart::GridTest::Results::exclude_disconnected

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::Results::export_to_json(const Chart& chart) const
{
    const auto export_point = [&chart](const auto& en) -> to_json::object {
        return to_json::object{
            to_json::key_val{"point_no", en.point_no},
            to_json::key_val{"name", en.point_no < chart.number_of_antigens() ? chart.antigen(en.point_no)->full_name() : chart.serum(en.point_no - chart.number_of_antigens())->full_name()},
            to_json::key_val{"distance", en.distance},
            to_json::key_val{"contribution_diff", en.contribution_diff},
            to_json::key_val{"pos", to_json::array(en.pos.begin(), en.pos.end())},
        };
    };

    to_json::array hemisphering, trapped, tested;
    for (const auto& en : *this) {
        tested << en.point_no;
        switch (en.diagnosis) {
          case Result::trapped:
              trapped << export_point(en);
              break;
          case Result::hemisphering:
              hemisphering << export_point(en);
              break;
          case Result::excluded:
          case Result::not_tested:
          case Result::normal:
              break;
        }
    }

    return fmt::format("{}\n", to_json::object{
            to_json::key_val{"  version", "grid-test-v1"},
            to_json::key_val{"chart", chart.make_name()},
            // to_json::key_val{"tested", std::move(tested)},
            to_json::key_val{"hemisphering", std::move(hemisphering)},
            to_json::key_val{"trapped", std::move(trapped)}
        });

} // acmacs::chart::GridTest::Results::export_to_json

// ----------------------------------------------------------------------

std::string acmacs::chart::GridTest::Results::export_to_layout_csv(const Chart& chart, const acmacs::chart::Projection& projection) const
{
    using DF = acmacs::DataFormatterCSV;

    auto antigens = chart.antigens();
    const auto number_of_antigens = antigens->size();
    auto sera = chart.sera();
    auto layout = projection.layout();
    const auto number_of_dimensions = layout->number_of_dimensions();

    std::string result;

    const auto add = [&](size_t point_no) {
        for (auto dim : acmacs::range<number_of_dimensions_t>(number_of_dimensions))
            DF::second_field(result, (*layout)(point_no, dim));
        if (const auto* res = find(point_no); res) {
            DF::second_field(result, res->diagnosis_str());
            DF::second_field(result, res->distance);
            DF::second_field(result, res->contribution_diff);
            for (auto dim : acmacs::range<number_of_dimensions_t>(number_of_dimensions))
                DF::second_field(result, res->pos[dim]);
        }
        else {
            DF::second_field(result, "not-tested");
            DF::second_field(result, "");
            DF::second_field(result, "");
        }
    };

    DF::first_field(result, "");
    DF::second_field(result, "name");
    for (auto dim : acmacs::range<number_of_dimensions_t>(number_of_dimensions))
        DF::second_field(result, fmt::format("coord {}", dim));
    DF::second_field(result, "hemisphering");
    DF::second_field(result, "distance");
    DF::second_field(result, "contribution_diff");
    for (auto dim : acmacs::range<number_of_dimensions_t>(number_of_dimensions))
        DF::second_field(result, fmt::format("other coord {}", dim));
    DF::end_of_record(result);

    for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
        DF::first_field(result, "AG");
        DF::second_field(result, antigen->full_name());
        add(ag_no);
        DF::end_of_record(result);
    }

    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        DF::first_field(result, "SR");
        DF::second_field(result, serum->full_name());
        add(sr_no + number_of_antigens);
        DF::end_of_record(result);
    }

    return result;

} // acmacs::chart::GridTest::Results::export_to_layout_csv

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
