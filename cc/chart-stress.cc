#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

template <typename Float> double measure(acmacs::chart::ProjectionP projection, const acmacs::chart::Stress<Float>& stress)
{
    constexpr const double test_duration{10};
    const auto start = acmacs::timestamp();
    double duration{0};
    size_t count = 0;
    for (; duration < test_duration; duration = acmacs::elapsed(start)) {
        for (size_t i = 0; i < 300; ++i, ++count)
            projection->calculate_stress(stress);
    }
    return count / duration;
}

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--projection", 0L},
                {"--time", false, "test speed"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const report_time report = args["--time"] ? report_time::Yes : report_time::No;
            auto chart = acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report);
            auto projection = chart->projection(args["--projection"]);
            auto stress = chart->make_stress<double>(args["--projection"]);
            if (args["--time"])
                std::cerr << "stress d: " << acmacs::to_string(projection->calculate_stress(stress)) << "   per second: " << measure(projection, stress) << '\n';
            else
                std::cerr << "stress d: " << acmacs::to_string(projection->calculate_stress(stress)) << '\n';
            auto stress_f = chart->make_stress<float>(args["--projection"]);
            if (args["--time"])
                std::cerr << "stress f: " << acmacs::to_string(projection->calculate_stress(stress_f)) << "   per second: " << measure(projection, stress_f) << '\n';
            else
                std::cerr << "stress f: " << acmacs::to_string(projection->calculate_stress(stress_f)) << '\n';
        }
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
