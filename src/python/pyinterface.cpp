#include <boost/python.hpp>

#include <boost/python/numpy.hpp>

#include "data.h"
#include "functions.h"

#include "../structures/polarization.h"

#include "../interface/aoflagger.h"

#include <iostream>

static boost::python::numpy::ndarray GetImageBuffer(const aoflagger::ImageSet* imageSet, size_t imageIndex)
{
	if(imageIndex >= imageSet->ImageCount())
		throw std::out_of_range("aoflagger.get_image_buffer: Image index out of bounds");
	namespace np = boost::python::numpy;
	const float* values = imageSet->ImageBuffer(imageIndex);
	Py_intptr_t shape[2] = { unsigned(imageSet->Height()), unsigned(imageSet->Width()) };
	np::ndarray result = np::zeros(2, shape, np::dtype::get_builtin<double>());
	char* resultData = result.get_data();
	int stride0 = result.get_strides()[0];
	int stride1 = result.get_strides()[1];
	for(size_t y=0; y!=imageSet->Height(); ++y)
	{
		const float* rowOut = values + y * imageSet->HorizontalStride();
		char* rowIn = resultData + y * stride0;
		for(size_t x=0; x!=imageSet->Width(); ++x)
		{
			*reinterpret_cast<double*>(rowIn + x * stride1) = rowOut[x];
		}
	}
	return result;
}

static void SetImageBuffer(aoflagger::ImageSet* imageSet, size_t imageIndex, const boost::python::numpy::ndarray& values)
{
	if(imageIndex >= imageSet->ImageCount())
		throw std::out_of_range("aoflagger.get_image_buffer: Image index out of bounds");
	namespace np = boost::python::numpy;
	if(values.get_dtype() != np::dtype::get_builtin<double>())
		throw std::runtime_error("ImageSet.set_image_buffer(): Invalid type specified for data array; double numpy array required");
	if(values.shape(0) != int(imageSet->Height()) || values.shape(1) != int(imageSet->Width()))
		throw std::runtime_error("ImageSet.set_image_buffer(): dimensions of provided array doesn't match with image set");
	int stride0 = values.get_strides()[0];
	int stride1 = values.get_strides()[1];
	const char *data = values.get_data();
	if(!data)
		throw std::runtime_error("Data needs to be provided that is interpretable as a double array");
	float* buffer = imageSet->ImageBuffer(imageIndex);
	for(size_t y=0; y!=imageSet->Height(); ++y)
	{
		const char* rowIn = data + y * stride0;
		float* rowOut = buffer + y * imageSet->HorizontalStride();
		for(size_t x=0; x!=imageSet->Width(); ++x)
		{
			rowOut[x] = *reinterpret_cast<const double*>(rowIn + x*stride1);
		}
	}
}

static boost::python::numpy::ndarray GetBuffer(const aoflagger::FlagMask* flagMask)
{
	namespace np = boost::python::numpy;
	const bool* values = flagMask->Buffer();
	Py_intptr_t shape[2] = { unsigned(flagMask->Height()), unsigned(flagMask->Width()) };
	np::ndarray result = np::zeros(2, shape, np::dtype::get_builtin<bool>());
	char* resultData = result.get_data();
	int stride0 = result.get_strides()[0];
	int stride1 = result.get_strides()[1];
	for(size_t y=0; y!=flagMask->Height(); ++y)
	{
		const bool* rowOut = values + y * flagMask->HorizontalStride();
		char* rowIn = resultData + y * stride0;
		for(size_t x=0; x!=flagMask->Width(); ++x)
		{
			*reinterpret_cast<bool*>(rowIn + x * stride1) = rowOut[x];
		}
	}
	return result;
}

