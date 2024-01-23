#include "luastrategy.h"

#include <stdexcept>

#include "datawrapper.h"
#include "functions.h"
#include "functionswrapper.h"
#include "scriptdata.h"
#include "tools.h"

LuaStrategy::LuaStrategy() : _state(luaL_newstate()) {}

LuaStrategy::LuaStrategy(LuaStrategy&& source) : _state(source._state) {
  source._state = nullptr;
}

LuaStrategy& LuaStrategy::operator=(LuaStrategy&& source) {
  std::swap(_state, source._state);
  return *this;
}

void LuaStrategy::clear() {
  if (_state) lua_close(_state);
  _state = nullptr;
}

void LuaStrategy::check(lua_State* state, int error) {
  if (error) {
    const char* msg = lua_tostring(state, -1);
    if (msg) {
      const std::string str(msg);
      lua_pop(state, 1);  // pop error
      throw std::runtime_error(str);
    } else {
      throw std::runtime_error("An unspecified error was thrown inside Lua");
    }
  }
}

void LuaStrategy::loadaoflagger() {
  luaL_newmetatable(_state, "AOFlaggerData");

  // Allow object access for AOFlaggerData structures
  lua_pushstring(_state, "__index");
  lua_pushvalue(_state, -2); /* pushes the AOFlaggerData metatable */
  lua_settable(_state, -3);  /* metatable.__index = metatable */

  static const struct luaL_Reg aofdatamembers[] = {
      {"clear_mask", Data::clear_mask},
      {"convert_to_complex", Data::convert_to_complex},
      {"convert_to_polarization", Data::convert_to_polarization},
      {"copy", Data::copy},
      {"flag_zeros", Data::flag_zeros},
      {"flag_nans", Data::flag_nans},
      {"get_antenna1_index", Data::get_antenna1_index},
      {"get_antenna1_name", Data::get_antenna1_name},
      {"get_antenna2_index", Data::get_antenna2_index},
      {"get_antenna2_name", Data::get_antenna2_name},
      {"get_antenna2_name", Data::get_antenna2_name},
      {"get_baseline_angle", Data::get_baseline_angle},
      {"get_baseline_distance", Data::get_baseline_distance},
      {"get_baseline_vector", Data::get_baseline_vector},
      {"get_complex_state", Data::get_complex_state},
      {"get_frequencies", Data::get_frequencies},
      {"get_polarizations", Data::get_polarizations},
      {"get_times", Data::get_times},
      {"has_metadata", Data::has_metadata},
      {"invert_mask", Data::invert_mask},
      {"is_auto_correlation", Data::is_auto_correlation},
      {"is_complex", Data::is_complex},
      {"join_mask", Data::join_mask},
      {"set_mask", Data::set_mask},
      {"set_mask_for_channel_range", Data::set_mask_for_channel_range},
      {"set_masked_visibilities", Data::set_masked_visibilities},
      {"set_polarization_data", Data::set_polarization_data},
      {"set_visibilities", Data::set_visibilities},
      {"__div", Data::div},
      {"__gc", Data::gc},
      {"__sub", Data::sub},
      {nullptr, nullptr}};
  luaL_setfuncs(_state, aofdatamembers, 0);

  // New table for package
  lua_newtable(_state);

  static const struct luaL_Reg aoflib[] = {
      {"apply_bandpass", Functions::apply_bandpass},
      {"collect_statistics", Functions::collect_statistics},
      {"copy_to_channel", Functions::copy_to_channel},
      {"copy_to_frequency", Functions::copy_to_frequency},
      {"downsample", Functions::downsample},
      {"high_pass_filter", Functions::high_pass_filter},
      {"low_pass_filter", Functions::low_pass_filter},
      {"norm", Functions::norm},
      {"normalize_bandpass", Functions::normalize_bandpass},
      {"normalize_subbands", Functions::normalize_subbands},
      {"print_polarization_statistics",
       Functions::print_polarization_statistics},
      {"require_min_version", Functions::require_min_version},
      {"require_max_version", Functions::require_max_version},
      {"save_heat_map", Functions::save_heat_map},
      {"scale_invariant_rank_operator",
       Functions::scale_invariant_rank_operator},
      {"scale_invariant_rank_operator_masked",
       Functions::scale_invariant_rank_operator_masked},
      {"set_progress", Functions::set_progress},
      {"set_progress_text", Functions::set_progress_text},
      {"sqrt", Functions::sqrt},
      {"sumthreshold", Functions::sumthreshold},
      {"sumthreshold_masked", Functions::sumthreshold_masked},
      {"threshold_channel_rms", Functions::threshold_channel_rms},
      {"threshold_timestep_rms", Functions::threshold_timestep_rms},
      {"trim_channels", Functions::trim_channels},
      {"trim_frequencies", Functions::trim_frequencies},
      {"upsample", Functions::upsample_image},  // DEPRECATED
      {"upsample_image", Functions::upsample_image},
      {"upsample_mask", Functions::upsample_mask},
      {"visualize", Functions::visualize},
      {nullptr, nullptr}};
  luaL_setfuncs(_state, aoflib, 0);

  // Name the package (which pops the table)
  lua_setglobal(_state, "aoflagger");
}

void LuaStrategy::Initialize() {
  luaL_openlibs(_state);
  loadaoflagger();
}

void LuaStrategy::LoadFile(const char* filename) {
  check(luaL_loadfile(_state, filename));
  check(lua_pcall(_state, 0, 0, 0));
}

void LuaStrategy::LoadText(const std::string& data) {
  check(luaL_loadstring(_state, data.c_str()));
  check(lua_pcall(_state, 0, 0, 0));
}

void LuaStrategy::RunPreamble(const std::vector<std::string>& preamble) {
  for (const std::string& str : preamble) {
    luaL_loadstring(_state, str.c_str());
    check(lua_pcall(_state, 0, 0, 0));
  }
}

void LuaStrategy::Execute(TimeFrequencyData& tfData,
                          TimeFrequencyMetaDataCPtr metaData,
                          class ScriptData& scriptData,
                          const std::string& executeFunctionName) {
  // store scriptData
  lua_pushstring(_state, "AOFlagger.ScriptData");
  lua_pushlightuserdata(_state, &scriptData);
  // Set: registry["AOFlagger.ScriptData"] = scriptData
  lua_settable(_state, LUA_REGISTRYINDEX);

  lua_getglobal(_state, executeFunctionName.c_str());
  if (lua_isnil(_state, -1)) {
    throw std::runtime_error(
        "The Lua script did not specify an execute function named " +
        executeFunctionName + "().");
  }
  aoflagger_lua::Data* data =
      Tools::NewData(_state, tfData, metaData, _context);
  check(lua_pcall(_state, 1, 0, 0));

  tfData = data->TFData();

  for (aoflagger_lua::Data* data : _context.list) {
    if (!data->is_persistent()) data->clear();
  }
  _context.list.clear();
}

std::string LuaStrategy::GetTemplateScript() {
  return "function execute(data)\n"
         "\n"
         "  data:clear_mask()\n"
         "  -- Insert detection code here\n"
         "\n"
         "end\n"
         "\n"
         "function options()\n"
         "  opts_main = {\n"
         "    [\"baselines\"] = \"cross\"\n"
         "  }\n"
         "  return { [\"main\"] = opts_main }\n"
         "end\n";
}
