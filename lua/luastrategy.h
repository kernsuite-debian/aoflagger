#ifndef LUA_STRATEGY_H
#define LUA_STRATEGY_H

#include "data.h"

#include "../structures/timefrequencymetadata.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <vector>

class LuaStrategy {
 public:
  LuaStrategy();
  ~LuaStrategy() { clear(); }

  LuaStrategy(LuaStrategy&& source);
  LuaStrategy& operator=(LuaStrategy&& source);

  LuaStrategy(const LuaStrategy&) = delete;
  LuaStrategy& operator=(const LuaStrategy&) = delete;

  void Initialize();
  void LoadFile(const char* filename);
  void LoadText(const std::string& data);
  void Execute(class TimeFrequencyData& tfData,
               TimeFrequencyMetaDataCPtr metaData, class ScriptData& scriptData,
               const std::string& executeFunctionName);
  void RunPreamble(const std::vector<std::string>& preamble);
  lua_State* State() { return _state; }

  static std::string GetTemplateScript();

 private:
  void loadaoflagger();

  static void check(lua_State* state, int error);
  void check(int error) { check(_state, error); }
  void clear();
  aoflagger_lua::Data::Context _context;
  lua_State* _state;
};

#endif
