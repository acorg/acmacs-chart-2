#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

class TiterMerger
{
 public:
    enum Type {
        DontCare,          // If there are just *, result is *
        Regular,         // If there are no < nor >, result is mean.
        ThresholdedBoth,   // If there are > and < titers, result is *
        ThresholdedOnly, // If there are just thresholded titers, result is min (<) or max (>) of them
        ThresholdedLess, // if max(<) of thresholded is more than max on non-thresholded (e.g. <40 20), then find minimum of thresholded which is more than max on non-thresholded, it is the result with <
        ThresholdedLessNormal, // result is next of min non-thresholded with < (e.g. <20 40 --> <80, <20 80 --> <160)
        ThresholdedMore, // if min(>) of thresholded is less than min on non-thresholded (e.g. >1280 2560), then find maximum of thresholded which is less than min on non-thresholded, it is the result with >
        ThresholdedMoreNormal // result is next of max non-thresholded with >
    };

    inline TiterMerger() = default;
    inline TiterMerger(std::vector<acmacs::chart::Titer>&& titers_for_layers, acmacs::chart::Titer&& merged)
        : mTiters(std::move(titers_for_layers)), mMerged(std::move(merged)) {}

    Type merge();

 private:
    std::vector<acmacs::chart::Titer> mTiters;
    acmacs::chart::Titer mMerged;
    Type mType = DontCare;

}; // class TiterMerger

// ----------------------------------------------------------------------

TiterMerger::Type TiterMerger::merge()
{
    if (mTiters.empty()) {
        mType = DontCare;
        if (mMerged != "*")
            throw std::runtime_error("Invalid pre-merged titer: " + mMerged.data() + " for sources: " + acmacs::to_string(mTiters) + ", must be: *");
    }
    else {
        size_t num_less = 0, num_more = 0;
        acmacs::Vector logged(mTiters.size());
        for (auto t_no: acmacs::range(0UL, mTiters.size())) {
            const auto& titer = mTiters[t_no];
            logged[t_no] = titer.logged_with_thresholded();
            switch (titer.type()) {
              case acmacs::chart::Titer::Invalid:
              case acmacs::chart::Titer::DontCare:
              case acmacs::chart::Titer::Dodgy:
                  throw std::runtime_error("Invalid titer: " + titer.data());
              case acmacs::chart::Titer::Regular:
                  break;
              case acmacs::chart::Titer::LessThan:
                  ++num_less;
                  break;
              case acmacs::chart::Titer::MoreThan:
                  ++num_more;
                  break;
            }
        }
        if (num_less == 0 && num_more == 0) {
            mType = Regular;
            const auto mean = logged.mean();
            const auto value = acmacs::chart::Titer::from_logged(mean);
            if (value != mMerged)
                throw std::runtime_error("Unxpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
        }
        else if (num_less && num_more) {
            mType = ThresholdedBoth;
            if (mMerged != "*")
                throw std::runtime_error("Invalid pre-merged titer: " + mMerged.data() + " for sources: " + acmacs::to_string(mTiters) + ", must be: *");
        }
        else if (num_less == mTiters.size()) {
            mType = ThresholdedOnly;
            // std::cerr << "less-only mean: " << logged.min() << " src:" << mTiters << '\n';
            const auto value = acmacs::chart::Titer::from_logged(logged.min() + 1, "<"); // +1 becasue logged contains values with subtructed 1
            if (value != mMerged)
                throw std::runtime_error("Unxpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
        }
        else if (num_more == mTiters.size()) {
            mType = ThresholdedOnly;
            const auto value = acmacs::chart::Titer::from_logged(logged.max() - 1, ">");
            if (value != mMerged)
                  // throw std::runtime_error("Unxpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
                std::cerr << "Unxpected merged titer: " << value << " for sources: " << mTiters << ", expected: " << mMerged << '\n';
        }
    }
    return mType;

} // TiterMerger::merge

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"--verify", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                {"--verbose", false}
        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() != 1) {
            std::cerr << "Usage: " << args.program() << " [options] <chart-file>\n" << args.usage_options() << '\n';
            exit_code = 1;
        }
        else {
            const bool verify = args["--verify"];
            auto chart = acmacs::chart::import_factory(args[0], verify ? acmacs::chart::Verify::All : acmacs::chart::Verify::None);

            auto titers = chart->titers();
            if (titers->number_of_layers() < 2)
                throw std::runtime_error{"chart without layers"};

            std::vector<std::vector<TiterMerger>> merge_data(chart->number_of_antigens(), {chart->number_of_sera(), TiterMerger()});
            for (auto ag_no: acmacs::range(0UL, chart->number_of_antigens())) {
                for (auto sr_no: acmacs::range(0UL, chart->number_of_sera())) {
                    merge_data[ag_no][sr_no] = TiterMerger(titers->titers_for_layers(ag_no, sr_no), titers->titer(ag_no, sr_no));
                    merge_data[ag_no][sr_no].merge();
                }
            }

              // std::cout << chart->make_info() << '\n';
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
