#include "acmacs-base/argv.hh"
#include "acmacs-base/log.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

static double measure(acmacs::chart::ProjectionP projection, const acmacs::chart::Stress& stress);
static double measure_gradient(acmacs::chart::ProjectionP projection, const acmacs::chart::Stress& stress);

// ----------------------------------------------------------------------

double measure(acmacs::chart::ProjectionP projection, const acmacs::chart::Stress& stress)
{
    constexpr const double test_duration{10};
    const auto start = acmacs::timestamp();
    double duration{0};
    size_t count = 0;
    for (; duration < test_duration; duration = acmacs::elapsed_seconds(start)) {
        for (size_t i = 0; i < 300; ++i, ++count)
            projection->calculate_stress(stress);
    }
    return static_cast<double>(count) / duration;
}

double measure_gradient(acmacs::chart::ProjectionP projection, const acmacs::chart::Stress& stress)
{
    constexpr const double test_duration{10};
    const auto start = acmacs::timestamp();
    double duration{0};
    size_t count = 0;
    for (; duration < test_duration; duration = acmacs::elapsed_seconds(start)) {
        for (size_t i = 0; i < 300; ++i, ++count)
            projection->calculate_gradient(stress);
    }
    return static_cast<double>(count) / duration;
}

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<size_t>  projection{*this, "projection", dflt{0ul}};
    option<bool>  stress_double{*this, "double", desc{"just report stress (double)"}};
    option<bool>  gradient{*this, "gradient", desc{"just report gradient (double)"}};
    option<bool>  gradient_max{*this, "gradient-max", desc{"just report gradient max (double)"}};
    option<size_t> precision{*this, "precision", dflt{17ul}, desc{"stress/gradient report precision"}};
    option<bool>  time{*this, "time", desc{"test speed"}};

    argument<str> chart{*this, arg_name{"chart"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        auto chart = acmacs::chart::import_from_file(opt.chart);
        auto projection = chart->projection(opt.projection);
        if (opt.stress_double) {
            fmt::print("{:.{}f}\n", projection->calculate_stress(), *opt.precision);
        }
        else if (opt.gradient) {
            fmt::print("{}\n", projection->calculate_gradient());
        }
        else if (opt.gradient_max) {
            const auto gradient = projection->calculate_gradient();
            const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
            fmt::print("{:.{}f}\n", gradient_max, *opt.precision);
        }
        else {
            auto stress = chart->make_stress(opt.projection);
            if (opt.time)
                fmt::print("stress d: {}   per second: {}\n", projection->calculate_stress(stress), measure(projection, stress));
            else
                fmt::print("stress d: {}\n", projection->calculate_stress(stress));
            const auto gradient = projection->calculate_gradient(stress);
            const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
            if (opt.time)
                fmt::print("gradie d: {}   per second: {}\n", gradient_max, measure_gradient(projection, stress));
            else
                fmt::print("gradie d: {}\n", gradient_max);
        }
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
