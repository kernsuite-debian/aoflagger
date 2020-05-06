#include "luastrategy.h"

#include <stdexcept>

#include "data.h"
#include "functions.h"
#include "tools.h"

#include "../python/functions.h"
#include "../python/scriptdata.h"

LuaStrategy::LuaStrategy() :
	_state(nullptr)
{
	_state = luaL_newstate();
	luaL_openlibs(_state);
}

void LuaStrategy::clear()
{
	if(_state)
		lua_close(_state);
	_state = nullptr;
}

void LuaStrategy::check(lua_State* state, int error)
{
	if (error)
	{
		std::string msg = lua_tostring(state, -1);
		lua_pop(state, 1); // pop error
		throw std::runtime_error(msg);
	}
}

void LuaStrategy::loadaoflagger()
{
	luaL_newmetatable(_state, "AOFlaggerData");
	
	// Allow object access for AOFlaggerData structures
	lua_pushstring(_state, "__index");
	lua_pushvalue(_state, -2);  /* pushes the AOFlaggerData metatable */
	lua_settable(_state, -3);  /* metatable.__index = metatable */
	
	static const struct luaL_Reg aofdatamembers [] = {
		{"clear_mask", Data::clear_mask},
		{"convert_to_complex", Data::convert_to_complex},
		{"convert_to_polarization", Data::convert_to_polarization},
		{"copy", Data::copy},
		{"flag_zeros", Data::flag_zeros},
		{"get_antenna1_index", Data::get_antenna1_index},
		{"get_antenna1_name", Data::get_antenna1_name},
		{"get_antenna2_index", Data::get_antenna2_index},
		{"get_antenna2_name", Data::get_antenna2_name},
		{"get_antenna2_name", Data::get_antenna2_name},
		{"get_baseline_angle", Data::get_baseline_angle},
		{"get_baseline_distance", Data::get_baseline_distance},
		{"get_baseline_vector", Data::get_baseline_vector},
		{"get_frequencies", Data::get_frequencies},
		{"get_times", Data::get_times},
		{"join_mask", Data::join_mask},
		{"make_complex", Data::make_complex},
		{"polarizations", Data::polarizations},
		{"set_mask", Data::set_mask},
		{"set_mask_for_channel_range", Data::set_mask_for_channel_range},
		{"set_polarization_data", Data::set_polarization_data},
		{"set_visibilities", Data::set_visibilities},
		{"__gc", Data::gc},
		{"__sub", Data::sub},
		{NULL, NULL}
	};
	luaL_setfuncs(_state, aofdatamembers, 0);
	
	// New table for package
	lua_newtable(_state);
	
	static const struct luaL_Reg aoflib [] = {
		{"apply_bandpass", Functions::apply_bandpass},
		{"downsample", Functions::downsample},
		{"high_pass_filter", Functions::high_pass_filter},
		{"low_pass_filter", Functions::low_pass_filter},
		{"normalize_subbands", Functions::normalize_subbands},
		{"print_polarization_statistics", Functions::print_polarization_statistics},
		{"save_heat_map", Functions::save_heat_map},
		{"scale_invariant_rank_operator", Functions::scale_invariant_rank_operator},
		{"scale_invariant_rank_operator_with_missing", Functions::scale_invariant_rank_operator_with_missing},
		{"sumthreshold", Functions::sumthreshold},
		{"sumthreshold_with_missing", Functions::sumthreshold_with_missing},
		{"threshold_channel_rms", Functions::threshold_channel_rms},
		{"threshold_timestep_rms", Functions::threshold_timestep_rms},
		{"upsample", Functions::upsample},
		{"visualize", Functions::visualize},
		{NULL, NULL}
	};
	luaL_setfuncs(_state, aoflib, 0);
	
	// Name the package (which pops the table)
	lua_setglobal(_state, "aoflagger");
}

void LuaStrategy::Load(const char* filename)
{
	loadaoflagger();
	check( luaL_loadfile(_state, filename));
	check( lua_pcall(_state, 0, 0, 0) );
}

std::vector<std::pair<std::string, std::string>> LuaStrategy::GetOptions()
{
	std::vector<std::pair<std::string, std::string>> options;
	lua_getglobal(_state, "options");
	int error = lua_pcall(_state, 0, 1, 0);
	if (error)
	{
		lua_pop(_state, 1); // pop error
		return options;
	}
	else {
		// options function should have returned a table:
		if(!lua_istable(_state, -1))
			throw std::runtime_error("Function options() did not return a table");
		
		lua_pushnil(_state);
		while (lua_next(_state, -2) != 0) {
			// 'key' is at index -2 and 'value' is at index -1
			const char* key = lua_tostring(_state, -2);
			const char* value = lua_tostring(_state, -1);
			if(key == nullptr)
				throw std::runtime_error("Function options() returned a table with keys that were not convertable to a string");
			if(value == nullptr)
				throw std::runtime_error("Function options() returned a table with values that were not convertable to a string");
			options.emplace_back(key, value);
			// remove 'value'; keeps 'key' for next iteration
			lua_pop(_state, 1);
		}
		return options;
	}
}

void LuaStrategy::RunPreamble(const std::vector<std::string>& preamble)
{
	for(const std::string& str : preamble)
	{
		luaL_loadstring(_state, str.c_str());
		check( lua_pcall(_state, 0, 0, 0) );
	}
}

void LuaStrategy::Execute(TimeFrequencyData& tfData, TimeFrequencyMetaDataCPtr metaData, class ScriptData& scriptData)
{
	// store scriptData
	lua_pushstring(_state, "AOFlagger.ScriptData");
	lua_pushlightuserdata(_state, &scriptData);
	// registry["AOFlagger.ScriptData"] = tfData
	lua_settable(_state, LUA_REGISTRYINDEX);
	
	lua_getglobal(_state, "execute");
	aoflagger_python::Data* data = Tools::NewData(_state, tfData, metaData);
	check( lua_pcall(_state, 1, 0, 0) );
	tfData = data->TFData();
}
