#include "rfiplots.h"

#include <cmath>
#include <iostream>

#include "../../util/plot.h"
#include "../../util/multiplot.h"

#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include "../algorithms/sinusfitter.h"
#include "../algorithms/thresholdtools.h"

#include "../../plot/plot2d.h"

void RFIPlots::Bin(Image2DCPtr image, Mask2DCPtr mask, std::vector<size_t> &valuesOutput, std::vector<long double> &binsOutput, size_t binCount, long double start, long double end, long double factor, long double stretch) throw()
{
	const long double min = start==end ? ThresholdTools::MinValue(image.get(), mask.get()) : start;
	const long double max = start==end ? ThresholdTools::MaxValue(image.get(), mask.get()) : end;
	const long double binsize = (max-min) / binCount;
	valuesOutput.resize(binCount);
	binsOutput.resize(binCount);
	for(size_t i=0;i<binCount;++i) {
		valuesOutput[i] = 0;
		binsOutput[i] = (binsize * ((long double) i + 0.5)) + min;
	}
	for(size_t y=0;y<image->Height();++y) {
		for(size_t x=0;x<image->Width();++x) {
			if(!mask->Value(x, y)) {
				long double value = image->Value(x, y);
				size_t index = (size_t) ((value * stretch - min) / binsize);
				if(index < binCount)
					valuesOutput[index] += 1;
			}
		}
	}
	if(factor != 1.0) {
		for(size_t i=0;i<binCount;++i) {
			valuesOutput[i] = (size_t) (factor * valuesOutput[i]);
		}
	}
}

void RFIPlots::MakeDistPlot(Plot2DPointSet &pointSet, Image2DCPtr image, Mask2DCPtr mask)
{
	std::vector<size_t> valuesOutput;
	std::vector<long double> binsOutput;
	pointSet.SetXDesc("Visibility");
	pointSet.SetYDesc("Occurences");

	num_t mean, stddev;
	num_t min = image->GetMinimum();
	num_t max = image->GetMaximum();
	ThresholdTools::WinsorizedMeanAndStdDev(image.get(), mean, stddev);
	if(min < mean-3.0L*stddev)
		min = mean-3.0L*stddev;
	if(max > mean+3.0L*stddev)
		max = mean+3.0L*stddev;

	Bin(image, mask, valuesOutput, binsOutput, 40, min, max);
	for(unsigned i=0;i<valuesOutput.size();++i)
		pointSet.PushDataPoint(binsOutput[i], valuesOutput[i]);
}

template <bool Weight>
void RFIPlots::MakeMeanSpectrumPlot(Plot2DPointSet &pointSet, const TimeFrequencyData &data, const Mask2DCPtr &mask, const TimeFrequencyMetaDataCPtr &metaData)
{
	bool hasBandInfo = metaData != 0 && metaData->HasBand();
	if(hasBandInfo)
	{
		pointSet.SetXDesc("Frequency (MHz)");
		std::stringstream yDesc;
		yDesc << metaData->ValueDescription() << " (" << metaData->ValueUnits() << ')';
		pointSet.SetYDesc(yDesc.str());
	} else {
		pointSet.SetXDesc("Index");
		pointSet.SetYDesc("Mean (undefined units)");
	}
	
	TimeFrequencyData displayData = data;
	if(displayData.ComplexRepresentation() == TimeFrequencyData::ComplexParts)
	{
		displayData = data.Make(TimeFrequencyData::AmplitudePart);
	}

	long double min = 1e100, max = -1e100;
	const size_t height = data.ImageHeight(), width = data.ImageWidth();

	for(size_t y=0;y<height;++y) {
		long double sum = 0.0L;
		size_t count = 0;
		for(size_t i=0;i<displayData.ImageCount();++i)
		{
			Image2DCPtr image = displayData.GetImage(i);
			for(size_t x=0;x<width;++x) {
				if(!mask->Value(x, y) && std::isnormal(image->Value(x, y))) {
					sum += image->Value(x, y);
					++count;
				}
			}
		}
		if(count > 0)
		{
			long double v;
			if(Weight)
				v = sum;
			else
				v = sum/count;
			if(v < min) min = v;
			if(v > max) max = v;
			if(hasBandInfo)
				pointSet.PushDataPoint(metaData->Band().channels[y].frequencyHz/1000000.0, v);
			else
				pointSet.PushDataPoint(y, v);
		}
	}
	pointSet.SetYRange(min * 0.9, max / 0.9);
}
template void RFIPlots::MakeMeanSpectrumPlot<true>(class Plot2DPointSet &pointSet, const TimeFrequencyData &data, const Mask2DCPtr &mask, const TimeFrequencyMetaDataCPtr &metaData);
template void RFIPlots::MakeMeanSpectrumPlot<false>(class Plot2DPointSet &pointSet, const TimeFrequencyData &data, const Mask2DCPtr &mask, const TimeFrequencyMetaDataCPtr &metaData);

