#include "functions.h"

#include "../python/data.h"
#include "../python/functions.h"
#include "../python/scriptdata.h"

#include "../strategy/algorithms/normalizepassband.h"

#include "tools.h"

int Functions::apply_bandpass(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	const std::string filename = luaL_checklstring(L, 2, nullptr);
	
	lua_pushstring(L, "AOFlagger.ScriptData");
	lua_gettable(L, LUA_REGISTRYINDEX);
	ScriptData* scriptData = reinterpret_cast<ScriptData*>( lua_touserdata(L, -1) );
	
	aoflagger_python::apply_bandpass(*data, filename, *scriptData);
	return 0;
}

int Functions::downsample(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	long
		horizontalFactor = luaL_checkinteger(L, 2),
		verticalFactor = luaL_checkinteger(L, 3);
	if(!lua_isboolean(L, 4))
	{
		lua_pushstring(L, "Parameters 4 should be of boolean type in call to downsample()");
		lua_error(L);
	}
	bool
	  masked = lua_toboolean(L, 4);
	try {
		if(masked)
			Tools::NewData(L, aoflagger_python::downsample_masked(*data, horizontalFactor, verticalFactor));
		else
			Tools::NewData(L, aoflagger_python::downsample(*data, horizontalFactor, verticalFactor));
	} catch(std::exception& e) {
		luaL_error(L, e.what());
	}
	return 1;
}

int Functions::high_pass_filter(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	long
		kernelWidth = luaL_checkinteger(L, 2),
		kernelHeight = luaL_checkinteger(L, 3);
	double
		horizontalSigmaSquared = luaL_checknumber(L, 4),
		verticalSigmaSquared = luaL_checknumber(L, 5);
	aoflagger_python::high_pass_filter(*data, kernelWidth, kernelHeight, horizontalSigmaSquared, verticalSigmaSquared);
	return 0;
}

int Functions::low_pass_filter(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	long
		kernelWidth = luaL_checkinteger(L, 2),
		kernelHeight = luaL_checkinteger(L, 3);
	double
		horizontalSigmaSquared = luaL_checknumber(L, 4),
		verticalSigmaSquared = luaL_checknumber(L, 5);
	aoflagger_python::low_pass_filter(*data, kernelWidth, kernelHeight, horizontalSigmaSquared, verticalSigmaSquared);
	return 0;
}

int Functions::normalize_subbands(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	long nSubbands = luaL_checkinteger(L, 2);
	NormalizePassband::NormalizeStepwise(data->TFData(), nSubbands);
	return 0;
}

int Functions::print_polarization_statistics(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	aoflagger_python::print_polarization_statistics(*data);
	return 0;
}

int Functions::save_heat_map(lua_State* L)
{
	const char* str = luaL_checklstring(L, 1, nullptr);
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	aoflagger_python::save_heat_map(str, *data);
	return 0;
}

int Functions::scale_invariant_rank_operator(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	double
		level_horizontal = luaL_checknumber(L, 2),
		level_vertical = luaL_checknumber(L, 3);
	aoflagger_python::scale_invariant_rank_operator(*data, level_horizontal, level_vertical);
	return 0;
}

int Functions::scale_invariant_rank_operator_with_missing(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	const aoflagger_python::Data* missing = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	double
		level_horizontal = luaL_checknumber(L, 3),
		level_vertical = luaL_checknumber(L, 4);
	aoflagger_python::scale_invariant_rank_operator_with_missing(*data, *missing, level_horizontal, level_vertical);
	return 0;
}

int Functions::sumthreshold(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	double
		hThresholdFactor = luaL_checknumber(L, 2),
		vThresholdFactor = luaL_checknumber(L, 3);
	if(!lua_isboolean(L, 4) || !lua_isboolean(L, 5))
	{
		lua_pushstring(L, "Parameters 4 and 5 should be of boolean type in call to sumthreshold()");
		lua_error(L);
	}
	else {
		bool
			horizontal = lua_toboolean(L, 4),
			vertical = lua_toboolean(L, 5);
		aoflagger_python::sumthreshold(*data, hThresholdFactor, vThresholdFactor, horizontal, vertical);
	}
	return 0;
}

int Functions::sumthreshold_with_missing(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	const aoflagger_python::Data* missing = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	double
		hThresholdFactor = luaL_checknumber(L, 3),
		vThresholdFactor = luaL_checknumber(L, 4);
	if(!lua_isboolean(L, 5) || !lua_isboolean(L, 6))
	{
		lua_pushstring(L, "Parameters 5 and 6 should be of boolean type in call to sumthreshold()");
		lua_error(L);
	}
	else {
		bool
			horizontal = lua_toboolean(L, 4),
			vertical = lua_toboolean(L, 5);
		aoflagger_python::sumthreshold_with_missing(*data, *missing, hThresholdFactor, vThresholdFactor, horizontal, vertical);
	}
	return 0;
}

int Functions::threshold_channel_rms(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	double threshold = luaL_checknumber(L, 2);
	if(!lua_isboolean(L, 3))
	{
		lua_pushstring(L, "3rd parameter should be of boolean type in call to threshold_channel_rms()");
		lua_error(L);
	}
	else {
		bool thresholdLowValues = lua_toboolean(L, 3);
		aoflagger_python::threshold_channel_rms(*data, threshold, thresholdLowValues);
	}
	return 0;
}

int Functions::threshold_timestep_rms(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	double threshold = luaL_checknumber(L, 2);
	aoflagger_python::threshold_timestep_rms(*data, threshold);
	return 0;
}

int Functions::upsample(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	aoflagger_python::Data *destination = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	long
		horizontalFactor = luaL_checkinteger(L, 3),
		verticalFactor = luaL_checkinteger(L, 4);
	try {
		aoflagger_python::upsample(*data, *destination, horizontalFactor, verticalFactor);
	} catch(std::exception& e) {
		luaL_error(L, e.what());
	}
	return 0;
}

int Functions::visualize(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	std::string label = luaL_checklstring(L, 2, nullptr);
	long sortingIndex = luaL_checkinteger(L, 3);
	lua_pushstring(L, "AOFlagger.ScriptData");
	lua_gettable(L, LUA_REGISTRYINDEX);
	ScriptData* scriptData = reinterpret_cast<ScriptData*>( lua_touserdata(L, -1) );
	scriptData->AddVisualization(data->TFData(), label, sortingIndex);
	return 0;
}
