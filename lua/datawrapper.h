#ifndef LUA_DATA_WRAPPER_H
#define LUA_DATA_WRAPPER_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class Data {
 public:
  static int clear_mask(lua_State* L);
  static int convert_to_complex(lua_State* L);
  static int convert_to_polarization(lua_State* L);
  static int copy(lua_State* L);
  static int flag_zeros(lua_State* L);
  static int flag_nans(lua_State* L);
  static int get_antenna1_index(lua_State* L);
  static int get_antenna1_name(lua_State* L);
  static int get_antenna2_index(lua_State* L);
  static int get_antenna2_name(lua_State* L);
  static int get_baseline_angle(lua_State* L);
  static int get_baseline_distance(lua_State* L);
  static int get_baseline_vector(lua_State* L);
  static int get_complex_state(lua_State* L);
  static int get_frequencies(lua_State* L);
  static int get_polarizations(lua_State* L);
  static int get_times(lua_State* L);
  static int has_metadata(lua_State* L);
  static int is_auto_correlation(lua_State* L);
  static int is_complex(lua_State* L);
  static int join_mask(lua_State* L);
  static int set_mask(lua_State* L);
  static int set_mask_for_channel_range(lua_State* L);
  static int set_visibilities(lua_State* L);
  static int set_polarization_data(lua_State* L);
  static int gc(lua_State* L);
  static int sub(lua_State* L);

 private:
};

#endif
