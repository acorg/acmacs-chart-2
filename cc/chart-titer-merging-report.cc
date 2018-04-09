#include <iostream>

#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/range.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------
// Algorithm in acmacs-c2 on 2017-12-13 in antigenic-table.hh meanTiter()
// 1. DontCare:        If there are just *, result is *
// 2. ThresholdedBoth: If there are > and < titers, result is *
// 3. ThresholdedOnly: If there are just thresholded titers, result is min (<) or max (>) of them. If MoreThanIgnore and >, result is *
// 4. SDTooBig:        if SD > sd_threshold_ (default: 1.0), result is *, SD is computed for logged_with_thresholded()
// 5. ThresholdedMax:  if max(<) of thresholded is more than max on non-thresholded (e.g. <40 20), then find minimum of thresholded which is more than max on non-thresholded, it is the result with < (for > use the same with min and max swapped)
// 6. Thresholded:     result is next of min non-thresholded with < (e.g. <20 40 --> <80, <20 80 --> <160) (corresponding result is for >)

class TiterMerger
{
 public:
    enum Type {
        DontCare,
        Regular,
        SDTooBig,
        ThresholdedBoth,
        ThresholdedOnly,
        Thresholded,
        ThresholdedMax,
    };

    enum MoreThanHandling { MoreThanIgnore, MoreThanAdjust };

    inline TiterMerger() = default;
    inline TiterMerger(std::vector<acmacs::chart::Titer>&& titers_for_layers, acmacs::chart::Titer&& merged)
        : mTiters(std::move(titers_for_layers)), mMerged(std::move(merged)) {}

    Type merge();

 private:
    std::vector<acmacs::chart::Titer> mTiters;
    acmacs::chart::Titer mMerged;
    Type mType = DontCare;
    double sd_threshold_ = 1.0;
    MoreThanHandling more_than_handling_ = MoreThanIgnore;

}; // class TiterMerger

// ----------------------------------------------------------------------

