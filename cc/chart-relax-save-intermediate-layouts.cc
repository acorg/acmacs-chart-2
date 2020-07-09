#include "acmacs-base/argc-argv.hh"
#include "acmacs-base/rjson-v2.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart-modify.hh"
#include "acmacs-chart-2/randomizer.hh"

// ----------------------------------------------------------------------

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        argc_argv args(argc, argv, {
                {"-d", 2, "number of dimensions"},
                {"-m", "none", "minimum column basis"},
                {"--rough", false},
                {"--method", "cg", "method: lbfgs, cg"},
                {"--md", 2.0, "randomization diameter multiplier"},
                {"--no-disconnect-having-few-titers", false, "do not disconnect points having too few numeric titers"},
                {"--time", false, "report time of loading chart"},
                {"--verbose", false},
                {"-h", false},
                {"--help", false},
                {"-v", false},
                        });
        if (args["-h"] || args["--help"] || args.number_of_arguments() < 2) {
            fmt::print(stderr, "Usage: {} [options] <chart-file> <output-layouts.json>\n{}\n", args.program(), args.usage_options());
            exit_code = 1;
        }
        else {
            const auto report = do_report_time(args["--time"]);
            acmacs::chart::ChartModify chart{acmacs::chart::import_from_file(args[0], acmacs::chart::Verify::None, report)};
            const auto precision = args["--rough"] ? acmacs::chart::optimization_precision::rough : acmacs::chart::optimization_precision::fine;
            const auto method{acmacs::chart::optimization_method_from_string(args["--method"])};
            acmacs::chart::DisconnectedPoints disconnected;
            if (!args["--no-disconnect-having-few-titers"])
                disconnected.extend(chart.titers()->having_too_few_numeric_titers());

            auto projection = chart.projections_modify()->new_from_scratch(acmacs::number_of_dimensions_t{static_cast<long>(args["-d"])}, static_cast<std::string_view>(args["-m"]));
            projection->randomize_layout(acmacs::chart::ProjectionModify::randomizer::plain_from_sample_optimization, 2.0);
            acmacs::chart::IntermediateLayouts intermediate_layouts;
            const auto status = projection->relax(acmacs::chart::optimization_options(method, precision, args["--md"]), intermediate_layouts);
            AD_INFO("{}", status);
            AD_INFO("intermediate_layouts: {}", intermediate_layouts.size());
            rjson::value rj_intermediate_layouts = rjson::array{};
            std::for_each(intermediate_layouts.begin(), intermediate_layouts.end(), [&rj_intermediate_layouts](const auto& entry) {
                rjson::value layout = rjson::array{};
                for (size_t p_no = 0; p_no < entry.layout.number_of_points(); ++p_no) {
                    const auto p_coord = entry.layout[p_no];
                    layout.append(rjson::array(p_coord.begin(), p_coord.end()));
                }
                rj_intermediate_layouts.append(rjson::object{{"layout", layout}, {"stress", entry.stress}});
            });
            const rjson::value output_data = rjson::object{{"intermediate_layouts", rj_intermediate_layouts}};
            acmacs::file::write(args[1], rjson::pretty(output_data));
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
