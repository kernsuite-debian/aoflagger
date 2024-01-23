#include "optionsfunction.h"

#include <version.h>

extern "C" {
#include <lualib.h>
#include <lauxlib.h>
}

std::map<std::string, Options> OptionsFunction::GetOptions(
    lua_State* state, const Options& cmdLineOptions) {
  lua_getglobal(state, "options");
  const int error = lua_pcall(state, 0, 1, 0);
  std::map<std::string, Options> optionMap;
  if (error) {
    lua_pop(state, 1);  // pop error
  } else {
    // options function should have returned a table:
    if (!lua_istable(state, -1))
      throw std::runtime_error("Function options() did not return a table");

    // Iterate over the table. Keys are the name of the run, values
    // are itself a table with the options for that run.
    lua_pushnil(state);
    while (lua_next(state, -2) != 0) {
      // 'key' is at index -2 and 'value' is at index -1

      // It is not allowed to change 'key' on the stack during traversal, and
      // lua_tostring() might change it. Therefore, we make a temp copy of the
      // key:
      lua_pushvalue(state, -2);
      const char* key = lua_tostring(state, -1);
      lua_pop(state, 1);

      if (key == nullptr)
        throw std::runtime_error(
            "Function options() returned a table with keys that were not "
            "convertable to a string");
      if (!lua_istable(state, -1))
        throw std::runtime_error(
            std::string("Invalid type of element '") + key +
            "' return by function options(): should be an option table");
      const Options option =
          fillOptions(state, cmdLineOptions, std::string(key));
      optionMap.emplace(key, option);

      // remove 'value'; keeps 'key' for next iteration
      lua_pop(state, 1);
    }
  }
  return optionMap;
}

std::string OptionsFunction::strOption(lua_State* state,
                                       const std::string& keyName,
                                       const std::string& runName) {
  const char* val = lua_tostring(state, -1);
  if (val == nullptr)
    throw std::runtime_error(
        "Option " + keyName + " for run name '" + runName +
        "' returned by options() was not convertable to a string");
  return std::string(val);
}

bool OptionsFunction::boolOption(lua_State* state, const std::string& keyName,
                                 const std::string& runName) {
  if (!lua_isboolean(state, -1))
    throw std::runtime_error(
        "Option " + keyName + " for run name '" + runName +
        "' returned by options() should be of type boolean");
  return lua_toboolean(state, -1);
}

size_t OptionsFunction::uintOption(lua_State* state, const std::string& keyName,
                                   const std::string& runName) {
  if (!lua_isinteger(state, -1))
    throw std::runtime_error(
        "Option " + keyName + " for run name '" + runName +
        "' returned by options() should be of type integer");
  return lua_tointeger(state, -1);
}

std::vector<size_t> OptionsFunction::uintListOption(
    lua_State* state, const std::string& keyName, const std::string& runName) {
  if (!lua_istable(state, -1))
    throw std::runtime_error(
        "Option " + keyName + " for run name '" + runName +
        "' returned by options() should be a table of integers");
  std::vector<size_t> vals;
  lua_pushnil(state);
  while (lua_next(state, -2) != 0) {
    if (!lua_isinteger(state, -1))
      throw std::runtime_error(
          "Option " + keyName + " for run name '" + runName +
          "' returned by options() should be a table of integers");
    vals.emplace_back(lua_tointeger(state, -1));
    lua_pop(state, 1);
  }
  return vals;
}

std::vector<std::string> OptionsFunction::stringListOption(
    lua_State* state, const std::string& keyName, const std::string& runName) {
  if (!lua_istable(state, -1))
    throw std::runtime_error(
        "Option " + keyName + " for run name '" + runName +
        "' returned by options() should be a table of strings");
  std::vector<std::string> vals;
  lua_pushnil(state);
  while (lua_next(state, -2) != 0) {
    if (!lua_isstring(state, -1))
      throw std::runtime_error(
          "Option " + keyName + " for run name '" + runName +
          "' returned by options() should be a table of strings");
    vals.emplace_back(lua_tostring(state, -1));
    lua_pop(state, 1);
  }
  return vals;
}