static void SetBuffer(aoflagger::FlagMask* flagMask, const boost::python::numpy::ndarray& values)
{
	namespace np = boost::python::numpy;
	if(values.get_dtype() != np::dtype::get_builtin<bool>())
		throw std::runtime_error("FlagMask.set_buffer(): Invalid type specified for data array; double numpy array required");
	if(values.get_nd() != 2)
		throw std::runtime_error("FlagMask.set_buffer(): Invalid dimensions specified for data array; two dimensional array required");
	if(values.shape(0) != int(flagMask->Height()) || values.shape(1) != int(flagMask->Width()))
		throw std::runtime_error("FlagMask.set_buffer(): dimensions of provided array doesn't match with image set");
	const char *data = values.get_data();
	if(!data)
		throw std::runtime_error("Data needs to be provided that is interpretable as a bool array");
	bool* buffer = flagMask->Buffer();
	int stride0 = values.get_strides()[0];
	int stride1 = values.get_strides()[1];
	for(size_t y=0; y!=flagMask->Height(); ++y)
	{
		const char* rowIn = data + y * stride0;
		bool* rowOut = buffer + y * flagMask->HorizontalStride();
		for(size_t x=0; x!=flagMask->Width(); ++x)
		{
			rowOut[x] = *reinterpret_cast<const double*>(rowIn + x*stride1);
		}
	}
}

boost::python::object MakeImageSet1(aoflagger::AOFlagger* flagger, size_t width, size_t height, size_t count)
{ return boost::python::object(flagger->MakeImageSet(width, height, count)); }

boost::python::object MakeImageSet2(aoflagger::AOFlagger* flagger, size_t width, size_t height, size_t count, size_t widthCapacity)
{ return boost::python::object(flagger->MakeImageSet(width, height, count, widthCapacity)); }

boost::python::object MakeImageSet3(aoflagger::AOFlagger* flagger, size_t width, size_t height, size_t count, float initialValue)
{ return boost::python::object(flagger->MakeImageSet(width, height, count, initialValue)); }

boost::python::object MakeImageSet4(aoflagger::AOFlagger* flagger, size_t width, size_t height, size_t count, float initialValue, size_t widthCapacity)
{ return boost::python::object(flagger->MakeImageSet(width, height, count, initialValue, widthCapacity)); }

boost::python::object MakeFlagMask1(aoflagger::AOFlagger* flagger, size_t width, size_t height)
{ return boost::python::object(flagger->MakeFlagMask(width, height)); }

boost::python::object MakeFlagMask2(aoflagger::AOFlagger* flagger, size_t width, size_t height, bool initialValue)
{ return boost::python::object(flagger->MakeFlagMask(width, height, initialValue)); }

boost::python::object MakeStrategy1(aoflagger::AOFlagger* flagger, enum aoflagger::TelescopeId telescopeId)
{ return boost::python::object(flagger->MakeStrategy(telescopeId)); }

boost::python::object MakeStrategy2(aoflagger::AOFlagger* flagger, enum aoflagger::TelescopeId telescopeId, unsigned strategyFlags)
{ return boost::python::object(flagger->MakeStrategy(telescopeId, strategyFlags)); }

boost::python::object MakeStrategy3(aoflagger::AOFlagger* flagger, enum aoflagger::TelescopeId telescopeId, unsigned strategyFlags, double frequency)
{ return boost::python::object(flagger->MakeStrategy(telescopeId, strategyFlags, frequency)); }

boost::python::object MakeStrategy4(aoflagger::AOFlagger* flagger, enum aoflagger::TelescopeId telescopeId, unsigned strategyFlags, double frequency, double timeRes)
{ return boost::python::object(flagger->MakeStrategy(telescopeId, strategyFlags, frequency, timeRes)); }

boost::python::object MakeStrategy5(aoflagger::AOFlagger* flagger, enum aoflagger::TelescopeId telescopeId, unsigned strategyFlags, double frequency, double timeRes, double frequencyRes)
{ return boost::python::object(flagger->MakeStrategy(telescopeId, strategyFlags, frequency, timeRes, frequencyRes)); }

boost::python::object LoadStrategy(aoflagger::AOFlagger* flagger, const char* filename)
{ return boost::python::object(flagger->LoadStrategy(std::string(filename))); }

boost::python::object Run1(aoflagger::AOFlagger* flagger, aoflagger::Strategy& strategy, const aoflagger::ImageSet& input)
{ return boost::python::object(flagger->Run(strategy, input)); }

