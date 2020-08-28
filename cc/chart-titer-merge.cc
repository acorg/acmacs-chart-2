#include "acmacs-base/argv.hh"
#include "acmacs-chart-2/chart-modify.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    std::string_view help_pre() const override { return "calculates merged titer for the ones in the command line"; }
    argument<str_array> titers{*this, arg_name{"titer"}, mandatory};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        if (opt.titers->size() < 2)
            throw std::runtime_error{"at least two titers required for merging"};
        constexpr double standard_deviation_threshold = 1.0; // lispmds: average-multiples-unless-sd-gt-1-ignore-thresholded-unless-only-entries-then-min-threshold

        std::vector<acmacs::chart::Titer> titers(opt.titers->size());
        std::transform(std::begin(opt.titers), std::end(opt.titers), std::begin(titers), [](std::string_view src) { return acmacs::chart::Titer{src}; });
        const auto [merged, report] = acmacs::chart::TitersModify::merge_titers(titers, acmacs::chart::TitersModify::more_than_thresholded::adjust_to_next, standard_deviation_threshold);
        fmt::print("{}\n", merged);
        if (report != acmacs::chart::TitersModify::titer_merge::regular_only)
            AD_INFO("{} -> {}: {}", titers, merged, acmacs::chart::TitersModify::titer_merge_report_long(report));
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
