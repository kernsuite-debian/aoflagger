#ifndef OPTIONS_H
#define OPTIONS_H

#include <boost/optional/optional.hpp>

#include "../structures/types.h"

#include "../util/logger.h"

#include <set>
#include <string>
#include <vector>

enum class BaselineSelection {
  All,
  CrossCorrelations,
  AutoCorrelations,
  EqualToCurrent,
  AutoCorrelationsOfCurrentAntennae,
  Current
};

struct BaselineIntegration {
  enum Mode { Count, Average, AverageAbs, Squared, Stddev };
  enum Differencing { NoDifference, TimeDifference, FrequencyDifference };

  boost::optional<bool> enable, withAutos, withFlagged;
  boost::optional<Mode> mode;
  boost::optional<Differencing> differencing;

  /**
   * All options that are set in @ref other will override the settings
   * in this.
   */
  void Override(const BaselineIntegration& other) {
    if (other.enable) enable = other.enable;
    if (other.withAutos) withAutos = other.withAutos;
    if (other.withFlagged) withFlagged = other.withFlagged;
    if (other.mode) mode = other.mode;
    if (other.differencing) differencing = other.differencing;
  }

  bool operator==(const BaselineIntegration& other) const {
    return enable == other.enable && withAutos == other.withAutos &&
           withFlagged == other.withFlagged && mode == other.mode &&
           differencing == other.differencing;
  }
};

struct Options {
  std::set<size_t> antennaeToInclude, antennaeToSkip;
  std::set<size_t> bands;
  boost::optional<BaselineSelection> baselineSelection;
  BaselineIntegration baselineIntegration;
  size_t chunkSize;
  boost::optional<bool> combineSPWs;
  std::string dataColumn;
  std::string executeFilename;
  std::string executeFunctionName;
  std::set<size_t> fields;
  boost::optional<BaselineIOMode> readMode;
  boost::optional<bool> readUVW;
  std::string scriptVersion;
  boost::optional<bool> skipFlagged;
  boost::optional<size_t> startTimestep, endTimestep;
  std::string strategyFilename;
  size_t threadCount;
  boost::optional<Logger::VerbosityLevel> logVerbosity;

  std::vector<std::string> preamble;
  std::string commandLine;
  std::vector<std::string> filenames;

  Options() : chunkSize(0), threadCount(0) {}

  /**
   * All options that are set in @ref other will override the settings
   * in this.
   */
  void Override(const Options& other) {
    if (!other.antennaeToInclude.empty())
      antennaeToInclude = other.antennaeToInclude;
    if (!other.antennaeToSkip.empty()) antennaeToSkip = other.antennaeToSkip;
    if (!other.bands.empty()) bands = other.bands;
    baselineIntegration.Override(other.baselineIntegration);
    if (other.baselineSelection) baselineSelection = other.baselineSelection;
    if (other.chunkSize) chunkSize = other.chunkSize;
    if (other.combineSPWs) combineSPWs = other.combineSPWs;
    if (!other.dataColumn.empty()) dataColumn = other.dataColumn;
    if (!other.executeFilename.empty()) executeFilename = other.executeFilename;
    if (!other.executeFunctionName.empty())
      executeFunctionName = other.executeFunctionName;
    if (!other.fields.empty()) fields = other.fields;
    if (other.readMode) readMode = other.readMode;
    if (other.readUVW) readUVW = other.readUVW;
    if (!other.scriptVersion.empty()) scriptVersion = other.scriptVersion;
    if (other.skipFlagged) skipFlagged = other.skipFlagged;
    if (other.startTimestep) startTimestep = other.startTimestep;
    if (other.endTimestep) endTimestep = other.endTimestep;
    if (!other.strategyFilename.empty())
      strategyFilename = other.strategyFilename;
    if (other.threadCount) threadCount = other.threadCount;
    if (other.logVerbosity) logVerbosity = other.logVerbosity;

    preamble.insert(preamble.begin(), other.preamble.begin(),
                    other.preamble.end());
    if (!other.commandLine.empty()) commandLine = other.commandLine;
    if (!other.filenames.empty()) filenames = other.filenames;
  }

  bool operator==(const Options& rhs) const {
    return antennaeToInclude == rhs.antennaeToInclude &&
           antennaeToSkip == rhs.antennaeToSkip && bands == rhs.bands &&
           baselineIntegration == rhs.baselineIntegration &&
           baselineSelection == rhs.baselineSelection &&
           chunkSize == rhs.chunkSize && combineSPWs == rhs.combineSPWs &&
           dataColumn == rhs.dataColumn &&
           executeFilename == rhs.executeFilename &&
           executeFunctionName == rhs.executeFunctionName &&
           fields == rhs.fields && readMode == rhs.readMode &&
           readUVW == rhs.readUVW && scriptVersion == rhs.scriptVersion &&
           skipFlagged == rhs.skipFlagged &&
           startTimestep == rhs.startTimestep &&
           endTimestep == rhs.endTimestep &&
           strategyFilename == rhs.strategyFilename &&
           threadCount == rhs.threadCount && logVerbosity == rhs.logVerbosity &&
           preamble == rhs.preamble && commandLine == rhs.commandLine &&
           filenames == rhs.filenames;
  }

  bool operator!=(const Options& rhs) const { return !(*this == rhs); }

  size_t CalculateThreadCount() const;
};

#endif