boost::python::object Run2(aoflagger::AOFlagger* flagger, aoflagger::Strategy& strategy, const aoflagger::ImageSet& input, const aoflagger::FlagMask& existingFlags)
{ return boost::python::object(flagger->Run(strategy, input, existingFlags)); }

boost::python::object MakeQualityStatistics1(aoflagger::AOFlagger* flagger, const boost::python::numpy::ndarray& scanTimes, const boost::python::numpy::ndarray& channelFrequencies, size_t nPolarizations, bool computeHistograms)
{
	namespace np = boost::python::numpy;
	
	if(scanTimes.get_dtype() != np::dtype::get_builtin<double>())
		throw std::runtime_error("AOFlagger.make_quality_statistics(): Invalid type specified for scanTimes array; double numpy array required");
	if(scanTimes.get_nd() != 1)
		throw std::runtime_error("AOFlagger.make_quality_statistics(): Invalid dimensions specified for scanTimes array; one dimensional array required");
	size_t nScans = scanTimes.shape(0);
	const double *scanTimesArr = reinterpret_cast<const double*>(scanTimes.get_data());
	if(!scanTimesArr)
		throw std::runtime_error("scanTimes data needs to be provided that is interpretable as a double array");
	
	if(scanTimes.get_dtype() != np::dtype::get_builtin<double>())
		throw std::runtime_error("AOFlagger.make_quality_statistics(): Invalid type specified for channelFrequencies array; double numpy array required");
	if(scanTimes.get_nd() != 1)
		throw std::runtime_error("AOFlagger.make_quality_statistics(): Invalid dimensions specified for channelFrequencies array; one dimensional array required");
	size_t nChannels = channelFrequencies.shape(0);
	const double *channelFrequenciesArr = reinterpret_cast<const double*>(channelFrequencies.get_data());
	if(!channelFrequenciesArr)
		throw std::runtime_error("Data needs to be provided that is interpretable as a double array");
	
	return boost::python::object(flagger->MakeQualityStatistics(scanTimesArr, nScans, channelFrequenciesArr, nChannels, nPolarizations, computeHistograms));
}

boost::python::object MakeQualityStatistics2(aoflagger::AOFlagger* flagger, const boost::python::numpy::ndarray& scanTimes, const boost::python::numpy::ndarray& channelFrequencies, size_t nPolarizations)
{
	return MakeQualityStatistics1(flagger, scanTimes, channelFrequencies, nPolarizations, false);
}

