#include "data.h"

#include <cstring>

#include <boost/python/object.hpp>

namespace aoflagger_python
{	
	boost::python::object get_flag_function();
	
	void enlarge(const Data& input, Data& destination, size_t horizontalFactor, size_t verticalFactor);
	
	void high_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight, double horizontalSigmaSquared, double verticalSigmaSquared);
	
	void low_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight, double horizontalSigmaSquared, double verticalSigmaSquared);
	
	// TODO this function should collect the statistics and print
	// them later on (and be renamed).
	void print_polarization_statistics(const Data& data);
	
	void save_heat_map(const char* filename, const Data& data);
	
	void scale_invariant_rank_operator(Data& data, double level_horizontal, double level_vertical);
	
	void set_flag_function(PyObject* callable);
	
	Data shrink(const Data& data, size_t horizontalFactor, size_t verticalFactor);
	
	void sumthreshold(Data& data, double hThresholdFactor, double vThresholdFactor, bool horizontal, bool vertical);
	
	void threshold_channel_rms(Data& data, double threshold, bool thresholdLowValues);
	
	void threshold_timestep_rms(Data& data, double threshold);
}
