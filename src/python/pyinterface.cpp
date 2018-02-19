#include <boost/python.hpp>
 
#include "data.h"
#include "functions.h"

#include "../structures/polarization.h"
 
BOOST_PYTHON_MODULE(aoflagger)
{
	using namespace boost::python;
	
	class_<aoflagger_python::Data>("Data")
		.def(self - self)
		.def("__copy__", &aoflagger_python::Data::copy)
		.def("clear_mask", &aoflagger_python::Data::clear_mask)
		.def("convert_to_polarization", &aoflagger_python::Data::convert_to_polarization)
		.def("convert_to_complex", &aoflagger_python::Data::convert_to_complex)
		.def("join_mask", &aoflagger_python::Data::join_mask)
		.def("make_complex", &aoflagger_python::Data::make_complex)
		.def("polarizations", &aoflagger_python::Data::polarizations)
		.def("set_image", &aoflagger_python::Data::set_image)
		.def("set_polarization_data", &aoflagger_python::Data::set_polarization_data);

	def("enlarge", aoflagger_python::enlarge);
	def("high_pass_filter", aoflagger_python::high_pass_filter);
	def("low_pass_filter", aoflagger_python::low_pass_filter);
	def("save_heat_map", aoflagger_python::save_heat_map);
	def("print_polarization_statistics", aoflagger_python::print_polarization_statistics);
	def("scale_invariant_rank_operator", aoflagger_python::scale_invariant_rank_operator);
	def("shrink", aoflagger_python::shrink);
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
}
