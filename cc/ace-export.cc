#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-chart/ace-export.hh"
#include "acmacs-chart/chart.hh"

// ----------------------------------------------------------------------

static void export_info(rjson::object& aTarget, std::shared_ptr<acmacs::chart::Info> aInfo);

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


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
