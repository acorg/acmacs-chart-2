#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-chart-2/chart.hh"

// ----------------------------------------------------------------------

static void process(const std::vector<std::string_view>& names, const std::vector<std::string_view>& chart_file_names);

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> charts_from{*this, "charts-from", desc{"file with chart file names"}};

    argument<str> names{*this, arg_name{"names"}, mandatory, desc{"file with names"}};
    argument<str> chart{*this, arg_name{"chart"}};
};

int main(int argc, char* const argv[])
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        if (opt.charts_from)
            process(acmacs::string::split(static_cast<std::string>(acmacs::file::read(opt.names)), "\n"), acmacs::string::split(static_cast<std::string>(acmacs::file::read(opt.charts_from)), "\n"));
        else
            process(acmacs::string::split(static_cast<std::string>(acmacs::file::read(opt.names)), "\n"), std::vector<std::string_view>{*opt.chart});
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

void process(const std::vector<std::string_view>& names, const std::vector<std::string_view>& chart_file_names)
{
    std::map<std::string_view, size_t, std::less<>> m_names;
    for (const auto& name : names) {
        if (!name.empty())
            m_names[name] = 0;
    }

    // fmt::print("INFO: names: {}  files: {}\n", names.size(), chart_file_names.size());
    for (const auto &filename : chart_file_names) {
        if (!filename.empty()) {
            try {
                auto chart = acmacs::chart::import_from_file(filename);
                if (chart->number_of_projections()) {
                    // fmt::print("INFO: {}\n", filename);
                    size_t num_found = 0;
                    auto antigens = chart->antigens();
                    for (auto antigen : *antigens) {
                        const std::string name{antigen->name()};
                        if (const auto found = m_names.find(name); found != m_names.end()) {
                            ++num_found;
                            ++found->second;
                            // fmt::print(stderr, "DEBUG: {}\n", found->first);
                        }
                    }
                    if (num_found)
                        fmt::print("{:3d} {}\n", num_found, filename);
                }
            } catch (std::exception &err) {
                fmt::print(stderr, "WARNING: {}: {}\n", filename, std::string(err.what()).substr(0, 200));
            }
        }
    }

    if (chart_file_names.size() > 1) {
        for (auto [name, count] : m_names) {
            if (count == 0)
                fmt::print(stderr, "WARNING: Name not found: {}\n", name);
        }
    }

} // process

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
