#ifndef LUA_FUNCTIONS_H
#define LUA_FUNCTIONS_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

class Functions {
public:
	static int apply_bandpass(lua_State* L);
	static int downsample(lua_State* L);
	static int high_pass_filter(lua_State* L);
	static int low_pass_filter(lua_State* L);
	static int normalize_subbands(lua_State* L);
	static int print_polarization_statistics(lua_State* L);
	static int save_heat_map(lua_State* L);
	static int scale_invariant_rank_operator(lua_State* L);
	static int scale_invariant_rank_operator_with_missing(lua_State* L);
	static int sumthreshold(lua_State* L);
	static int sumthreshold_with_missing(lua_State* L);
	static int threshold_channel_rms(lua_State* L);
	static int threshold_timestep_rms(lua_State* L);
	static int upsample(lua_State* L);
	static int visualize(lua_State* L);
};

#endif
