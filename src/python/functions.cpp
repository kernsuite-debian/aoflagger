#include "functions.h"
#include "scriptdata.h"

#include "../strategy/algorithms/applybandpass.h"
#include "../strategy/algorithms/highpassfilter.h"
#include "../strategy/algorithms/medianwindow.h"
#include "../strategy/algorithms/siroperator.h"
#include "../strategy/algorithms/thresholdconfig.h"

#include "../structures/image2d.h"
#include "../structures/samplerow.h"
#include "../structures/timefrequencydata.h"

#ifdef HAVE_GTKMM
#include "../plot/heatmapplot.h"
#endif

#include "../strategy/algorithms/polarizationstatistics.h"
#include "../strategy/algorithms/thresholdtools.h"

#include <boost/python.hpp>

using namespace boost::python;

#include <iostream>
namespace aoflagger_python
{

static object flagFunction;

void apply_bandpass(Data& data, const std::string& filename, ScriptData& scriptData)
{
	std::unique_ptr<BandpassFile>& bpFile = scriptData.GetBandpassFile();
	{
		std::lock_guard<std::mutex> lock(scriptData.BandpassMutex());
		if(bpFile == nullptr)
		{
			bpFile.reset(new BandpassFile(filename));
		}
	}
	ApplyBandpass::Apply(data.TFData(), *bpFile,
		data.MetaData()->Antenna1().name,
		data.MetaData()->Antenna2().name);
}

void upsample(const Data& input, Data& destination, size_t horizontalFactor, size_t verticalFactor)
{
	TimeFrequencyData timeFrequencyData = input.TFData();
	const size_t
		imageCount = timeFrequencyData.ImageCount(),
		newWidth = destination.TFData().ImageWidth(),
		newHeight = destination.TFData().ImageHeight();
	if(destination.TFData().ImageCount() != imageCount)
		throw std::runtime_error("Error in enlarge() call: source and image have different number of images");
	//std::cout << "Enlarging " << horizontalFactor << " x " << verticalFactor << " to " << newWidth << " x " << newHeight << '\n';
	
	if(horizontalFactor > 1)
	{
		for(size_t i=0;i<imageCount;++i)
		{
			Image2DPtr newImage(new Image2D(
				timeFrequencyData.GetImage(i)->EnlargeHorizontally(horizontalFactor, newWidth)));
			timeFrequencyData.SetImage(i, newImage);
		}
	}
	
	for(size_t i=0;i<imageCount;++i)
	{
		Image2DCPtr image = timeFrequencyData.GetImage(i);
		if(verticalFactor > 1)
		{
			Image2DPtr newImage(new Image2D(
				timeFrequencyData.GetImage(i)->EnlargeVertically(verticalFactor, newHeight)));
			destination.TFData().SetImage(i, newImage);
		}
		else {
			destination.TFData().SetImage(i, timeFrequencyData.GetImage(i));
		}
	}
}

object get_flag_function()
{
	return flagFunction;
}
	
void low_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight, double horizontalSigmaSquared, double verticalSigmaSquared)
{
	if(data.TFData().PolarizationCount() != 1)
		throw std::runtime_error("High-pass filtering needs single polarization");
	HighPassFilter filter;
	filter.SetHWindowSize(kernelWidth);
	filter.SetVWindowSize(kernelHeight);
	filter.SetHKernelSigmaSq(horizontalSigmaSquared);
	filter.SetVKernelSigmaSq(verticalSigmaSquared);
	Mask2DCPtr mask = data.TFData().GetSingleMask();
	size_t imageCount = data.TFData().ImageCount();
	
	for(size_t i=0;i<imageCount;++i)
		data.TFData().SetImage(i, filter.ApplyLowPass(data.TFData().GetImage(i), mask));
}

void high_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight, double horizontalSigmaSquared, double verticalSigmaSquared)
{
	if(data.TFData().PolarizationCount() != 1)
		throw std::runtime_error("High-pass filtering needs single polarization");
	HighPassFilter filter;
	filter.SetHWindowSize(kernelWidth);
	filter.SetVWindowSize(kernelHeight);
	filter.SetHKernelSigmaSq(horizontalSigmaSquared);
	filter.SetVKernelSigmaSq(verticalSigmaSquared);
	Mask2DCPtr mask = data.TFData().GetSingleMask();
	size_t imageCount = data.TFData().ImageCount();
	
	for(size_t i=0;i<imageCount;++i)
		data.TFData().SetImage(i, filter.ApplyHighPass(data.TFData().GetImage(i), mask));
}

