#ifndef LUA_OPTIONS_FUNCTION_H
#define LUA_OPTIONS_FUNCTION_H

#include <map>
#include <string>
#include <vector>

#include "../aoluarunner/options.h"

extern "C" {
#include <lua.h>
}

class OptionsFunction {
 public:
  // Returns a map of options. The map keys are the name of the runs.
  static std::map<std::string, Options> GetOptions(
      lua_State* state, const Options& cmdLineOptions);

 private:
  static Options fillOptions(lua_State* state, const Options& cmdLineOptions,
                             const std::string& runName);

  static bool boolOption(lua_State* state, const std::string& keyName,
                         const std::string& runName);
  static size_t uintOption(lua_State* state, const std::string& keyName,
                           const std::string& runName);
  static std::vector<size_t> uintListOption(lua_State* state,
                                            const std::string& keyName,
                                            const std::string& runName);
  static std::vector<std::string> stringListOption(lua_State* state,
                                                   const std::string& keyName,
                                                   const std::string& runName);
  static std::string strOption(lua_State* state, const std::string& keyName,
                               const std::string& runName);
};

#endif
