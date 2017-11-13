#include "acmacs-base/string.hh"
#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

std::string acmacs::chart::Chart::make_info() const
{
    return string::join("\n", {info()->make_info(),
                    "Antigens:" + std::to_string(number_of_antigens()) + " Sera:" + std::to_string(number_of_sera()),
                    projections()->make_info()
                    });

} // acmacs::chart::Chart::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Info::make_info() const
{
    const auto n_sources = number_of_sources();
    return string::join(" ", {name(),
                    virus(Compute::Yes),
                    lab(Compute::Yes),
                    virus_type(Compute::Yes),
                    subset(Compute::Yes),
                    assay(Compute::Yes),
                    rbc_species(Compute::Yes),
                    date(Compute::Yes),
                    n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{}
                             });

} // acmacs::chart::Info::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Titer::logged_as_string() const
{
    if (data().empty())
        throw std::runtime_error("internal: titer is empty");

    auto log_titer = [](std::string source) -> std::string {
        const double val = std::stod(source);
        return acmacs::to_string(std::log2(val / 10.0));
    };

    switch (data()[0]) {
      case '*':
          return data();
      case '<':
      case '>':
          return data()[0] + log_titer(data().substr(1));
    }
    return log_titer(data());

} // acmacs::chart::Titer::logged_as_string

// ----------------------------------------------------------------------

std::string acmacs::chart::Projection::make_info() const
{
    return std::to_string(stress()) + " " + std::to_string(number_of_dimensions()) + "d";

} // acmacs::chart::Projection::make_info

// ----------------------------------------------------------------------

std::string acmacs::chart::Projections::make_info() const
{
    std::string result = "Projections: " + std::to_string(size());
    if (!empty())
        result += "\n" + operator[](0)->make_info();
    return result;

} // acmacs::chart::Projections::make_info

// ----------------------------------------------------------------------

acmacs::chart::Chart::~Chart()
{
} // acmacs::chart::Chart::~Chart

// ----------------------------------------------------------------------

acmacs::chart::Info::~Info()
{
} // acmacs::chart::Info::~Info

// ----------------------------------------------------------------------

acmacs::chart::Antigen::~Antigen()
{
} // acmacs::chart::Antigen::~Antigen

// ----------------------------------------------------------------------

acmacs::chart::Serum::~Serum()
{
} // acmacs::chart::Serum::~Serum

// ----------------------------------------------------------------------

acmacs::chart::Antigens::~Antigens()
{
} // acmacs::chart::Antigens::~Antigens

// ----------------------------------------------------------------------

acmacs::chart::Sera::~Sera()
{
} // acmacs::chart::Sera::~Sera

// ----------------------------------------------------------------------

acmacs::chart::Titers::~Titers()
{
} // acmacs::chart::Titers::~Titers

// ----------------------------------------------------------------------

acmacs::chart::ColumnBases::~ColumnBases()
{
} // acmacs::chart::ColumnBases::~ColumnBases

// ----------------------------------------------------------------------

acmacs::chart::Projection::~Projection()
{
} // acmacs::chart::Projection::~Projection

// ----------------------------------------------------------------------

acmacs::chart::Projections::~Projections()
{
} // acmacs::chart::Projections::~Projections

// ----------------------------------------------------------------------

acmacs::chart::PlotSpec::~PlotSpec()
{
} // acmacs::chart::PlotSpec::~PlotSpec

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