void RFIPlots::MakePowerSpectrumPlot(Plot2DPointSet &pointSet, Image2DCPtr image, Mask2DCPtr mask, TimeFrequencyMetaDataCPtr metaData)
{
	bool hasBandInfo = metaData != 0 && metaData->HasBand();
	if(hasBandInfo)
	{
		pointSet.SetXDesc("Frequency (MHz)");
		std::stringstream yDesc;
		yDesc << metaData->ValueDescription() << " (" << metaData->ValueUnits() << ')';
		pointSet.SetYDesc(yDesc.str());
	} else {
		pointSet.SetXDesc("Index");
		pointSet.SetYDesc("Power (undefined units)");
	}

	long double min = 1e100, max = 0.0;

	for(size_t y=0;y<image->Height();++y) {
		long double sum = 0.0L;
		size_t count = 0;
		for(size_t x=0;x<image->Width();++x) {
			if(!mask->Value(x, y) && std::isnormal(image->Value(x, y))) {
				sum += image->Value(x, y);
				++count;
			}
		}
		if(count > 0)
		{
			long double v = sum/count;
			if(v < min) min = v;
			if(v > max) max = v;
			if(hasBandInfo)
				pointSet.PushDataPoint(metaData->Band().channels[y].frequencyHz/1000000.0, v);
			else
				pointSet.PushDataPoint(y, v);
		}
	}
	pointSet.SetYRange(min * 0.9, max / 0.9);
}

void RFIPlots::MakePowerTimePlot(Plot2DPointSet &pointSet, Image2DCPtr image, Mask2DCPtr mask, TimeFrequencyMetaDataCPtr metaData)
{
	pointSet.SetXDesc("Time (s)");
	pointSet.SetYDesc("Visibility");
	bool useMeta;
	double firstTimeStep;
	if(metaData != 0 && metaData->HasObservationTimes()) {
		useMeta = true;
		firstTimeStep = metaData->ObservationTimes()[0];
	}
	else {
		useMeta = false;
		firstTimeStep = 0;
	}
	
	size_t binSize = (size_t) ceil(image->Width() / 256.0L);

	unsigned index = 0;
	for(size_t x=0;x<image->Width();x += binSize) {
		long double sum = 0.0L;
		size_t count = 0;
		for(size_t binx=0;binx<binSize;++binx) {
			for(size_t y=0;y<image->Height();++y) {
				if(!mask->Value(x + binx, y) && std::isnormal(image->Value(x + binx, y))) {
					sum += image->Value(x + binx, y);
					++count;
				}
			}
		}
		if(useMeta)
			pointSet.PushDataPoint(metaData->ObservationTimes()[x] - firstTimeStep, sum / count);
		else
			pointSet.PushDataPoint(index, sum / count);
		++index;
	}
}

void RFIPlots::MakeComplexPlanePlot(Plot2DPointSet &pointSet, const TimeFrequencyData &data, size_t xStart, size_t length, size_t y, size_t yAvgSize, Mask2DCPtr mask, bool realVersusImaginary, bool drawImaginary)
{

	if(realVersusImaginary)
	{
		pointSet.SetXDesc("real");
		pointSet.SetYDesc("imaginary");
	} else {
		//pointSet.SetXRange(xStart, xStart+length-1);
		pointSet.SetXDesc("time");
		pointSet.SetYDesc("real/imaginary visibility");
	}

	Image2DCPtr real = data.GetRealPart();
	Image2DCPtr imaginary = data.GetImaginaryPart();

	for(size_t x=xStart;x<xStart + length;++x)
	{
		long double r = 0.0L, i = 0.0L;
		for(size_t yi=y;yi<yAvgSize+y;++yi)
		{
			if(!mask->Value(x, yi) && std::isfinite(real->Value(x,yi)) && std::isfinite(imaginary->Value(x,yi)))
			{
				r += real->Value(x, yi);
				i += imaginary->Value(x, yi);
			}
		}
		if(realVersusImaginary)
			pointSet.PushDataPoint(r, i);
		else if(drawImaginary)
			pointSet.PushDataPoint(x, i);
		else
			pointSet.PushDataPoint(x, r);
	}
}

