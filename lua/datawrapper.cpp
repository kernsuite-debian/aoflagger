#include "datawrapper.h"

#include "data.h"
#include "tools.h"

#include "../algorithms/restorechannelrange.h"

int Data::clear_mask(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  data->TFData().SetNoMask();
  return 0;
}

int Data::convert_to_complex(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  std::string reprStr = luaL_checklstring(L, 2, nullptr);
  try {
    enum TimeFrequencyData::ComplexRepresentation complexRepresentation;
    // PhasePart, AmplitudePart, RealPart, ImaginaryPart, ComplexParts
    if (reprStr == "phase")
      complexRepresentation = TimeFrequencyData::PhasePart;
    else if (reprStr == "amplitude")
      complexRepresentation = TimeFrequencyData::AmplitudePart;
    else if (reprStr == "real")
      complexRepresentation = TimeFrequencyData::RealPart;
    else if (reprStr == "imaginary")
      complexRepresentation = TimeFrequencyData::ImaginaryPart;
    else if (reprStr == "complex")
      complexRepresentation = TimeFrequencyData::ComplexParts;
    else {
      return luaL_error(
          L,
          "Unknown complex representation specified in convert_to_complex(): "
          "should be phase, amplitude, real, imaginary or complex");
    }
    Tools::NewData(L, data->TFData().Make(complexRepresentation),
                   data->MetaData(), data->GetContext());
    return 1;
  } catch (std::exception& e) {
    return luaL_error(
        L, (std::string("convert_to_complex(): ") + e.what()).c_str());
  }
}

int Data::convert_to_polarization(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  std::string polStr = luaL_checklstring(L, 2, nullptr);
  try {
    aocommon::PolarizationEnum polarization =
        aocommon::Polarization::ParseString(polStr);
    Tools::NewData(L, data->TFData().Make(polarization), data->MetaData(),
                   data->GetContext());
    return 1;
  } catch (std::exception& e) {
    return luaL_error(
        L, (std::string("convert_to_polarization(): ") + e.what()).c_str());
  }
}

int Data::copy(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  Tools::NewData(L, *data);
  return 1;
}

int Data::flag_zeros(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  Mask2DPtr mask(new Mask2D(*data->TFData().GetSingleMask()));
  Image2DCPtr image = data->TFData().GetSingleImage();
  for (unsigned y = 0; y < image->Height(); ++y) {
    for (unsigned x = 0; x < image->Width(); ++x) {
      if (image->Value(x, y) == 0.0) mask->SetValue(x, y, true);
    }
  }
  data->TFData().SetGlobalMask(mask);
  return 0;
}

int Data::flag_nans(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  TimeFrequencyData newData = data->TFData();
  for (size_t p = 0; p != newData.PolarizationCount(); ++p) {
    TimeFrequencyData singlePol = newData.MakeFromPolarizationIndex(p);
    Mask2DPtr mask = Mask2D::MakePtr(*singlePol.GetSingleMask());
    for (size_t i = 0; i != singlePol.ImageCount(); ++i) {
      Image2DCPtr image = singlePol.GetImage(i);
      for (unsigned y = 0; y < image->Height(); ++y) {
        for (unsigned x = 0; x < image->Width(); ++x) {
          if (!std::isfinite(image->Value(x, y))) mask->SetValue(x, y, true);
        }
      }
    }
    singlePol.SetGlobalMask(std::move(mask));
    newData.SetPolarizationData(p, std::move(singlePol));
  }
  data->TFData() = newData;
  return 0;
}

int Data::get_antenna1_index(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L, "Can't call Data.antenna1_index(): no metadata available");
  if (!data->MetaData()->HasAntenna1())
    luaL_error(L, "Can't call Data.antenna1_index(): antenna1 not in metadata");
  lua_pushinteger(L, data->MetaData()->Antenna1().id);
  return 1;
}

int Data::get_antenna1_name(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L, "Can't call Data.get_antenna1_name(): no metadata available");
  if (!data->MetaData()->HasAntenna1())
    luaL_error(L,
               "Can't call Data.get_antenna1_name(): antenna1 not in metadata");
  lua_pushstring(L, data->MetaData()->Antenna1().name.c_str());
  return 1;
}

int Data::get_antenna2_index(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L, "Can't call Data.antenna2_index(): no metadata available");
  if (!data->MetaData()->HasAntenna2())
    luaL_error(L, "Can't call Data.antenna2_index(): antenna1 not in metadata");
  lua_pushinteger(L, data->MetaData()->Antenna2().id);
  return 1;
}

