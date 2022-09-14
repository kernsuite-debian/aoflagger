#include "functionswrapper.h"

#include "data.h"

#include "../algorithms/normalizebandpass.h"

#include "../util/progress/progresslistener.h"

#include "functions.h"
#include "scriptdata.h"
#include "tools.h"
#include "version.h"

#include "../structures/versionstring.h"

using algorithms::NormalizeBandpass;

int Functions::apply_bandpass(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const std::string filename = luaL_checklstring(L, 2, nullptr);

  lua_pushstring(L, "AOFlagger.ScriptData");
  lua_gettable(L, LUA_REGISTRYINDEX);
  ScriptData* scriptData = reinterpret_cast<ScriptData*>(lua_touserdata(L, -1));

  try {
    aoflagger_lua::apply_bandpass(*data, filename, *scriptData);
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  return 0;
}

int Functions::collect_statistics(lua_State* L) {
  const aoflagger_lua::Data* dataAfter = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const aoflagger_lua::Data* dataBefore =
      reinterpret_cast<aoflagger_lua::Data*>(
          luaL_checkudata(L, 2, "AOFlaggerData"));

  lua_pushstring(L, "AOFlagger.ScriptData");
  lua_gettable(L, LUA_REGISTRYINDEX);
  ScriptData* scriptData = reinterpret_cast<ScriptData*>(lua_touserdata(L, -1));

  try {
    aoflagger_lua::collect_statistics(*dataAfter, *dataBefore, *scriptData);
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  return 0;
}

int Functions::copy_to_channel(lua_State* L) {
  aoflagger_lua::Data* destination = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const aoflagger_lua::Data* source =
      reinterpret_cast<const aoflagger_lua::Data*>(
          luaL_checkudata(L, 2, "AOFlaggerData"));
  long channel = luaL_checkinteger(L, 3);
  try {
    aoflagger_lua::copy_to_channel(*destination, *source, channel);
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  return 0;
}

int Functions::copy_to_frequency(lua_State* L) {
  aoflagger_lua::Data* destination = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const aoflagger_lua::Data* source =
      reinterpret_cast<const aoflagger_lua::Data*>(
          luaL_checkudata(L, 2, "AOFlaggerData"));
  double frequencyMHz = luaL_checknumber(L, 3);
  try {
    aoflagger_lua::copy_to_frequency(*destination, *source, frequencyMHz * 1e6);
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  return 0;
}

int Functions::downsample(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  long horizontalFactor = luaL_checkinteger(L, 2),
       verticalFactor = luaL_checkinteger(L, 3);
  if (!lua_isboolean(L, 4)) {
    luaL_error(
        L, "Parameters 4 should be of boolean type in call to downsample()");
  }
  bool masked = lua_toboolean(L, 4);
  try {
    if (masked)
      Tools::NewData(L, aoflagger_lua::downsample_masked(
                            *data, horizontalFactor, verticalFactor));
    else
      Tools::NewData(L, aoflagger_lua::downsample(*data, horizontalFactor,
                                                  verticalFactor));
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  return 1;
}

int Functions::high_pass_filter(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  long kernelWidth = luaL_checkinteger(L, 2),
       kernelHeight = luaL_checkinteger(L, 3);
  double horizontalSigmaSquared = luaL_checknumber(L, 4),
         verticalSigmaSquared = luaL_checknumber(L, 5);
  aoflagger_lua::high_pass_filter(*data, kernelWidth, kernelHeight,
                                  horizontalSigmaSquared, verticalSigmaSquared);
  return 0;
}

int Functions::low_pass_filter(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  long kernelWidth = luaL_checkinteger(L, 2),
       kernelHeight = luaL_checkinteger(L, 3);
  double horizontalSigmaSquared = luaL_checknumber(L, 4),
         verticalSigmaSquared = luaL_checknumber(L, 5);
  aoflagger_lua::low_pass_filter(*data, kernelWidth, kernelHeight,
                                 horizontalSigmaSquared, verticalSigmaSquared);
  return 0;
}

int Functions::normalize_bandpass(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  NormalizeBandpass::NormalizeSmooth(data->TFData());
  return 0;
}

int Functions::normalize_subbands(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  long nSubbands = luaL_checkinteger(L, 2);
  NormalizeBandpass::NormalizeStepwise(data->TFData(), nSubbands);
  return 0;
}

int Functions::print_polarization_statistics(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  aoflagger_lua::print_polarization_statistics(*data);
  return 0;
}

int Functions::require_min_version(lua_State* L) {
  VersionString minVersion;
  try {
    minVersion = VersionString(luaL_checkstring(L, 1));
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  bool met = minVersion.Major() < AOFLAGGER_VERSION_MAJOR;
  if (minVersion.Major() == AOFLAGGER_VERSION_MAJOR) {
    if (!minVersion.HasMinor() || minVersion.Minor() < AOFLAGGER_VERSION_MINOR)
      met = true;
    else if (minVersion.Minor() == AOFLAGGER_VERSION_MINOR &&
             (!minVersion.HasSubminor() ||
              minVersion.Subminor() <= AOFLAGGER_VERSION_SUBMINOR))
      met = true;
  }
  if (!met) {
    std::string err =
        std::string(
            "Requirements on AOFlagger version not met: This "
            "is " AOFLAGGER_VERSION_STR ", required is version >= ") +
        minVersion.String();
    luaL_error(L, err.c_str());
  }
  return 0;
}

int Functions::require_max_version(lua_State* L) {
  VersionString maxVersion;
  try {
    maxVersion = VersionString(luaL_checkstring(L, 1));
  } catch (std::exception& e) {
    return luaL_error(L, e.what());
  }
  bool met = maxVersion.Major() > AOFLAGGER_VERSION_MAJOR;
  if (maxVersion.Major() == AOFLAGGER_VERSION_MAJOR) {
    if (!maxVersion.HasMinor() || maxVersion.Minor() > AOFLAGGER_VERSION_MINOR)
      met = true;
    else if (maxVersion.Minor() == AOFLAGGER_VERSION_MINOR &&
             (!maxVersion.HasSubminor() ||
              maxVersion.Subminor() >= AOFLAGGER_VERSION_SUBMINOR))
      met = true;
  }
  if (!met) {
    std::string err =
        std::string(
            "Requirements on AOFlagger version not met: This "
            "is " AOFLAGGER_VERSION_STR ", required is version <= ") +
        maxVersion.String();
    luaL_error(L, err.c_str());
  }
  return 0;
}

int Functions::save_heat_map(lua_State* L) {
  const char* str = luaL_checklstring(L, 1, nullptr);
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  aoflagger_lua::save_heat_map(str, *data);
  return 0;
}

int Functions::scale_invariant_rank_operator(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const double level_horizontal = luaL_checknumber(L, 2);
  const double level_vertical = luaL_checknumber(L, 3);
  try {
    aoflagger_lua::scale_invariant_rank_operator(*data, level_horizontal,
                                                 level_vertical);
  } catch (std::exception& e) {
    luaL_error(L, e.what());
  }
  return 0;
}

int Functions::scale_invariant_rank_operator_masked(lua_State* L) {
  bool hasPenalty = lua_gettop(L) >= 5;
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const aoflagger_lua::Data* missing = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  const double level_horizontal = luaL_checknumber(L, 3);
  const double level_vertical = luaL_checknumber(L, 4);
  const double penalty = hasPenalty ? luaL_checknumber(L, 5) : 0.1;
  try {
    aoflagger_lua::scale_invariant_rank_operator_masked(
        *data, *missing, level_horizontal, level_vertical, penalty);
  } catch (std::exception& e) {
    luaL_error(L, e.what());
  }
  return 0;
}

int Functions::set_progress(lua_State* L) {
  double progress = luaL_checkinteger(L, 1),
         maxProgress = luaL_checkinteger(L, 2);
  lua_pushstring(L, "AOFlagger.ScriptData");
  lua_gettable(L, LUA_REGISTRYINDEX);
  ScriptData* scriptData = reinterpret_cast<ScriptData*>(lua_touserdata(L, -1));
  if (scriptData->Progress())
    scriptData->Progress()->OnProgress(progress, maxProgress);
  return 0;
}

int Functions::set_progress_text(lua_State* L) {
  std::string str = luaL_checklstring(L, 1, nullptr);
  lua_pushstring(L, "AOFlagger.ScriptData");
  lua_gettable(L, LUA_REGISTRYINDEX);
  ScriptData* scriptData = reinterpret_cast<ScriptData*>(lua_touserdata(L, -1));
  if (scriptData->Progress()) scriptData->Progress()->OnStartTask(str);
  return 0;
}

int Functions::sumthreshold(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  double hThresholdFactor = luaL_checknumber(L, 2),
         vThresholdFactor = luaL_checknumber(L, 3);
  if (!lua_isboolean(L, 4) || !lua_isboolean(L, 5)) {
    luaL_error(L,
               "Parameters 4 and 5 should be of boolean type in call to "
               "sumthreshold()");
  } else {
    bool horizontal = lua_toboolean(L, 4), vertical = lua_toboolean(L, 5);
    aoflagger_lua::sumthreshold(*data, hThresholdFactor, vThresholdFactor,
                                horizontal, vertical);
  }
  return 0;
}

int Functions::sumthreshold_masked(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const aoflagger_lua::Data* missing = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  double hThresholdFactor = luaL_checknumber(L, 3),
         vThresholdFactor = luaL_checknumber(L, 4);
  if (!lua_isboolean(L, 5) || !lua_isboolean(L, 6)) {
    luaL_error(L,
               "Parameters 5 and 6 should be of boolean type in call to "
               "sumthreshold()");
  } else {
    bool horizontal = lua_toboolean(L, 4), vertical = lua_toboolean(L, 5);
    aoflagger_lua::sumthreshold_masked(*data, *missing, hThresholdFactor,
                                       vThresholdFactor, horizontal, vertical);
  }
  return 0;
}

int Functions::threshold_channel_rms(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  double threshold = luaL_checknumber(L, 2);
  if (!lua_isboolean(L, 3)) {
    luaL_error(L,
               "3rd parameter should be of boolean type in call to "
               "threshold_channel_rms()");
  } else {
    bool thresholdLowValues = lua_toboolean(L, 3);
    aoflagger_lua::threshold_channel_rms(*data, threshold, thresholdLowValues);
  }
  return 0;
}

int Functions::threshold_timestep_rms(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  double threshold = luaL_checknumber(L, 2);
  aoflagger_lua::threshold_timestep_rms(*data, threshold);
  return 0;
}

int Functions::trim_channels(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  long start_channel = luaL_checkinteger(L, 2);
  long end_channel = luaL_checkinteger(L, 3);
  try {
    aoflagger_lua::Data trimmed_data =
        aoflagger_lua::trim_channels(*data, start_channel, end_channel);
    Tools::NewData(L, std::move(trimmed_data));
  } catch (std::exception& e) {
    luaL_error(L, e.what());
  }
  return 1;
}

int Functions::trim_frequencies(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  double start_frequency = luaL_checknumber(L, 2);
  double end_frequency = luaL_checknumber(L, 3);
  try {
    aoflagger_lua::Data trimmed_data = aoflagger_lua::trim_frequencies(
        *data, start_frequency * 1e6, end_frequency * 1e6);
    Tools::NewData(L, std::move(trimmed_data));
  } catch (std::exception& e) {
    luaL_error(L, e.what());
  }
  return 1;
}

int Functions::upsample_image(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  aoflagger_lua::Data* destination = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  long horizontalFactor = luaL_checkinteger(L, 3),
       verticalFactor = luaL_checkinteger(L, 4);
  try {
    aoflagger_lua::upsample_image(*data, *destination, horizontalFactor,
                                  verticalFactor);
  } catch (std::exception& e) {
    luaL_error(L, e.what());
  }
  return 0;
}

int Functions::upsample_mask(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  aoflagger_lua::Data* destination = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  long horizontalFactor = luaL_checkinteger(L, 3),
       verticalFactor = luaL_checkinteger(L, 4);
  try {
    aoflagger_lua::upsample_mask(*data, *destination, horizontalFactor,
                                 verticalFactor);
  } catch (std::exception& e) {
    luaL_error(L, e.what());
  }
  return 0;
}

int Functions::visualize(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  std::string label = luaL_checklstring(L, 2, nullptr);
  long sortingIndex = luaL_checkinteger(L, 3);
  lua_pushstring(L, "AOFlagger.ScriptData");
  lua_gettable(L, LUA_REGISTRYINDEX);
  ScriptData* scriptData = reinterpret_cast<ScriptData*>(lua_touserdata(L, -1));
  scriptData->AddVisualization(data->TFData(), label, sortingIndex);
  return 0;
}