BOOST_PYTHON_MODULE(aoflagger)
{
	using namespace boost::python;
	
	boost::python::numpy::initialize();
	
	class_<aoflagger_python::Data>("Data")
		.def(self - self)
		.def("__copy__", &aoflagger_python::Data::copy)
		.def("clear_mask", &aoflagger_python::Data::clear_mask)
		.def("convert_to_polarization", &aoflagger_python::Data::convert_to_polarization)
		.def("convert_to_complex", &aoflagger_python::Data::convert_to_complex)
		.def("join_mask", &aoflagger_python::Data::join_mask)
		.def("make_complex", &aoflagger_python::Data::make_complex)
		.def("polarizations", &aoflagger_python::Data::polarizations)
		.def("set_visibilities", &aoflagger_python::Data::set_visibilities)
		.def("set_polarization_data", &aoflagger_python::Data::set_polarization_data);

	def("upsample", aoflagger_python::upsample);
	def("high_pass_filter", aoflagger_python::high_pass_filter);
	def("low_pass_filter", aoflagger_python::low_pass_filter);
	def("save_heat_map", aoflagger_python::save_heat_map);
	def("print_polarization_statistics", aoflagger_python::print_polarization_statistics);
	def("scale_invariant_rank_operator", aoflagger_python::scale_invariant_rank_operator);
	def("downsample", aoflagger_python::downsample);
	def("sumthreshold", aoflagger_python::sumthreshold);
	def("set_flag_function", aoflagger_python::set_flag_function);
	def("threshold_channel_rms", aoflagger_python::threshold_channel_rms);
	def("threshold_timestep_rms", aoflagger_python::threshold_timestep_rms);
	
	enum_<PolarizationEnum>("Polarization")
		.value("StokesI", Polarization::StokesI)
		.value("StokesQ", Polarization::StokesQ)
		.value("StokesU", Polarization::StokesU)
		.value("StokesV", Polarization::StokesV)
		.value("RR", Polarization::RR)
		.value("RL", Polarization::RL)
		.value("LR", Polarization::LR)
		.value("LL", Polarization::LL)
		.value("XX", Polarization::XX)
		.value("XY", Polarization::XY)
		.value("YX", Polarization::YX)
		.value("YY", Polarization::YY);
		
	enum_<enum TimeFrequencyData::ComplexRepresentation>("ComplexRepresentation")
		.value("RealPart", TimeFrequencyData::RealPart)
		.value("ImaginaryPart", TimeFrequencyData::ImaginaryPart)
		.value("PhasePart", TimeFrequencyData::PhasePart)
		.value("AmplitudePart", TimeFrequencyData::AmplitudePart)
		.value("ComplexParts", TimeFrequencyData::ComplexParts);
		
	enum_<enum aoflagger::TelescopeId>("TelescopeId")
		.value("Generic", aoflagger::GENERIC_TELESCOPE)
		.value("AARTFAAC", aoflagger::AARTFAAC_TELESCOPE)
		.value("Arecibo", aoflagger::ARECIBO_TELESCOPE)
		.value("Bighorns", aoflagger::BIGHORNS_TELESCOPE)
		.value("JVLA", aoflagger::JVLA_TELESCOPE)
		.value("LOFAR", aoflagger::LOFAR_TELESCOPE)
		.value("MWA", aoflagger::MWA_TELESCOPE)
		.value("Parkes", aoflagger::PARKES_TELESCOPE)
		.value("WSRT", aoflagger::WSRT_TELESCOPE);

	class_<aoflagger::StrategyFlags>("StrategyFlags", no_init)
		.def_readonly("NONE", &aoflagger::StrategyFlags::NONE)
		.def_readonly("LARGE_BANDWIDTH", &aoflagger::StrategyFlags::LARGE_BANDWIDTH)
		.def_readonly("SMALL_BANDWIDTH", &aoflagger::StrategyFlags::SMALL_BANDWIDTH)
		.def_readonly("TRANSIENTS", &aoflagger::StrategyFlags::TRANSIENTS)
		.def_readonly("ROBUST", &aoflagger::StrategyFlags::ROBUST)
		.def_readonly("FAST", &aoflagger::StrategyFlags::FAST)
		.def_readonly("INSENSITIVE", &aoflagger::StrategyFlags::INSENSITIVE)
		.def_readonly("SENSITIVE", &aoflagger::StrategyFlags::SENSITIVE)
		.def_readonly("USE_ORIGINAL_FLAGS", &aoflagger::StrategyFlags::USE_ORIGINAL_FLAGS)
		.def_readonly("AUTO_CORRELATION", &aoflagger::StrategyFlags::AUTO_CORRELATION)
		.def_readonly("HIGH_TIME_RESOLUTION", &aoflagger::StrategyFlags::HIGH_TIME_RESOLUTION);

	class_<aoflagger::ImageSet>("ImageSet",
			"A set of time-frequency 'images' which together contain data for one \n"
			"correlated baseline or dish. \n"
			"The class either holds 1, 2, 4 or 8 images. These images have time on the \n"
			"x-axis (most rapidly changing index) and frequency on the y-axis. The \n"
			"cells specify flux levels, which do not need to have been calibrated. \n"
			"\n" 
			"If the set contains only one image, it specifies amplitudes of a single \n"
			"polarization. If it contains two images, it specifies the real and imaginary \n"
			"parts of a single polarization. With four images, it contains the real \n"
			"and imaginary values of two polarizations (ordered real pol A, imag pol A, \n"
			"real pol B, imag pol B). With eight images, it contains complex values for \n"
			"four correlated polarizations (ordered real pol A, imag pol A, real pol B, \n"
			"... etc). \n"
			"\n"
			"This class wraps the C++ class aoflagger::ImageSet.\n"
		)
		.def("width", &aoflagger::ImageSet::Width, "Get width (number of time steps) of images")
		.def("height", &aoflagger::ImageSet::Height, "Get height (number of frequency channels) of images")
		.def("image_count", &aoflagger::ImageSet::ImageCount, "Get number of images, see class description for details")
		.def("horizontal_stride", &aoflagger::ImageSet::HorizontalStride)
		.def("set", &aoflagger::ImageSet::Set, "Set all samples to the specified value")
		.def("get_image_buffer", GetImageBuffer,
			"Get access to one of the image sets stored in this object. \n"
			"Returns a numpy double array of ntimes x nchannels.")
		.def("set_image_buffer", SetImageBuffer,
			"Replace the data of one of the image sets. This function expects\n"
			"a numpy double array of ntimes x nchannels.")
		.def("resize_without_reallocation", &aoflagger::ImageSet::ResizeWithoutReallocation);
	
	class_<aoflagger::FlagMask>("FlagMask",
			"A two-dimensional flag mask.\n\n"
			"The flag mask specifies which values in an ImageSet are flagged.\n"
			"A value true means a value is flagged, i.e., contains RFI and should\n"
			"not be used in further data processing (calibration, imaging, etc.).\n"
			"A flag denotes that all the value at that time-frequency position should\n"
			"be ignored for all polarizations.\n"
			"\n"
			"If polarization-specific flags are needed, one could run the flagger on\n"
			"each polarization individually. However, note that some algorithms, like\n"
			"the morphological scale-invariant rank operator (SIR operator), work best\n"
			"when seeing the flags from all polarizations.\n"
			"\n"
			"This class wraps the C++ class aoflagger::FlagMask."
		)
		.def("width", &aoflagger::FlagMask::Width, "Get width (number of time steps) of flag mask")
		.def("height", &aoflagger::FlagMask::Height, "Get height (number of frequency channels) of flag mask")
		.def("horizontal_stride", &aoflagger::FlagMask::HorizontalStride)
		.def("get_buffer", GetBuffer, "Returns the flag mask as a bool numpy array with dimensions ntimes x nchannels.")
		.def("set_buffer", SetBuffer, "Sets the flag mask from a bool numpy array with dimensions ntimes x nchannels.");
		
	class_<aoflagger::Strategy>("Strategy",
		"Holds a flagging strategy.\n\n"
		"Telescope-specific flagging strategies can be created with \n"
		"AOFlagger.make_strategy(), or "
		"can be loaded from disk with AOFlagger.load_strategy(). Strategies\n"
		"can not be changed with this interface. A user can create strategies\n"
		"with the @c rfigui tool that is part of the aoflagger package.",
		no_init
	);
	
	class_<aoflagger::QualityStatistics>("QualityStatistics")
		.def(self += self);
	
	class_<aoflagger::AOFlagger, boost::noncopyable>("AOFlagger",
			"Main class that gives access to the aoflagger functions.")
		.def("make_image_set", MakeImageSet1)
		.def("make_image_set", MakeImageSet2)
		.def("make_image_set", MakeImageSet3)
		.def("make_image_set", MakeImageSet4)
		.def("make_flag_mask", MakeFlagMask1)
		.def("make_flag_mask", MakeFlagMask2)
		.def("make_strategy", MakeStrategy1)
		.def("make_strategy", MakeStrategy2)
		.def("make_strategy", MakeStrategy3)
		.def("make_strategy", MakeStrategy4)
		.def("make_strategy", MakeStrategy5)
		.def("load_strategy", LoadStrategy)
		.def("run", Run1)
		.def("run", Run2)
		.def("make_quality_statistics", MakeQualityStatistics1)
		.def("make_quality_statistics", MakeQualityStatistics2)
		.def("collect_statistics", &aoflagger::AOFlagger::CollectStatistics)
		.def("write_statistics", &aoflagger::AOFlagger::WriteStatistics)
		.def("get_version_string", &aoflagger::AOFlagger::GetVersionString).staticmethod("get_version_string")
		.def("get_version_date", &aoflagger::AOFlagger::GetVersionDate).staticmethod("get_version_date");
}
