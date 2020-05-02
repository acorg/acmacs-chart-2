#include <iostream>
#include <algorithm>

#include "acmacs-base/argv.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/factory-export.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

inline void extend(std::vector<size_t>& target, std::vector<size_t>&& source, size_t aIncrementEach, size_t aMax)
{
    std::transform(source.begin(), source.end(), std::back_inserter(target), [aIncrementEach,aMax](auto v) {
        if (v >= aMax)
            throw std::runtime_error("invalid index " + acmacs::to_string(v) + ", expected in range 0.." + acmacs::to_string(aMax - 1) + " inclusive");
        return v + aIncrementEach;
    });
}

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> points{*this, "points", desc{"comma or space separated list of point indexes, no spaces!"}};
    option<str> antigens{*this, "antigens", desc{"comma or space separated list of antigen indexes, no spaces!"}};
    option<str> sera{*this, "sera", desc{"comma or space separated list of serum indexes, no spaces!"}};
    option<double> size{*this, "size", desc{"change point size"}};
    option<str> fill{*this, "fill", desc{"change point fill color"}};
    option<str> outline{*this, "outline", desc{"change point outline color"}};

    argument<str> input_chart{*this, arg_name{"input-chart"}, mandatory};
    argument<str> output_chart{*this, arg_name{"output-chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);

        auto chart = acmacs::chart::import_from_file(opt.input_chart);
        std::cout << chart->make_info() << '\n';

        std::vector<size_t> points;
        if (opt.points)
            extend(points, acmacs::string::split_into_size_t(static_cast<std::string_view>(opt.points)), 0, chart->number_of_points());
        if (opt.antigens)
            extend(points, acmacs::string::split_into_size_t(static_cast<std::string_view>(opt.antigens)), 0, chart->number_of_antigens());
        if (opt.sera)
            extend(points, acmacs::string::split_into_size_t(static_cast<std::string_view>(opt.sera)), chart->number_of_antigens(), chart->number_of_sera());
        std::sort(points.begin(), points.end());
        points.erase(std::unique(points.begin(), points.end()), points.end());
        if (points.empty())
            throw std::runtime_error("No point indexes were provided, use --points, --antigens, --sera");
        // else if (!points.empty() && points.back() >= chart->number_of_points())
        //     throw std::runtime_error("Invalid point index: " + acmacs::to_string(points.back()) + ", expected in range 0.." + acmacs::to_string(chart->number_of_points() - 1) + " inclusive");
        std::cerr << "points: " << points << '\n';

        acmacs::chart::ChartModify chart_modify(chart);
        auto plot_spec = chart_modify.plot_spec_modify();

        if (opt.size) {
            for (auto point_no : points)
                plot_spec->size(point_no, Pixels{*opt.size});
        }
        if (opt.fill) {
            for (auto point_no : points)
                plot_spec->fill(point_no, Color(opt.fill));
        }
        if (opt.outline) {
            for (auto point_no : points)
                plot_spec->outline(point_no, Color(opt.outline));
        }

        acmacs::chart::export_factory(chart_modify, opt.output_chart, opt.program_name());
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
