#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-chart/ace-export.hh"
#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

static void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo);
static void export_antigens(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens);
static void export_sera(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera);
static void export_titers(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters);

// ----------------------------------------------------------------------

std::string acmacs::chart::ace_export(std::shared_ptr<acmacs::chart::Chart> aChart, std::string aProgramName)
{
    rjson::value ace{rjson::object{{
                {"  version", rjson::string{"acmacs-ace-v1"}},
                {"?created", rjson::string{aProgramName + " on " + acmacs::time_format()}},
                {"c", rjson::object{{
                            {"i", rjson::object{}},
                            {"a", rjson::array{}},
                            {"s", rjson::array{}},
                            {"t", rjson::object{}},
                        }}}
            }}};
    export_info(ace["c"]["i"], aChart->info());
    export_antigens(ace["c"]["a"], aChart->antigens());
    export_sera(ace["c"]["s"], aChart->sera());
    export_titers(ace["c"]["t"], aChart->titers());
      // projections
      // plot spec
    return ace.to_json_pp(1);

} // acmacs::chart::ace_export

// ----------------------------------------------------------------------

void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo)
{
    aTarget.set_field_if_not_empty("v", aInfo->virus());
    aTarget.set_field_if_not_empty("V", aInfo->virus_type());
    aTarget.set_field_if_not_empty("A", aInfo->assay());
    aTarget.set_field_if_not_empty("D", aInfo->date());
    aTarget.set_field_if_not_empty("N", aInfo->name());
    aTarget.set_field_if_not_empty("l", aInfo->lab());
    aTarget.set_field_if_not_empty("r", aInfo->rbc_species());
    aTarget.set_field_if_not_empty("s", aInfo->subset());
      //aTarget.set_field_if_not_empty("T", aInfo->table_type());

    const auto number_of_sources = aInfo->number_of_sources();
    if (number_of_sources) {
        rjson::array& array = aTarget.set_field("S", rjson::array{});
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            export_info(array.insert(rjson::object{}), aInfo->source(source_no));
        }
    }

} // export_info

// ----------------------------------------------------------------------

void export_antigens(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Antigens> aAntigens)
{
    for (auto antigen: *aAntigens) {
        rjson::object& object = aTarget.insert(rjson::object{});

        object.set_field("N", rjson::string{antigen->name()});
        object.set_field_if_not_empty("D", static_cast<const std::string&>(antigen->date()));
        object.set_field_if_not_empty("P", static_cast<const std::string&>(antigen->passage()));
        object.set_field_if_not_empty("R", static_cast<const std::string&>(antigen->reassortant()));
        object.set_array_field_if_not_empty("l", antigen->lab_ids());
        if (antigen->reference())
            object.set_field("S", rjson::string{"R"});
        object.set_array_field_if_not_empty("a", antigen->annotations());
        object.set_array_field_if_not_empty("c", antigen->clades());

        switch (antigen->lineage()) {
          case acmacs::chart::BLineage::Victoria:
              object.set_field("L", rjson::string{"V"});
              break;
          case acmacs::chart::BLineage::Yamagata:
              object.set_field("L", rjson::string{"Y"});
              break;
          case acmacs::chart::BLineage::Unknown:
              break;
        }
    }

} // export_antigens

// ----------------------------------------------------------------------

void export_sera(rjson::array& aTarget, std::shared_ptr<acmacs::chart::Sera> aSera)
{
    for (auto serum: *aSera) {
        rjson::object& object = aTarget.insert(rjson::object{});

        object.set_field("N", rjson::string{serum->name()});
        object.set_field_if_not_empty("P", static_cast<const std::string&>(serum->passage()));
        object.set_field_if_not_empty("R", static_cast<const std::string&>(serum->reassortant()));
        object.set_field_if_not_empty("I", static_cast<const std::string&>(serum->serum_id()));
        object.set_array_field_if_not_empty("a", serum->annotations());
        object.set_field_if_not_empty("s", static_cast<const std::string&>(serum->serum_species()));
        object.set_array_field_if_not_empty("h", serum->homologous_antigens());

        switch (serum->lineage()) {
          case acmacs::chart::BLineage::Victoria:
              object.set_field("L", rjson::string{"V"});
              break;
          case acmacs::chart::BLineage::Yamagata:
              object.set_field("L", rjson::string{"Y"});
              break;
          case acmacs::chart::BLineage::Unknown:
              break;
        }
    }

} // export_sera

// ----------------------------------------------------------------------

void export_titers(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Titers> aTiters)
{
      // std::cerr << "number_of_non_dont_cares: " << aTiters->number_of_non_dont_cares() << '\n';
      // std::cerr << "percent_of_non_dont_cares: " << aTiters->percent_of_non_dont_cares() << '\n';
    const size_t number_of_antigens = aTiters->number_of_antigens();
    const size_t number_of_sera = aTiters->number_of_sera();
    const double percent_of_non_dont_cares = static_cast<double>(aTiters->number_of_non_dont_cares()) / (number_of_antigens * number_of_sera);
    if (percent_of_non_dont_cares > 0.7) {
        rjson::array& list = aTarget.set_field("l", rjson::array{});
        for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
            rjson::array& row = list.insert(rjson::array{});
            for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
                row.insert(rjson::string{aTiters->titer(ag_no, sr_no)});
            }
        }
    }
    else {
        rjson::array& list = aTarget.set_field("d", rjson::array{});
        for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
            rjson::object& row = list.insert(rjson::object{});
            for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
                const auto titer = aTiters->titer(ag_no, sr_no);
                if (!titer.is_dont_care())
                    row.set_field(acmacs::to_string(sr_no), rjson::string{titer});
            }
        }
    }

      // layers

} // export_titers

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
