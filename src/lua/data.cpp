
#include "data.h"
#include "tools.h"

#include "../python/data.h"

#include "../strategy/algorithms/restorechannelrange.h"

int Data::convert_to_complex(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	std::string reprStr = luaL_checklstring(L, 2, nullptr);
	try {
		enum TimeFrequencyData::ComplexRepresentation complexRepresentation;
		// PhasePart, AmplitudePart, RealPart, ImaginaryPart, ComplexParts
		if(reprStr == "phase")
			complexRepresentation = TimeFrequencyData::PhasePart;
		else if(reprStr == "amplitude")
			complexRepresentation = TimeFrequencyData::AmplitudePart;
		else if(reprStr == "real")
			complexRepresentation = TimeFrequencyData::RealPart;
		else if(reprStr == "imaginary")
			complexRepresentation = TimeFrequencyData::ImaginaryPart;
		else if(reprStr == "complex")
			complexRepresentation = TimeFrequencyData::ComplexParts;
		else {
			lua_pushstring(L, "Unknown complex representation specified in convert_to_complex(): should be phase, amplitude, real, imaginary or complex");
			lua_error(L);
			return 0;
		}
		Tools::NewData(L, data->TFData().Make(complexRepresentation), data->MetaData());
		return 1;
	}
	catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
		return 1;
	}
}

int Data::convert_to_polarization(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	std::string polStr = luaL_checklstring(L, 2, nullptr);
	try {
		PolarizationEnum polarization = Polarization::ParseString(polStr);
		Tools::NewData(L, data->TFData().Make(polarization), data->MetaData());
		return 1;
	}
	catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
		return 0;
	}
}

int Data::copy(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	Tools::NewData(L, *data);
	return 1;
}

int Data::clear_mask(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	data->TFData().SetNoMask();
	return 0;
}

int Data::flag_zeros(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	Mask2DPtr mask(new Mask2D(*data->TFData().GetSingleMask()));
	Image2DCPtr image = data->TFData().GetSingleImage();
	for(unsigned y=0;y<image->Height();++y) {
		for(unsigned x=0;x<image->Width();++x) {
			if(image->Value(x, y) == 0.0)
				mask->SetValue(x, y, true);
		}
	}
	data->TFData().SetGlobalMask(mask);
	return 0;
}

int Data::get_antenna1_index(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.antenna1_index(): no metadata available");
	if(!data->MetaData()->HasAntenna1())
		luaL_error(L, "Can't call Data.antenna1_index(): antenna1 not in metadata");
	lua_pushinteger(L, data->MetaData()->Antenna1().id);
	return 1;
}

int Data::get_antenna1_name(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.get_antenna1_name(): no metadata available");
	if(!data->MetaData()->HasAntenna1())
		luaL_error(L, "Can't call Data.get_antenna1_name(): antenna1 not in metadata");
	lua_pushstring(L, data->MetaData()->Antenna1().name.c_str());
	return 1;
}

int Data::get_antenna2_index(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.antenna2_index(): no metadata available");
	if(!data->MetaData()->HasAntenna2())
		luaL_error(L, "Can't call Data.antenna2_index(): antenna1 not in metadata");
	lua_pushinteger(L, data->MetaData()->Antenna2().id);
	return 1;
}

int Data::get_antenna2_name(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.get_antenna2_name(): no metadata available");
	if(!data->MetaData()->HasAntenna2())
		luaL_error(L, "Can't call Data.get_antenna2_name(): antenna1 not in metadata");
	lua_pushstring(L, data->MetaData()->Antenna2().name.c_str());
	return 1;
}

int Data::get_baseline_angle(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.get_baseline_angle(): no metadata available");
	if(!data->MetaData()->HasBaseline())
		luaL_error(L, "Can't call Data.get_baseline_angle(): basesline information not in metadata");
	lua_pushnumber(L, data->MetaData()->Baseline().Angle());
	return 1;
}

int Data::get_baseline_distance(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.get_baseline_distance(): no metadata available");
	if(!data->MetaData()->HasBaseline())
		luaL_error(L, "Can't call Data.get_baseline_distance(): basesline information not in metadata");
	lua_pushnumber(L, data->MetaData()->Baseline().Distance());
	return 1;
}