void RFIPlots::MakeFittedComplexPlot(Plot2DPointSet &pointSet, const TimeFrequencyData &data, size_t xStart, size_t length, size_t y, size_t yAvgSize, Mask2DCPtr mask, num_t frequency, bool realVersusImaginary, bool drawImaginary)
{
	if(realVersusImaginary)
	{
		pointSet.SetXDesc("real");
		pointSet.SetYDesc("imaginary");
	} else {
		//plot.SetXRange(xStart, xStart+length-1);
		pointSet.SetXDesc("time");
		pointSet.SetYDesc("real/imaginary visibility");
	}
	Image2DCPtr real = data.GetRealPart();
	Image2DCPtr imaginary = data.GetImaginaryPart();

	std::vector<num_t> xReal(length);
	std::vector<num_t> xImag(length);
	std::vector<num_t> t(length);
	size_t dataIndex = 0;

	for(size_t x=xStart;x<xStart + length;++x)
	{
		num_t r = 0.0L, i = 0.0L;
		size_t count = 0;
		for(size_t yi=y;yi<yAvgSize+y;++yi)
		{
			if(!mask->Value(x, yi) && std::isfinite(real->Value(x,yi)) && std::isfinite(imaginary->Value(x,yi)) )
			{
				r += real->Value(x, yi);
				i += imaginary->Value(x, yi);
				++count;
			}
		}
		if(count > 0)
		{
			t[dataIndex] = x;
			xReal[dataIndex] = r;
			xImag[dataIndex] = i;
			++dataIndex;
		}
	}
	if(dataIndex != length)
		std::cout << "Warning: " << (length-dataIndex) << " time points were removed." << std::endl; 
	SinusFitter fitter;
	num_t
		realPhase, realAmplitude, realMean,
		imagPhase, imagAmplitude, imagMean;
	const num_t twopi = 2.0*M_PIn;

	fitter.FindPhaseAndAmplitudeComplex(realPhase, realAmplitude, xReal.data(), xImag.data(), t.data(), dataIndex, frequency*twopi);
	imagPhase = realPhase + 0.5*M_PIn;
	imagAmplitude = realAmplitude;
	realMean = fitter.FindMean(realPhase, realAmplitude, xReal.data(), t.data(), dataIndex, frequency*twopi);
	imagMean = fitter.FindMean(imagPhase, imagAmplitude, xImag.data(), t.data(), dataIndex, frequency*twopi);

	std::cout << "Amplitude found: " << realAmplitude << " phase found: " << realPhase << std::endl;

	for(size_t x=xStart;x<xStart + length;++x)
	{
		if(realVersusImaginary)
			pointSet.PushDataPoint(
				cosn(frequency*twopi*(long double) x + realPhase) * realAmplitude + realMean,
				cosn(frequency*twopi*(long double) x + imagPhase) * imagAmplitude + imagMean);
		else if(drawImaginary)
			pointSet.PushDataPoint(x,
				cosn(frequency*twopi*(long double) x + imagPhase) * imagAmplitude + imagMean);
		else
			pointSet.PushDataPoint(x,
				cosn(frequency*twopi*(long double) x + realPhase) * realAmplitude + realMean);
	}
}

void RFIPlots::MakeTimeScatterPlot(class MultiPlot &plot, size_t plotIndex, const Image2DCPtr& image, const Mask2DCPtr& mask, const TimeFrequencyMetaDataCPtr& metaData)
{
	plot.SetXAxisText("Time (s)");
	plot.SetYAxisText("Visibility");
	bool useMeta;
	if(metaData != 0 && metaData->HasObservationTimes())
		useMeta = true;
	else
		useMeta = false;
	double firstTimeStep;
	if(useMeta)
		firstTimeStep = metaData->ObservationTimes()[0];
	else
		firstTimeStep = 0;

	for(size_t x=0;x<image->Width();++x) {
		size_t count = 0;
		num_t sum = 0.0;
		for(size_t y=0;y<image->Height();++y) {
			if(!mask->Value(x, y) && std::isnormal(image->Value(x, y))) {
				sum += image->Value(x, y);
				++count;
			}
		}
		if(count > 0)
		{
			if(useMeta)
				plot.AddPoint(plotIndex, metaData->ObservationTimes()[x] - firstTimeStep, sum / count);
			else
				plot.AddPoint(plotIndex, x, sum / count);
		}
	}
}

