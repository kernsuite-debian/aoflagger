#ifndef DEFAULTSTRATEGYSET_H
#define DEFAULTSTRATEGYSET_H

#include <string>
#include <memory>
#include <vector>

#include "../interface/aoflagger.h"

class TelescopeFile {
 public:
  /**
   * The contents of this enum needs to be equal to aoflagger::StrategyId
   * defined in interfaces/aoflagger.h
   */
  enum TelescopeId {
    GENERIC_TELESCOPE = (int)aoflagger::TelescopeId::GENERIC_TELESCOPE,
    AARTFAAC_TELESCOPE = (int)aoflagger::TelescopeId::AARTFAAC_TELESCOPE,
    APERTIF_TELESCOPE = (int)aoflagger::TelescopeId::APERTIF_TELESCOPE,
    ARECIBO_TELESCOPE = (int)aoflagger::TelescopeId::ARECIBO_TELESCOPE,
    ATCA_TELESCOPE = (int)aoflagger::TelescopeId::ATCA_TELESCOPE,
    BIGHORNS_TELESCOPE = (int)aoflagger::TelescopeId::BIGHORNS_TELESCOPE,
    JVLA_TELESCOPE = (int)aoflagger::TelescopeId::JVLA_TELESCOPE,
    LOFAR_TELESCOPE = (int)aoflagger::TelescopeId::LOFAR_TELESCOPE,
    MWA_TELESCOPE = (int)aoflagger::TelescopeId::MWA_TELESCOPE,
    NENUFAR_TELESCOPE = (int)aoflagger::TelescopeId::NENUFAR_TELESCOPE,
    PARKES_TELESCOPE = (int)aoflagger::TelescopeId::PARKES_TELESCOPE,
    WSRT_TELESCOPE = (int)aoflagger::TelescopeId::WSRT_TELESCOPE
  };

  static std::vector<TelescopeFile::TelescopeId> List() {
    return std::vector<TelescopeFile::TelescopeId>{
        GENERIC_TELESCOPE, AARTFAAC_TELESCOPE, APERTIF_TELESCOPE,
        ARECIBO_TELESCOPE, ATCA_TELESCOPE,     BIGHORNS_TELESCOPE,
        JVLA_TELESCOPE,    LOFAR_TELESCOPE,    MWA_TELESCOPE,
        NENUFAR_TELESCOPE, PARKES_TELESCOPE,   WSRT_TELESCOPE};
  }

  /**
   * @brief Searches a strategy for a given telescope.
   *
   * @param argv0 The argv[0] variable passed to main(), or empty if it is not
   * available.
   * @param telescopeId One of the telescopes, if known, otherwise
   * GENERIC_TELESCOPE.
   * @param scenario Used as 'suffix' to the name of the telescope. This allows
   * multiple versions for the same telescope.
   * @returns Path to the strategy, or empty string if not found.
   */
  static std::string FindStrategy(const std::string& argv0,
                                  enum TelescopeId telescopeId,
                                  const std::string& scenario = "");

  static std::string TelescopeName(TelescopeFile::TelescopeId telescopeId);

  static std::string TelescopeDescription(
      TelescopeFile::TelescopeId telescopeId);

  static TelescopeFile::TelescopeId TelescopeIdFromName(
      const std::string& name);
};

#endif
