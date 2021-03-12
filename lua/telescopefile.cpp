#include "telescopefile.h"

#include "../imagesets/bhfitsimageset.h"
#include "../imagesets/filterbankset.h"
#include "../imagesets/fitsimageset.h"
#include "../imagesets/imageset.h"
#include "../imagesets/msimageset.h"

#include "../../structures/msmetadata.h"

#include <boost/algorithm/string/case_conv.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <version.h>

std::string TelescopeFile::TelescopeName(
    TelescopeFile::TelescopeId telescopeId) {
  switch (telescopeId) {
    case GENERIC_TELESCOPE:
      return "Generic";
    case AARTFAAC_TELESCOPE:
      return "Aartfaac";
    case APERTIF_TELESCOPE:
      return "APERTIF";
    case ARECIBO_TELESCOPE:
      return "Arecibo";
    case ATCA_TELESCOPE:
      return "ATCA";
    case BIGHORNS_TELESCOPE:
      return "Bighorns";
    case JVLA_TELESCOPE:
      return "JVLA";
    case LOFAR_TELESCOPE:
      return "LOFAR";
    case MWA_TELESCOPE:
      return "MWA";
    case NENUFAR_TELESCOPE:
      return "NenuFAR";
    case PARKES_TELESCOPE:
      return "Parkes";
    case WSRT_TELESCOPE:
      return "WSRT";
  }
  return "";
}

std::string TelescopeFile::TelescopeDescription(
    TelescopeFile::TelescopeId telescopeId) {
  switch (telescopeId) {
    default:
    case GENERIC_TELESCOPE:
      return "Generic";
    case AARTFAAC_TELESCOPE:
      return "AARTFAAC";
    case APERTIF_TELESCOPE:
      return "WSRT with APERTIF multi-beaming system";
    case ARECIBO_TELESCOPE:
      return "Arecibo (305 m single dish, Puerto Rico)";
    case BIGHORNS_TELESCOPE:
      return "Bighorns (low-frequency wide-band EoR instrument, Curtin uni, "
             "Australia)";
    case JVLA_TELESCOPE:
      return "JVLA (Jansky Very Large Array, New Mexico)";
    case LOFAR_TELESCOPE:
      return "LOFAR (Low-Frequency Array, Europe)";
    case MWA_TELESCOPE:
      return "MWA (Murchison Widefield Array, Australia)";
    case NENUFAR_TELESCOPE:
      return "NenuFAR (New Extension in Nançay upgrading LOFAR, France)";
    case PARKES_TELESCOPE:
      return "Parkes (single dish, Australia)";
    case WSRT_TELESCOPE:
      return "WSRT (Westerbork Synthesis Radio Telescope, Netherlands)";
  }
}

TelescopeFile::TelescopeId TelescopeFile::TelescopeIdFromName(
    const std::string& name) {
  const std::string nameUpper = boost::algorithm::to_upper_copy(name);
  if (nameUpper == "AARTFAAC")
    return AARTFAAC_TELESCOPE;
  else if (nameUpper == "APERTIF")
    return APERTIF_TELESCOPE;
  else if (nameUpper == "ARECIBO" || nameUpper == "ARECIBO 305M")
    return ARECIBO_TELESCOPE;
  else if (nameUpper == "BIGHORNS")
    return BIGHORNS_TELESCOPE;
  else if (nameUpper == "EVLA" || nameUpper == "JVLA")
    return JVLA_TELESCOPE;
  else if (nameUpper == "LOFAR")
    return LOFAR_TELESCOPE;
  else if (nameUpper == "MWA")
    return MWA_TELESCOPE;
  else if (nameUpper == "NENUFAR")
    return NENUFAR_TELESCOPE;
  else if (nameUpper == "PARKES" || nameUpper == "PKS" ||
           nameUpper == "ATPKSMB")
    return PARKES_TELESCOPE;
  else if (nameUpper == "WSRT")
    return WSRT_TELESCOPE;
  else
    return GENERIC_TELESCOPE;
}

std::string TelescopeFile::FindStrategy(const std::string& argv0,
                                        enum TelescopeId telescopeId,
                                        const std::string& scenario) {
  std::string filename = boost::to_lower_copy(TelescopeName(telescopeId));
  if (scenario.empty())
    filename = filename + "-default.lua";
  else
    filename = filename + "-" + boost::to_lower_copy(scenario) + ".lua";

  boost::filesystem::path search;

  search = boost::filesystem::path(AOFLAGGER_INSTALL_PATH) /
           "share/aoflagger/strategies" / filename;
  if (boost::filesystem::exists(search)) return search.string();

  if (!argv0.empty()) {
    boost::filesystem::path root = boost::filesystem::path(argv0).remove_leaf();

    search = root / "../share/aoflagger/strategies" / filename;
    if (boost::filesystem::exists(search)) return search.string();

    search = root / "../data/strategies" / filename;
    if (boost::filesystem::exists(search)) return search.string();
  }

  search =
      boost::filesystem::path("/usr/share/aoflagger/strategies") / filename;
  if (boost::filesystem::exists(search)) return search.string();

  search = boost::filesystem::path("/usr/local/share/aoflagger/strategies") /
           filename;
  if (boost::filesystem::exists(search)) return search.string();

  return std::string();
}
