#include "acmacs-base/argc-argv.hh"
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

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--projection", 0},
                {"--double", false, "just report stress (double)"},
                {"--gradient", false, "just report gradient (double)"},
                {"--gradient-max", false, "just report gradient max (double)"},
                {"--precision", 17, "stress/gradient report precision (double)"},
                {"--time", false, "test speed"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            fmt::print(stderr, "Usage: {} [options] <chart-file>\n{}\n", args.program(), args.usage_options());
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto projection = chart->projection(args["--projection"]);
            if (args["--double"]) {
                fmt::print("{:.{}f}\n", projection->calculate_stress(), args["--precision"]);
            }
            else if (args["--gradient"]) {
                fmt::print("{}\n", projection->calculate_gradient());
            }
            else if (args["--gradient-max"]) {
                const auto gradient = projection->calculate_gradient();
                const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
                fmt::print("{:.{}f}\n", gradient_max, args["--precision"]);
            }
            else {
                auto stress = chart->make_stress(args["--projection"]);
                if (args["--time"])
                    fmt::print("stress d: {}   per second: {}\n", projection->calculate_stress(stress), measure(projection, stress));
                else
                    fmt::print("stress d: {}\n", projection->calculate_stress(stress));
                const auto gradient = projection->calculate_gradient(stress);
                const auto gradient_max = std::accumulate(gradient.begin(), gradient.end(), 0.0, [](auto mx, auto val) { return std::max(mx, std::abs(val)); });
                if (args["--time"])
                    fmt::print("gradie d: {}   per second: {}\n", gradient_max, measure_gradient(projection, stress));
                else
                    fmt::print("gradie d: {}\n", gradient_max);
            }
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