int Data::get_antenna2_name(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L, "Can't call Data.get_antenna2_name(): no metadata available");
  if (!data->MetaData()->HasAntenna2())
    luaL_error(L,
               "Can't call Data.get_antenna2_name(): antenna1 not in metadata");
  lua_pushstring(L, data->MetaData()->Antenna2().name.c_str());
  return 1;
}

int Data::get_baseline_angle(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L,
               "Can't call Data.get_baseline_angle(): no metadata available");
  if (!data->MetaData()->HasBaseline())
    luaL_error(L,
               "Can't call Data.get_baseline_angle(): basesline information "
               "not in metadata");
  lua_pushnumber(L, data->MetaData()->Baseline().Angle());
  return 1;
}

int Data::get_baseline_distance(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(
        L, "Can't call Data.get_baseline_distance(): no metadata available");
  if (!data->MetaData()->HasBaseline())
    luaL_error(L,
               "Can't call Data.get_baseline_distance(): basesline information "
               "not in metadata");
  lua_pushnumber(L, data->MetaData()->Baseline().Distance());
  return 1;
}

int Data::get_baseline_vector(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L,
               "Can't call Data.get_baseline_vector(): no metadata available");
  if (!data->MetaData()->HasBaseline())
    luaL_error(L,
               "Can't call Data.get_baseline_vector(): basesline information "
               "not in metadata");
  lua_newtable(L);
  lua_pushstring(L, "x");
  lua_pushnumber(L, data->MetaData()->Baseline().DeltaX());
  lua_settable(L, -3);
  lua_pushstring(L, "y");
  lua_pushnumber(L, data->MetaData()->Baseline().DeltaY());
  lua_settable(L, -3);
  lua_pushstring(L, "z");
  lua_pushnumber(L, data->MetaData()->Baseline().DeltaZ());
  lua_settable(L, -3);
  return 1;
}

int Data::get_complex_state(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  switch (data->TFData().ComplexRepresentation()) {
    case TimeFrequencyData::PhasePart:
      lua_pushstring(L, "phase");
    case TimeFrequencyData::AmplitudePart:
      lua_pushstring(L, "amplitude");
    case TimeFrequencyData::RealPart:
      lua_pushstring(L, "real");
    case TimeFrequencyData::ImaginaryPart:
      lua_pushstring(L, "imaginary");
    case TimeFrequencyData::ComplexParts:
      lua_pushstring(L, "complex");
  }
  return 1;
}

int Data::get_frequencies(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(
        L, "Error in call to Data.get_frequencies(): no metadata available");
  if (!data->MetaData()->HasBand())
    luaL_error(
        L, "Error in call to Data.get_frequencies(): no metadata available");
  lua_newtable(L);
  const BandInfo& band = data->MetaData()->Band();
  for (size_t i = 0; i != band.channels.size(); ++i) {
    const ChannelInfo& c = band.channels[i];
    lua_pushnumber(L, c.frequencyHz);
    lua_rawseti(L, -2, i + 1);
  }
  return 1;
}

int Data::get_polarizations(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  const std::vector<aocommon::PolarizationEnum> pols =
      data->TFData().Polarizations();
  lua_createtable(L, pols.size(), 0);
  for (size_t i = 0; i != pols.size(); ++i) {
    aocommon::PolarizationEnum p = pols[i];
    lua_pushstring(L, aocommon::Polarization::TypeToShortString(p).c_str());
    lua_rawseti(L, -2, i + 1);
  }
  return 1;
}

int Data::get_times(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  if (!data->MetaData())
    luaL_error(L, "Error in call to Data.get_times(): no metadata available");
  if (!data->MetaData()->HasObservationTimes())
    luaL_error(L, "Error in call to Data.get_times(): no metadata available");
  lua_newtable(L);
  const std::vector<double>& times = data->MetaData()->ObservationTimes();
  for (size_t i = 0; i != times.size(); ++i) {
    lua_pushnumber(L, times[i]);
    lua_rawseti(L, -2, i + 1);
  }
  return 1;
}

int Data::has_metadata(lua_State* L) {
  const aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  bool hasMetaData;
  if (data->MetaData() != nullptr) {
    const TimeFrequencyMetaDataCPtr& md = data->MetaData();
    hasMetaData = md->HasAntenna1() && md->HasAntenna2() && md->HasBand() &&
                  // Since there are no attributes of the field that can yet be
                  // queried by Lua, for now Field is ignored: md->HasField() &&
                  md->HasObservationTimes();
  } else {
    hasMetaData = false;
  }
  lua_pushboolean(L, hasMetaData);
  return 1;
}