void RFIPlots::MakeFrequencyScatterPlot(class MultiPlot &plot, size_t plotIndex, const Image2DCPtr& image, const Mask2DCPtr& mask, const TimeFrequencyMetaDataCPtr& metaData)
{
	plot.SetXAxisText("Frequency (MHz)");
	plot.SetYAxisText("Visibility");
	bool useMeta;
	if(metaData != 0 && metaData->HasBand())
		useMeta = true;
	else
		useMeta = false;

	for(size_t y=0;y<image->Height();++y) {
		size_t count = 0;
		num_t sum = 0.0;
		for(size_t x=0;x<image->Width();++x) {
			if(!mask->Value(x, y) && std::isnormal(image->Value(x, y))) {
				sum += image->Value(x, y);
				++count;
			}
		}
		if(count > 0)
		{
			if(useMeta)
				plot.AddPoint(plotIndex, metaData->Band().channels[y].frequencyHz*1e-6, sum / count);
			else
				plot.AddPoint(plotIndex, y, sum / count);
		}
	}
}

void RFIPlots::MakeTimeScatterPlot(class MultiPlot &plot, const TimeFrequencyData &data, const TimeFrequencyMetaDataCPtr& metaData, unsigned startIndex)
{
	for(size_t polIndex = 0; polIndex!=data.PolarizationCount(); ++polIndex)
	{
		PolarizationEnum pol = data.GetPolarization(polIndex);
		TimeFrequencyData polTF = data.Make(pol);
		MakeTimeScatterPlot(plot, startIndex+polIndex, polTF.GetSingleImage(), polTF.GetSingleMask(), metaData);
		if(data.PolarizationCount() == 1)
			plot.SetLegend(startIndex, data.Description());
		else
			plot.SetLegend(startIndex+polIndex, Polarization::TypeToFullString(pol));
	}
}

void RFIPlots::MakeFrequencyScatterPlot(class MultiPlot &plot, const TimeFrequencyData &data, const TimeFrequencyMetaDataCPtr& metaData, unsigned startIndex)
{
	for(size_t polIndex = 0; polIndex!=data.PolarizationCount(); ++polIndex)
	{
		PolarizationEnum pol = data.GetPolarization(polIndex);
		TimeFrequencyData polTF = data.Make(pol);
		MakeFrequencyScatterPlot(plot, startIndex+polIndex, polTF.GetSingleImage(), polTF.GetSingleMask(), metaData);
		if(data.PolarizationCount() == 1)
			plot.SetLegend(startIndex, data.Description());
		else
			plot.SetLegend(startIndex+polIndex, Polarization::TypeToFullString(pol));
	}
}

void RFIPlots::MakeRMSSpectrumPlot(Plot2DPointSet &pointSet, Image2DCPtr image, Mask2DCPtr mask)
{
	pointSet.SetXDesc("Channel");
	pointSet.SetYDesc("Visibility RMS");

	long double min = 1e100, max = 0.0;

	for(size_t y=0;y<image->Height();++y) {
		long double sum = 0.0L;
		size_t count = 0;
		for(size_t x=0;x<image->Width();++x) {
			if(!mask->Value(x, y) && std::isnormal(image->Value(x, y))) {
				sum += image->Value(x, y) * image->Value(x, y);
				++count;
			}
		}
		if(count > 0)
		{
			num_t v = sqrtn(sum/count);

			if(v < min) min = v;
			if(v > max) max = v;
			pointSet.PushDataPoint(y, v);
		}
	}
	pointSet.SetYRange(min, max);
}

void RFIPlots::MakeSNRSpectrumPlot(Plot2DPointSet &plot, Image2DCPtr image, Image2DCPtr model, Mask2DCPtr mask)
{
	plot.SetXDesc("Channel");
	plot.SetYDesc("Visibility RMS");

	long double min = 1e100, max = 0.0;

	for(size_t y=0;y<image->Height();++y) {
		num_t v = FrequencySNR(image, model, mask, y);

		if(v < min) min = v;
		if(v > max) max = v;
		plot.PushDataPoint(y, v);
	}
	plot.SetYRange(min, max);
}

num_t RFIPlots::FrequencySNR(Image2DCPtr image, Image2DCPtr model, Mask2DCPtr mask, unsigned channel)
{
	num_t sum = 0.0;
	size_t count = 0;
	for(unsigned x=0;x<image->Width();++x)
	{
		if(!mask->Value(x, channel))
		{
			num_t noise = fabsn(image->Value(x, channel) - model->Value(x, channel));
			num_t signal = fabsn(model->Value(x, channel));
			if(std::isfinite(signal) && std::isfinite(noise))
			{
				num_t snr = logn(signal / noise);
				sum += snr;
	
				++count;
			}
		}
	}
	return expn(sum / count);
}