void save_heat_map(const char* filename, const Data& data)
{
#ifdef HAVE_GTKMM
	const TimeFrequencyData tfData = data.TFData();
	HeatMapPlot plot;
	plot.SetImage(tfData.GetSingleImage());
	plot.SetAlternativeMask(tfData.GetSingleMask());
	plot.SaveByExtension(filename, 800, 500);
#else
	throw std::runtime_error("Compiled without GTKMM -- can not save heat map");
#endif
}

void print_polarization_statistics(const Data& data)
{
	PolarizationStatistics statistics;
	statistics.Add(data.TFData());
	statistics.Report();
}

void scale_invariant_rank_operator(Data& data, double level_horizontal, double level_vertical)
{
	Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
	
	SIROperator::OperateHorizontally(*mask, level_horizontal);
	SIROperator::OperateVertically(*mask, level_vertical);
	data.TFData().SetGlobalMask(mask);
}

void scale_invariant_rank_operator_with_missing(Data& data, const Data& missing, double level_horizontal, double level_vertical)
{
	Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
	
	Mask2DCPtr missingMask = missing.TFData().GetSingleMask();
	SIROperator::OperateHorizontallyMissing(*mask, *missingMask, level_horizontal);
	SIROperator::OperateVerticallyMissing(*mask, *missingMask, level_vertical);
	data.TFData().SetGlobalMask(mask);
}

void set_flag_function(PyObject* callable)
{
	flagFunction = object(boost::python::handle<>(boost::python::borrowed(callable)));
}

Data downsample(const Data& data, size_t horizontalFactor, size_t verticalFactor)
{
	TimeFrequencyData timeFrequencyData = data.TFData();
	const size_t imageCount = timeFrequencyData.ImageCount();
	const size_t maskCount = timeFrequencyData.MaskCount();
	
	if(horizontalFactor > 1)
	{
		for(size_t i=0;i<imageCount;++i)
		{
			Image2DPtr newImage(new Image2D(timeFrequencyData.GetImage(i)->ShrinkHorizontally(horizontalFactor)));
			timeFrequencyData.SetImage(i, newImage);
		}
		for(size_t i=0;i<maskCount;++i)
		{
			Mask2DPtr newMask(new Mask2D(timeFrequencyData.GetMask(i)->ShrinkHorizontally(horizontalFactor)));
			timeFrequencyData.SetMask(i, newMask);
		}
	}
	
	if(verticalFactor > 1)
	{
		for(size_t i=0;i<imageCount;++i)
		{
			Image2DPtr newImage(new Image2D(timeFrequencyData.GetImage(i)->ShrinkVertically(verticalFactor)));
			timeFrequencyData.SetImage(i, newImage);
		}
		for(size_t i=0;i<maskCount;++i)
		{
			Mask2DPtr newMask(new Mask2D(timeFrequencyData.GetMask(i)->ShrinkVertically(verticalFactor)));
			timeFrequencyData.SetMask(i, newMask);
		}
	}
	return Data(timeFrequencyData, data.MetaData());
}

Data downsample_masked(const Data& data, size_t horizontalFactor, size_t verticalFactor)
{
	TimeFrequencyData timeFrequencyData = data.TFData();
	
	// Decrease in horizontal direction
	size_t polCount = timeFrequencyData.PolarizationCount();
	for(size_t i=0; i<polCount; ++i)
	{
		TimeFrequencyData polData(timeFrequencyData.MakeFromPolarizationIndex(i));
		const Mask2DCPtr mask = polData.GetSingleMask();
		for(unsigned j=0; j<polData.ImageCount(); ++j)
		{
			const Image2DCPtr image = polData.GetImage(j);
			polData.SetImage(j, ThresholdTools::ShrinkHorizontally(horizontalFactor, image.get(), mask.get()));
		}
		timeFrequencyData.SetPolarizationData(i, std::move(polData));
	}
	size_t maskCount = timeFrequencyData.MaskCount();
	for(size_t i=0; i<maskCount; ++i)
	{
		Mask2DCPtr mask = timeFrequencyData.GetMask(i);
		Mask2DPtr newMask(new Mask2D(mask->ShrinkHorizontallyForAveraging(horizontalFactor)));
		timeFrequencyData.SetMask(i, std::move(newMask));
	}
	
	// Decrease in vertical direction
	for(size_t i=0; i<polCount; ++i)
	{
		TimeFrequencyData polData(timeFrequencyData.MakeFromPolarizationIndex(i));
		const Mask2DCPtr mask = polData.GetSingleMask();
		for(unsigned j=0;j<polData.ImageCount();++j)
		{
			const Image2DCPtr image = polData.GetImage(j);
			polData.SetImage(j, ThresholdTools::ShrinkVertically(verticalFactor, image.get(), mask.get()));
		}
		timeFrequencyData.SetPolarizationData(i, std::move(polData));
	}
	for(size_t i=0; i<maskCount; ++i)
	{
		Mask2DCPtr mask = timeFrequencyData.GetMask(i);
		Mask2DPtr newMask(new Mask2D(mask->ShrinkVerticallyForAveraging(verticalFactor)));
		timeFrequencyData.SetMask(i, std::move(newMask));
	}
	
	return Data(timeFrequencyData, data.MetaData());
}