int Data::get_baseline_vector(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Can't call Data.get_baseline_angle(): no metadata available");
	if(!data->MetaData()->HasBaseline())
		luaL_error(L, "Can't call Data.get_baseline_angle(): basesline information not in metadata");
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

int Data::get_frequencies(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Error in call to Data.get_frequencies(): no metadata available");
	if(!data->MetaData()->HasBand())
		luaL_error(L, "Error in call to Data.get_frequencies(): no metadata available");
	lua_newtable(L);
	const BandInfo& band = data->MetaData()->Band();
	for(size_t i=0; i!=band.channels.size(); ++i)
	{
		const ChannelInfo& c = band.channels[i];
		lua_pushnumber(L, c.frequencyHz);
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

int Data::get_times(lua_State* L)
{
	const aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	if(!data->MetaData())
		luaL_error(L, "Error in call to Data.get_times(): no metadata available");
	if(!data->MetaData()->HasObservationTimes())
		luaL_error(L, "Error in call to Data.get_times(): no metadata available");
	lua_newtable(L);
	const std::vector<double>& times = data->MetaData()->ObservationTimes();
	for(size_t i=0; i!=times.size(); ++i)
	{
		lua_pushnumber(L, times[i]);
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

int Data::join_mask(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	aoflagger_python::Data* other = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	try {
		data->TFData().JoinMask(other->TFData());
	}
	catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 0;
}

int Data::make_complex(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	try {
		Tools::NewData(L, TimeFrequencyData::MakeFromComplexCombination(data->TFData(), data->TFData()), data->MetaData());
	} catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 1;
}

int Data::polarizations(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	const std::vector<PolarizationEnum> pols = data->TFData().Polarizations();
	lua_createtable(L, pols.size(), 0);
	for(size_t i=0; i!=pols.size(); ++i)
	{
		PolarizationEnum p = pols[i];
		lua_pushstring(L, Polarization::TypeToShortString(p).c_str());
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

int Data::set_mask(lua_State* L)
{
	aoflagger_python::Data
		*lhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") ),
		*rhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	try {
		lhs->TFData().SetMask(rhs->TFData());
	} catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 0;
}

int Data::set_mask_for_channel_range(lua_State* L)
{
	aoflagger_python::Data
		*lhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") ),
		*rhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	double
	  startMHz = luaL_checknumber(L, 3),
	  endMHz = luaL_checknumber(L, 4);
	try {
		RestoreChannelRange::Execute(lhs->TFData(), rhs->TFData(), *rhs->MetaData(), startMHz, endMHz);
	} catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 0;
}

int Data::set_polarization_data(lua_State* L)
{
	aoflagger_python::Data* lhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	std::string polStr = luaL_checklstring(L, 2, nullptr);
	aoflagger_python::Data* rhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 3, "AOFlaggerData") );
	try {
		PolarizationEnum polarization = Polarization::ParseString(polStr);
		size_t polIndex = lhs->TFData().GetPolarizationIndex(polarization);
		lhs->TFData().SetPolarizationData(polIndex, rhs->TFData());
	} catch(std::exception& e)
	{
		lua_pushstring(L, e.what());
		lua_error(L);
	}
	return 0;
}

int Data::set_visibilities(lua_State* L)
{
	aoflagger_python::Data* lhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	aoflagger_python::Data* rhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	if(rhs->TFData().ImageCount() != lhs->TFData().ImageCount())
	{
		std::string err =
			"set_image() was executed with inconsistent data types: right hand side had " +
			std::to_string(rhs->TFData().ImageCount()) +
			", destination had " + std::to_string(lhs->TFData().ImageCount());
		lua_pushstring(L, err.c_str());
		lua_error(L);
		return 0;
	}
	for(size_t i=0; i!=rhs->TFData().ImageCount(); ++i)
		lhs->TFData().SetImage(i, rhs->TFData().GetImage(i));
	return 0;
}

int Data::gc(lua_State* L)
{
	aoflagger_python::Data* data = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	data->~Data();
	return 0;
}

int Data::sub(lua_State* L)
{
	aoflagger_python::Data* lhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 1, "AOFlaggerData") );
	aoflagger_python::Data* rhs = reinterpret_cast<aoflagger_python::Data*>( luaL_checkudata(L, 2, "AOFlaggerData") );
	Tools::NewData(L, TimeFrequencyData::MakeFromDiff(lhs->TFData(), rhs->TFData()), lhs->MetaData());
	return 1;
}