TiterMerger::Type TiterMerger::merge()
{
    if (mTiters.empty()) {
        mType = DontCare;
        if (mMerged != "*")
            throw std::runtime_error("Invalid pre-merged titer: " + mMerged + " for sources: " + acmacs::to_string(mTiters) + ", must be: *");
    }
    else {
        size_t num_less = 0, num_more = 0;
          // double max_thresholded = 0, max_non_thresholded = 0, min_thresholded = std::numeric_limits<double>::max(), min_non_thresholded = std::numeric_limits<double>::max();
        acmacs::Vector logged(mTiters.size()), thresholded, regular;
        for (auto t_no: acmacs::range(0UL, mTiters.size())) {
            const auto& titer = mTiters[t_no];
            const auto lgd = logged[t_no] = titer.logged_with_thresholded();
            switch (titer.type()) {
              case acmacs::chart::Titer::Invalid:
              case acmacs::chart::Titer::DontCare:
              case acmacs::chart::Titer::Dodgy:
                  throw std::runtime_error("Invalid titer: " + titer);
              case acmacs::chart::Titer::Regular:
                  regular.push_back(lgd);
                  break;
              case acmacs::chart::Titer::LessThan:
                  thresholded.push_back(lgd + 1);
                  ++num_less;
                  break;
              case acmacs::chart::Titer::MoreThan:
                  thresholded.push_back(lgd - 1);
                  ++num_more;
                  break;
            }
        }
        if (num_less && num_more) {
            mType = ThresholdedBoth;
            if (mMerged != "*")
                throw std::runtime_error("Invalid pre-merged titer: " + mMerged + " for sources: " + acmacs::to_string(mTiters) + ", must be: *");
        }
        else if (num_less == mTiters.size()) {
            mType = ThresholdedOnly;
            // std::cerr << "less-only mean: " << logged.min() << " src:" << mTiters << '\n';
            const auto value = acmacs::chart::Titer::from_logged(logged.min() + 1, "<"); // +1 becasue logged contains values with subtructed 1
            if (value != mMerged)
                throw std::runtime_error("Unexpected merged titer: " + static_cast<std::string>(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
        }
        else if (num_more == mTiters.size()) {
            mType = ThresholdedOnly;
            const auto value = more_than_handling_ == MoreThanAdjust ? acmacs::chart::Titer::from_logged(logged.max() - 1, ">") : acmacs::chart::Titer("*");
            if (value != mMerged)
                  // throw std::runtime_error("Unexpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
                std::cerr << "WARNING: Unexpected merged titer: " << value << " for sources: " << mTiters << ", expected: " << mMerged << '\n';
        }
        else {
            const auto [mean, sd] = logged.mean_and_standard_deviation();
            if (sd > sd_threshold_) {
                mType = SDTooBig;
                // std::cerr << "SDTooBig: " << sd << ": " << mMerged << " <-- " << mTiters << '\n';
                if (mMerged != "*")
                    throw std::runtime_error("Invalid pre-merged titer: " + mMerged + " for sources: " + acmacs::to_string(mTiters) + ", must be: * (because SD is " + std::to_string(sd) + " > " + std::to_string(sd_threshold_) + ")");
            }
            else if (num_less) {
                if (const auto regular_max = regular.max(), thresholded_max = thresholded.max(); thresholded_max > regular_max) {
                      //  if max(<) of thresholded is more than max on non-thresholded (e.g. <40 20), then find minimum of thresholded which is more than max on non-thresholded, it is the result with <
                    mType = ThresholdedMax;
                    double thresholded_to_go = thresholded_max;
                    for (double thresholded_value: thresholded) {
                        if (thresholded_value > regular_max)
                            thresholded_to_go = std::min(thresholded_to_go, thresholded_value);
                    }
                    const auto value = acmacs::chart::Titer::from_logged(thresholded_to_go, "<");
                    if (value != mMerged)
                        // throw std::runtime_error("Unexpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
                        std::cerr << "WARNING: Unexpected merged titer: " << value << " for sources: " << mTiters << ", expected: " << mMerged << '\n';
                }
                else {
                    mType = Thresholded;
                    const auto value = acmacs::chart::Titer::from_logged(regular_max + 1, "<");
                    if (value != mMerged)
                        // throw std::runtime_error("Unexpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
                        std::cerr << "WARNING: Unexpected merged titer: " << value << " for sources: " << mTiters << ", expected: " << mMerged << '\n';
                }
            }
            else if (num_more) {
                if (const auto regular_min = regular.min(), thresholded_min = thresholded.min(); thresholded_min < regular_min) {
                      // if min(>) of thresholded is less than min on non-thresholded (e.g. >1280 2560), then find maximum of thresholded which is less than min on non-thresholded, it is the result with >
                    mType = ThresholdedMax;
                    double thresholded_to_go = thresholded_min;
                    for (double thresholded_value: thresholded) {
                        if (thresholded_value < regular_min)
                            thresholded_to_go = std::max(thresholded_to_go, thresholded_value);
                    }
                    auto value = acmacs::chart::Titer::from_logged(thresholded_to_go, ">");
                    if (value != mMerged)
                          // throw std::runtime_error("Unexpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
                        std::cerr << "WARNING: Unexpected merged titer: " << value << " for sources: " << mTiters << ", expected: " << mMerged << '\n';
                }
                else {
                    mType = Thresholded;
                    const auto value = acmacs::chart::Titer::from_logged(regular_min - 1, ">");
                    if (value != mMerged)
                          // throw std::runtime_error("Unexpected merged titer: " + acmacs::to_string(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
                        std::cerr << "WARNING: Unexpected merged titer: " << value << " for sources: " << mTiters << ", expected: " << mMerged << '\n';
                }
            }
            else {
                mType = Regular;
                const auto value = acmacs::chart::Titer::from_logged(mean);
                if (value != mMerged)
                    throw std::runtime_error("Unexpected merged titer: " + static_cast<std::string>(value) + " for sources: " + acmacs::to_string(mTiters) + ", expected: " + acmacs::to_string(mMerged));
            }
        }
    }
    return mType;

} // TiterMerger::merge

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv,
                       {
                           {"--count-not-merged", false, "also count titer found in just one layer"},
                           {"--verify", false},
                           {"--time", false, "report time of loading chart"},
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
            const bool verify = args["--verify"];
            const bool count_not_merged = args["--count-not-merged"];
            const auto report = do_report_time(args["--time"]);
            auto chart = acmacs::chart::import_from_file(args[0], verify ? acmacs::chart::Verify::All : acmacs::chart::Verify::None, report);
            std::cout << chart->make_info() << '\n';

            auto titers = chart->titers();
            if (titers->number_of_layers() < 2)
                throw std::runtime_error{"chart without layers"};

            std::map<TiterMerger::Type, size_t> stat;
            size_t total = 0;
            std::vector<std::vector<TiterMerger>> merge_data(chart->number_of_antigens(), {chart->number_of_sera(), TiterMerger()});
            for (auto ag_no : acmacs::range(0UL, chart->number_of_antigens())) {
                for (auto sr_no : acmacs::range(0UL, chart->number_of_sera())) {
                    auto titers_for_layers = titers->titers_for_layers(ag_no, sr_no);
                    if (titers_for_layers.size() > 1 || count_not_merged) {
                        merge_data[ag_no][sr_no] = TiterMerger(std::move(titers_for_layers), titers->titer(ag_no, sr_no));
                        const auto type = merge_data[ag_no][sr_no].merge();
                        ++stat[type];
                        ++total;
                        if ((total % 1000) == 0)
                            std::cerr << '.';
                    }
                }
            }

            const double total_sans_dontcare = total - stat[TiterMerger::DontCare];

            if (true) {
                std::cout << "\n\n";
                std::cout << "<td class=\"number\">" << total << "</td><td></td>\n";
                std::cout << "<td class=\"number\">" << total_sans_dontcare << "</td><td class=\"percent\">" << std::setprecision(3) << (total_sans_dontcare / total * 100.0) << "%</td>\n";
                std::cout << "<td class=\"number\">" << stat[TiterMerger::Regular] << "</td><td class=\"percent\">" << std::setprecision(3)
                          << (stat[TiterMerger::Regular] / total_sans_dontcare * 100.0) << "%</td>\n";
                std::cout << "<td class=\"number\">" << stat[TiterMerger::SDTooBig] << "</td><td class=\"percent\">" << std::setprecision(3)
                          << (stat[TiterMerger::SDTooBig] / total_sans_dontcare * 100.0) << "%</td>\n";
                std::cout << "<td class=\"number\">" << stat[TiterMerger::ThresholdedBoth] << "</td><td class=\"percent\">" << std::setprecision(3)
                          << (stat[TiterMerger::ThresholdedBoth] / total_sans_dontcare * 100.0) << "%</td>\n";
                std::cout << "<td class=\"number\">" << stat[TiterMerger::ThresholdedOnly] << "</td><td class=\"percent\">" << std::setprecision(3)
                          << (stat[TiterMerger::ThresholdedOnly] / total_sans_dontcare * 100.0) << "%</td>\n";
                std::cout << "<td class=\"number\">" << stat[TiterMerger::Thresholded] << "</td><td class=\"percent\">" << std::setprecision(3)
                          << (stat[TiterMerger::Thresholded] / total_sans_dontcare * 100.0) << "%</td>\n";
                std::cout << "<td class=\"number\">" << stat[TiterMerger::ThresholdedMax] << "</td><td class=\"percent\">" << std::setprecision(3)
                          << (stat[TiterMerger::ThresholdedMax] / total_sans_dontcare * 100.0) << "%</td>\n";
            }

            // if (false) {
            //     std::cout << "Total:           " << std::setw(5) << total << '\n';
            //     std::cout << "With titers:     " << std::setw(5) << total_sans_dontcare << ' ' << std::setprecision(3) << (total_sans_dontcare / total * 100.0) << '%' << '\n';
            //     std::cout << "Regular:         " << std::setw(5) << stat[TiterMerger::Regular] << ' ' << std::setprecision(3) << (stat[TiterMerger::Regular] / total_sans_dontcare * 100.0) << '%' <<
            //     '\n'; std::cout << "SDTooBig:        " << std::setw(5) << stat[TiterMerger::SDTooBig] << ' ' << std::setprecision(3) << (stat[TiterMerger::SDTooBig] / total_sans_dontcare * 100.0)
            //     << '%' << '\n'; std::cout << "ThresholdedBoth: " << std::setw(5) << stat[TiterMerger::ThresholdedBoth] << ' ' << std::setprecision(3) << (stat[TiterMerger::ThresholdedBoth] /
            //     total_sans_dontcare * 100.0) << '%' << '\n'; std::cout << "ThresholdedOnly: " << std::setw(5) << stat[TiterMerger::ThresholdedOnly] << ' ' << std::setprecision(3) <<
            //     (stat[TiterMerger::ThresholdedOnly] / total_sans_dontcare * 100.0) << '%' << '\n'; std::cout << "Thresholded:     " << std::setw(5) << stat[TiterMerger::Thresholded] << ' ' <<
            //     std::setprecision(3) << (stat[TiterMerger::Thresholded] / total_sans_dontcare * 100.0) << '%' << '\n'; std::cout << "ThresholdedMax:  " << std::setw(5) <<
            //     stat[TiterMerger::ThresholdedMax] << ' ' << std::setprecision(3) << (stat[TiterMerger::ThresholdedMax] / total_sans_dontcare * 100.0) << '%' << '\n';
            // }
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
