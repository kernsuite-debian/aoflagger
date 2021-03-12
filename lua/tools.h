#ifndef LUA_TOOLS_H
#define LUA_TOOLS_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "data.h"

class Tools {
 public:
  static aoflagger_lua::Data* NewData(lua_State* L, TimeFrequencyData&& tfData,
                                      TimeFrequencyMetaDataCPtr metaData,
                                      aoflagger_lua::Data::Context& context) {
    void* userdatum = lua_newuserdata(L, sizeof(aoflagger_lua::Data));
    aoflagger_lua::Data* data = new (userdatum)
        aoflagger_lua::Data(std::move(tfData), std::move(metaData), context);
    luaL_getmetatable(L, "AOFlaggerData");
    lua_setmetatable(L, -2);
    return data;
  }

  static aoflagger_lua::Data* NewData(lua_State* L, TimeFrequencyData& tfData,
                                      TimeFrequencyMetaDataCPtr metaData,
                                      aoflagger_lua::Data::Context& context) {
    void* userdatum = lua_newuserdata(L, sizeof(aoflagger_lua::Data));
    aoflagger_lua::Data* data = new (userdatum)
        aoflagger_lua::Data(tfData, std::move(metaData), context);
    luaL_getmetatable(L, "AOFlaggerData");
    lua_setmetatable(L, -2);
    return data;
  }

  static aoflagger_lua::Data* NewData(lua_State* L,
                                      aoflagger_lua::Data&& source) {
    void* userdatum = lua_newuserdata(L, sizeof(aoflagger_lua::Data));
    aoflagger_lua::Data* data =
        new (userdatum) aoflagger_lua::Data(std::move(source));
    luaL_getmetatable(L, "AOFlaggerData");
    lua_setmetatable(L, -2);
    return data;
  }

  static aoflagger_lua::Data* NewData(lua_State* L,
                                      const aoflagger_lua::Data& source) {
    void* userdatum = lua_newuserdata(L, sizeof(aoflagger_lua::Data));
    aoflagger_lua::Data* data = new (userdatum) aoflagger_lua::Data(source);
    luaL_getmetatable(L, "AOFlaggerData");
    lua_setmetatable(L, -2);
    return data;
  }
};

#endif