static void sumthreshold_generic(Data& data, const Data* missing, double hThresholdFactor, double vThresholdFactor, bool horizontal, bool vertical)
{
	ThresholdConfig thresholdConfig;
	thresholdConfig.InitializeLengthsDefault();
	thresholdConfig.InitializeThresholdsFromFirstThreshold(6.0L, ThresholdConfig::Rayleigh);
	if(!horizontal)
		thresholdConfig.RemoveHorizontalOperations();
	if(!vertical)
		thresholdConfig.RemoveVerticalOperations();
	
	if(data.TFData().PolarizationCount() != 1)
		throw std::runtime_error("Input data in sum_threshold has wrong format");
	
	Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
	Image2DCPtr image = data.TFData().GetSingleImage();
	
	if(missing != nullptr)
	{
		Mask2DCPtr missingMask = missing->TFData().GetSingleMask(); 
		thresholdConfig.ExecuteWithMissing(image.get(), mask.get(), missingMask.get(), false, hThresholdFactor, vThresholdFactor);
	}
	else {
		thresholdConfig.Execute(image.get(), mask.get(), false, hThresholdFactor, vThresholdFactor);
	}
	data.TFData().SetGlobalMask(mask);
}

void sumthreshold(Data& data, double hThresholdFactor, double vThresholdFactor, bool horizontal, bool vertical)
{
	sumthreshold_generic(data, nullptr, hThresholdFactor, vThresholdFactor, horizontal, vertical);
}

void sumthreshold_with_missing(Data& data, const Data& missing, double hThresholdFactor, double vThresholdFactor, bool horizontal, bool vertical)
{
	sumthreshold_generic(data, &missing, hThresholdFactor, vThresholdFactor, horizontal, vertical);
}

void threshold_channel_rms(Data& data, double threshold, bool thresholdLowValues)
{
	Image2DCPtr image(data.TFData().GetSingleImage());
	SampleRow channels = SampleRow::MakeEmpty(image->Height());
	Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
	for(size_t y=0;y<image->Height();++y)
	{
		SampleRow row = SampleRow::MakeFromRowWithMissings(image.get(), mask.get(), y);
		channels.SetValue(y, row.RMSWithMissings());
	}
	bool change;
	do {
		num_t median = channels.MedianWithMissings();
		num_t stddev = channels.StdDevWithMissings(median);
		change = false;
		double effectiveThreshold = threshold * stddev;
		for(size_t y=0;y<channels.Size();++y)
		{
			if(!channels.ValueIsMissing(y) && (channels.Value(y) - median > effectiveThreshold || (thresholdLowValues && median - channels.Value(y) > effectiveThreshold)))
			{
				mask->SetAllHorizontally<true>(y);
				channels.SetValueMissing(y);
				change = true;
			}
		}
	} while(change);
	data.TFData().SetGlobalMask(std::move(mask));
}

void threshold_timestep_rms(Data& data, double threshold)
{
	Image2DCPtr image = data.TFData().GetSingleImage();
	SampleRow timesteps = SampleRow::MakeEmpty(image->Width());
	Mask2DPtr mask(new Mask2D(*data.TFData().GetSingleMask()));
	for(size_t x=0;x<image->Width();++x)
	{
		SampleRow row = SampleRow::MakeFromColumnWithMissings(image.get(), mask.get(), x);
		timesteps.SetValue(x, row.RMSWithMissings());
	}
	bool change;
	MedianWindow<num_t>::SubtractMedian(timesteps, 512);
	do {
		num_t median = 0.0;
		num_t stddev = timesteps.StdDevWithMissings(0.0);
		change = false;
		for(size_t x=0;x<timesteps.Size();++x)
		{
			if(!timesteps.ValueIsMissing(x) && (timesteps.Value(x) - median > stddev * threshold || median - timesteps.Value(x) > stddev * threshold))
			{
				mask->SetAllVertically<true>(x);
				timesteps.SetValueMissing(x);
				change = true;
			}
		}
	} while(change);
	data.TFData().SetGlobalMask(std::move(mask));
}

void visualize(Data& data, const std::string& label, size_t sortingIndex, ScriptData& scriptData)
{
	scriptData.AddVisualization(data.TFData(), label, sortingIndex);
}

}
