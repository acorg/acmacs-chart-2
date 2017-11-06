#include "acmacs-base/rjson.hh"
#include "acmacs-base/time.hh"
#include "acmacs-chart/ace-export.hh"

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
    return ace.to_json_pp();

} // acmacs::chart::ace_export

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