Options OptionsFunction::fillOptions(lua_State* state,
                                     const Options& cmdLineOptions,
                                     const std::string& runName) {
  Options options;
  // Iterate over the table. Keys are the name of the run, values
  // are itself a table with the options for that run.
  lua_pushnil(state);
  while (lua_next(state, -2) != 0) {
    if (!lua_isstring(state, -2))
      throw std::runtime_error("options(): Key in option table for run name '" +
                               runName + "' was not convertable to a string");
    const char* key = lua_tostring(state, -2);
    const std::string keyStr(key);

    if (keyStr == "bands") {
      std::vector<size_t> list = uintListOption(state, keyStr, runName);
      options.bands = std::set<size_t>(list.begin(), list.end());
    } else if (keyStr == "baseline-integration") {
      options.baselineIntegration.enable = true;
      const std::string val = strOption(state, keyStr, runName);
      if (val == "count")
        options.baselineIntegration.mode = BaselineIntegration::Count;
      else if (val == "average")
        options.baselineIntegration.mode = BaselineIntegration::Average;
      else if (val == "average-abs")
        options.baselineIntegration.mode = BaselineIntegration::AverageAbs;
      else if (val == "squared")
        options.baselineIntegration.mode = BaselineIntegration::Squared;
      else if (val == "stddev")
        options.baselineIntegration.mode = BaselineIntegration::Stddev;
      else
        throw std::runtime_error(
            "options(): Invalid setting '" + val +
            "' for option 'baseline-integration' returned");
    } else if (keyStr == "baselines") {
      const std::string val = strOption(state, keyStr, runName);
      if (val == "all")
        options.baselineSelection = BaselineSelection::All;
      else if (val == "cross")
        options.baselineSelection = BaselineSelection::CrossCorrelations;
      else if (val == "auto")
        options.baselineSelection = BaselineSelection::AutoCorrelations;
      else
        throw std::runtime_error("options(): Invalid setting '" + val +
                                 "' for option 'baselines' returned");
    } else if (keyStr == "chunk-size") {
      options.chunkSize = uintOption(state, keyStr, runName);
    } else if (keyStr == "column-name") {
      options.dataColumn = strOption(state, keyStr, runName);
    } else if (keyStr == "combine-spws") {
      options.combineSPWs = boolOption(state, keyStr, runName);
    } else if (keyStr == "execute-file") {
      options.executeFilename = strOption(state, keyStr, runName);
    } else if (keyStr == "execute-function") {
      options.executeFunctionName = strOption(state, keyStr, runName);
    } else if (keyStr == "fields") {
      std::vector<size_t> list = uintListOption(state, keyStr, runName);
      options.fields = std::set<size_t>(list.begin(), list.end());
    } else if (keyStr == "files") {
      options.filenames = stringListOption(state, keyStr, runName);
    } else if (keyStr == "min‑aoflagger-version") {
      const std::string minVersion = strOption(state, keyStr, runName);
      const size_t dot = minVersion.find('.');
      if (dot == minVersion.npos)
        throw std::runtime_error(
            "options(): Invalid version specified in option "
            "min-aoflagger-version: should be of the form major.minor");
      int major = std::atoi(minVersion.substr(0, dot).c_str()),
          minor = std::atoi(minVersion.substr(dot + 1).c_str());
      const bool tooOld =
          (AOFLAGGER_VERSION_MAJOR < major) ||
          (AOFLAGGER_VERSION_MAJOR == major && AOFLAGGER_VERSION_MINOR < minor);
      if (tooOld)
        throw std::runtime_error(
            "AOFlagger is too old for this script: required: " + minVersion +
            ", this version: " + AOFLAGGER_VERSION_STR);
    } else if (keyStr == "quiet") {
      if (boolOption(state, keyStr, runName))
        options.logVerbosity = Logger::QuietVerbosity;
    } else if (keyStr == "read-mode") {
      const std::string readMode = strOption(state, keyStr, runName);
      if (readMode == "direct")
        options.readMode = DirectReadMode;
      else if (readMode == "indirect")
        options.readMode = ReorderingReadMode;
      else if (readMode == "memory")
        options.readMode = MemoryReadMode;
      else if (readMode == "auto")
        options.readMode = AutoReadMode;
      else
        throw std::runtime_error(
            "options(): Invalid setting for option 'read-mode' returned");
    } else if (keyStr == "read-uvws") {
      options.readUVW = boolOption(state, keyStr, runName);
    } else if (keyStr == "script-version") {
      options.scriptVersion = strOption(state, keyStr, runName);
    } else if (keyStr == "start-timestep") {
      options.startTimestep = uintOption(state, keyStr, runName);
    } else if (keyStr == "end-timestep") {
      options.endTimestep = uintOption(state, keyStr, runName);
    } else if (keyStr == "threads") {
      options.threadCount = uintOption(state, keyStr, runName);
    } else if (keyStr == "verbose") {
      // Option 'quiet' conflicts with verbose; quiet override verbose:
      if (boolOption(state, keyStr, runName) && !options.logVerbosity)
        options.logVerbosity = Logger::VerboseVerbosity;
    } else {
      Logger::Warn << "options(): Ignoring unknown key '" + keyStr + "'.\n";
    }

    // remove 'value'; keeps 'key' for next iteration
    lua_pop(state, 1);
  }
  options.Override(cmdLineOptions);
  return options;
}