int Data::is_auto_correlation(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  bool isAuto =
      data->MetaData() && data->MetaData()->HasAntenna1() &&
      data->MetaData()->HasAntenna2() &&
      data->MetaData()->Antenna1().id == data->MetaData()->Antenna2().id;
  lua_pushboolean(L, isAuto);
  return 1;
}

int Data::is_complex(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  bool isComplex =
      data->TFData().ComplexRepresentation() == TimeFrequencyData::ComplexParts;
  lua_pushboolean(L, isComplex);
  return 1;
}

int Data::join_mask(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  aoflagger_lua::Data* other = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  try {
    data->TFData().JoinMask(other->TFData());
  } catch (std::exception& e) {
    return luaL_error(L, (std::string("join_mask(): ") + e.what()).c_str());
  }
  return 0;
}

int Data::set_mask(lua_State* L) {
  aoflagger_lua::Data *lhs = reinterpret_cast<aoflagger_lua::Data*>(
                          luaL_checkudata(L, 1, "AOFlaggerData")),
                      *rhs = reinterpret_cast<aoflagger_lua::Data*>(
                          luaL_checkudata(L, 2, "AOFlaggerData"));
  try {
    lhs->TFData().SetMask(rhs->TFData());
  } catch (std::exception& e) {
    return luaL_error(L, (std::string("set_mask(): ") + e.what()).c_str());
  }
  return 0;
}

int Data::set_mask_for_channel_range(lua_State* L) {
  aoflagger_lua::Data *lhs = reinterpret_cast<aoflagger_lua::Data*>(
                          luaL_checkudata(L, 1, "AOFlaggerData")),
                      *rhs = reinterpret_cast<aoflagger_lua::Data*>(
                          luaL_checkudata(L, 2, "AOFlaggerData"));
  double startMHz = luaL_checknumber(L, 3), endMHz = luaL_checknumber(L, 4);
  try {
    if (rhs->MetaData() != nullptr && rhs->MetaData()->HasBand())
      RestoreChannelRange::Execute(lhs->TFData(), rhs->TFData(),
                                   *rhs->MetaData(), startMHz, endMHz);
    else
      throw std::runtime_error(
          "set_mask_for_channel_range(): No spectral band information "
          "available!");
  } catch (std::exception& e) {
    return luaL_error(
        L, (std::string("set_mask_for_channel_range(): ") + e.what()).c_str());
  }
  return 0;
}

int Data::set_polarization_data(lua_State* L) {
  aoflagger_lua::Data* lhs = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  std::string polStr = luaL_checklstring(L, 2, nullptr);
  aoflagger_lua::Data* rhs = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 3, "AOFlaggerData"));
  try {
    aocommon::PolarizationEnum polarization =
        aocommon::Polarization::ParseString(polStr);
    size_t polIndex = lhs->TFData().GetPolarizationIndex(polarization);
    lhs->TFData().SetPolarizationData(polIndex, rhs->TFData());
  } catch (std::exception& e) {
    return luaL_error(
        L, (std::string("set_polarization_data(): ") + e.what()).c_str());
  }
  return 0;
}

int Data::set_visibilities(lua_State* L) {
  aoflagger_lua::Data* lhs = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  aoflagger_lua::Data* rhs = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  if (rhs->TFData().ImageCount() != lhs->TFData().ImageCount()) {
    std::string err =
        "set_visibilities() was executed with inconsistent data types: right "
        "hand side had " +
        std::to_string(rhs->TFData().ImageCount()) + ", destination had " +
        std::to_string(lhs->TFData().ImageCount());
    return luaL_error(L, err.c_str());
  }
  for (size_t i = 0; i != rhs->TFData().ImageCount(); ++i)
    lhs->TFData().SetImage(i, rhs->TFData().GetImage(i));
  return 0;
}

int Data::gc(lua_State* L) {
  aoflagger_lua::Data* data = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  data->~Data();
  return 0;
}

int Data::sub(lua_State* L) {
  aoflagger_lua::Data* lhs = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 1, "AOFlaggerData"));
  aoflagger_lua::Data* rhs = reinterpret_cast<aoflagger_lua::Data*>(
      luaL_checkudata(L, 2, "AOFlaggerData"));
  Tools::NewData(L,
                 TimeFrequencyData::MakeFromDiff(lhs->TFData(), rhs->TFData()),
                 lhs->MetaData(), lhs->GetContext());
  return 1;
}
