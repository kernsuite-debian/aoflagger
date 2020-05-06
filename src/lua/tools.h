#ifndef LUA_TOOLS_H
#define LUA_TOOLS_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "../python/data.h"

class Tools {
public:
	static aoflagger_python::Data* NewData(lua_State* L, TimeFrequencyData&& tfData, TimeFrequencyMetaDataCPtr metaData)
	{
		void *userdatum = lua_newuserdata(L, sizeof(aoflagger_python::Data));
		aoflagger_python::Data* data = new(userdatum) aoflagger_python::Data(std::move(tfData), std::move(metaData));
		luaL_getmetatable(L, "AOFlaggerData");
		lua_setmetatable(L, -2);
		return data;
	}
	
	static aoflagger_python::Data* NewData(lua_State* L, TimeFrequencyData& tfData, TimeFrequencyMetaDataCPtr metaData)
	{
		void *userdatum = lua_newuserdata(L, sizeof(aoflagger_python::Data));
		aoflagger_python::Data* data = new(userdatum) aoflagger_python::Data(tfData, std::move(metaData));
		luaL_getmetatable(L, "AOFlaggerData");
		lua_setmetatable(L, -2);
		return data;
	}
	
	static aoflagger_python::Data* NewData(lua_State* L, aoflagger_python::Data&& source)
	{
		void *userdatum = lua_newuserdata(L, sizeof(aoflagger_python::Data));
		aoflagger_python::Data* data = new(userdatum) aoflagger_python::Data(std::move(source));
		luaL_getmetatable(L, "AOFlaggerData");
		lua_setmetatable(L, -2);
		return data;
	}
	
	static aoflagger_python::Data* NewData(lua_State* L, const aoflagger_python::Data& source)
	{
		void *userdatum = lua_newuserdata(L, sizeof(aoflagger_python::Data));
		aoflagger_python::Data* data = new(userdatum) aoflagger_python::Data(source);
		luaL_getmetatable(L, "AOFlaggerData");
		lua_setmetatable(L, -2);
		return data;
	}
};

#endif
