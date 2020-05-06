#ifndef LUA_STRATEGY_H
#define LUA_STRATEGY_H

#include "../python/data.h"

#include "../structures/timefrequencymetadata.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <string>
#include <vector>

class LuaStrategy
{
public:
	LuaStrategy();
	~LuaStrategy()
	{ clear(); }
	
	LuaStrategy(const LuaStrategy&) = delete;
	LuaStrategy(LuaStrategy&&) noexcept = default;
	LuaStrategy& operator=(const LuaStrategy&) = delete;
	LuaStrategy& operator=(LuaStrategy&&) noexcept = default;
	
	void Load(const char* filename);
	std::vector<std::pair<std::string, std::string>> GetOptions();
	void Execute(class TimeFrequencyData& tfData, TimeFrequencyMetaDataCPtr metaData, class ScriptData& scriptData);
	void RunPreamble(const std::vector<std::string>& preamble);
	
private:
	void loadaoflagger();
	
	static void check(lua_State* state, int error);
	void check(int error) { check(_state, error); }
	void clear();
	lua_State *_state;
};

#endif
