#include "histogrampagecontroller.h"

#include "../histogrampage.h"

#include "../../quality/histogramtablesformatter.h"
#include "../../quality/histogramcollection.h"
#include "../../quality/rayleighfitter.h"

#include "../../structures/measurementset.h"

#ifndef HAVE_EXP10
#define exp10(x) exp( (2.3025850929940456840179914546844) * (x) )
#endif

HistogramPageController::HistogramPageController() :
	_page(nullptr),
	_histograms(nullptr),
	_summedPolarizationHistograms(nullptr)
{
}

HistogramPageController::~HistogramPageController()
{
	CloseStatistics();
}

void HistogramPageController::readFromFile()
{
	CloseStatistics();
	HistogramTablesFormatter histogramTables(_statFilename);
	if(histogramTables.HistogramsExist())
	{
		MeasurementSet set(_statFilename);
		
		const unsigned polarizationCount = set.PolarizationCount();

		_histograms = new HistogramCollection(polarizationCount);
		_histograms->Load(histogramTables);
	}
}

void HistogramPageController::CloseStatistics()
{
	_statFilename = std::string();
	if(_histograms != 0)
	{
		delete _histograms;
		_histograms = 0;
	}
	if(_summedPolarizationHistograms != 0)
	{
		delete _summedPolarizationHistograms;
		_summedPolarizationHistograms = 0;
	}
}

void HistogramPageController::SetHistograms(const HistogramCollection *histograms)
{
	CloseStatistics();
	_histograms = new HistogramCollection(*histograms);
	_summedPolarizationHistograms = _histograms->CreateSummedPolarizationCollection();
	_histograms->CreateMissingBins();
	_summedPolarizationHistograms->CreateMissingBins();
	updatePlot();
}

void HistogramPageController::updatePlot()
{
	if(HasStatistics())
	{
		_plot.Clear();
		
		const unsigned polarizationCount = _histograms->PolarizationCount();
		if(_drawXX)
			plotPolarization(*_histograms, 0);
		if(_drawXY && polarizationCount>=2)
			plotPolarization(*_histograms, 1);
		if(_drawYX && polarizationCount>=3)
			plotPolarization(*_histograms, 2);
		if(_drawYY && polarizationCount>=4)
			plotPolarization(*_histograms, 3);
		if(_drawSum)
			plotPolarization(*_summedPolarizationHistograms, 0);
		
		if(_page != nullptr)
			_page->Redraw();
	}
}

void HistogramPageController::plotPolarization(const HistogramCollection &histogramCollection, unsigned polarization)
{
	LogHistogram totalHistogram, rfiHistogram;
	histogramCollection.GetTotalHistogramForCrossCorrelations(polarization, totalHistogram);
	histogramCollection.GetRFIHistogramForCrossCorrelations(polarization, rfiHistogram);
	plotPolarization(totalHistogram, rfiHistogram);
}

std::string HistogramPageController::SlopeText(const LogHistogram &histogram)
{
	if(_automaticSlopeRange)
	{
		histogram.GetRFIRegion(_slopeStart, _slopeEnd);
	}
	std::stringstream str;
	const double
		slope = histogram.NormalizedSlope(_slopeStart, _slopeEnd),
		powerLawExp = histogram.PowerLawExponent(_slopeStart),
		powerLawExpError = histogram.PowerLawExponentStdError(_slopeStart, powerLawExp),
		offset = histogram.NormalizedSlopeOffset(_slopeStart, _slopeEnd, slope),
		error = histogram.NormalizedSlopeStdError(_slopeStart, _slopeEnd, slope),
		errorB = histogram.NormalizedSlopeStdDevBySampling(_slopeStart, _slopeEnd, slope, _deltaS),
		upperLimit = histogram.PowerLawUpperLimit(_slopeStart, slope, exp10(offset)),
		lowerLimit = histogram.PowerLawLowerLimit(_slopeStart, slope, exp10(offset), _slopeRFIRatio),
		lowerError = fabs(lowerLimit - histogram.PowerLawLowerLimit(_slopeStart, slope - error, exp10(offset), _slopeRFIRatio)),
		lowerLimit2 = histogram.PowerLawLowerLimit2(_slopeStart, slope, exp10(offset), _slopeRFIRatio);
	str << slope << "±" << error << "\n/±" << errorB << "\nb=" << exp10(offset)
		<< "\nPL:"
		<< powerLawExp << "±" << powerLawExpError << "\n["
		<< log10(lowerLimit) << "±" << lowerError << ';' << log10(upperLimit) << ']' << '\n'
		<< log10(lowerLimit2);
	return str.str();
}

void HistogramPageController::plotSlope(const LogHistogram &histogram, const std::string &title, bool useLowerLimit2)
{
	double start, end;
	if(_automaticSlopeRange)
	{
		histogram.GetRFIRegion(start, end);
	} else {
		start = _slopeStart;
		end = _slopeEnd;
	}
	double
		xMin = log10(histogram.MinPositiveAmplitude()),
		rfiRatio = _slopeRFIRatio,
		slope = histogram.NormalizedSlope(start, end),
		offset = histogram.NormalizedSlopeOffset(start, end, slope),
		upperLimit = log10(histogram.PowerLawUpperLimit(start, slope, exp10(offset))),
		lowerLimit = useLowerLimit2 ?
			log10(histogram.PowerLawLowerLimit2(start, slope, exp10(offset), rfiRatio)) :
			log10(histogram.PowerLawLowerLimit(start, slope, exp10(offset), rfiRatio));
	double xStart, xEnd;
	if(std::isfinite(lowerLimit))
		xStart = lowerLimit;
	else
		xStart = log10(start) - 1.0;
	if(std::isfinite(upperLimit))
		xEnd = upperLimit;
	else
		xEnd = log10(histogram.MaxAmplitude());
	double
		yStart = xStart*slope + offset,
		yEnd = xEnd*slope + offset;
	_plot.StartLine(title, "Amplitude in arbitrary units (log)", "Frequency (log)");
	if(useLowerLimit2 && std::isfinite(xMin))
		_plot.PushDataPoint(xMin, yStart);
	_plot.PushDataPoint(xStart, yStart);
	_plot.PushDataPoint(xEnd, yEnd);
}

void HistogramPageController::plotPolarization(const LogHistogram &totalHistogram, const LogHistogram &rfiHistogram)
{
	if(_totalHistogram)
	{
		_plot.StartLine("Total histogram", "Amplitude in arbitrary units (log)", "Frequency (log)");
		addHistogramToPlot(totalHistogram);
		
		if(_fit || _subtractFit)
		{
			plotFit(totalHistogram, "Fit to total");
		}
		if(_drawSlope)
		{
			plotSlope(totalHistogram, "Fitted slope", false);
		}
		if(_drawSlope2)
		{
			plotSlope(totalHistogram, "Fitted slope", true);
		}
		std::string str = SlopeText(totalHistogram);
		_page->SetSlopeFrame(str);
	}

	if(_rfiHistogram)
	{
		_plot.StartLine("RFI histogram", "Amplitude in arbitrary units (log)", "Frequency (log)");
		addHistogramToPlot(rfiHistogram);

		if(_fit || _subtractFit)
		{
			plotFit(rfiHistogram, "Fit to RFI");
		}
		std::string str = SlopeText(rfiHistogram);
		_page->SetSlopeFrame(str);
		if(_drawSlope)
		{
			plotSlope(rfiHistogram, "Fitted slope", false);
		}
		if(_drawSlope2)
		{
			plotSlope(rfiHistogram, "Fitted slope", true);
		}
	}
	
	if(_notRFIHistogram)
	{
		_plot.StartLine("Non-RFI histogram", "Amplitude in arbitrary units (log)", "Frequency (log)");
		LogHistogram histogram(totalHistogram);
		histogram -= rfiHistogram;
		addHistogramToPlot(histogram);

		if(_fit || _subtractFit)
		{
			plotFit(histogram, "Fit to Non-RFI");
		}
	}
}

void HistogramPageController::plotFit(const LogHistogram &histogram, const std::string &title)
{
	double sigmaEstimate;
	sigmaEstimate = RayleighFitter::SigmaEstimate(histogram);
	if(_automaticFitRange)
	{
		RayleighFitter::FindFitRangeUnderRFIContamination(histogram.MinPositiveAmplitude(), sigmaEstimate, _fitStart, _fitEnd);
	}
	double sigma = RayleighFitter::SigmaEstimate(histogram, _fitStart, _fitEnd), n = RayleighFitter::NEstimate(histogram, _fitStart, _fitEnd);
	RayleighFitter fitter;
	fitter.SetFitLogarithmic(_fitLogarithmic);
	fitter.Fit(_fitStart, _fitEnd, histogram, sigma, n);
	if(_fit)
	{
		_plot.StartLine(title, "Amplitude in arbitrary units (log)", "Frequency (log)");
		addRayleighToPlot(histogram, sigma, n);
	}
	if(_subtractFit)
	{
		_plot.StartLine(title, "Amplitude in arbitrary units (log)", "Frequency (log)");
		addRayleighDifferenceToPlot(histogram, sigma, n);
	}

	std::stringstream str;
	str << "σ=1e" << log10(sigma) << ",n=1e" << log10(n) << '\n'
		<< "n_t=1e" << log10(histogram.NormalizedTotalCount()) << '\n'
		<< "mode=1e" << log10(histogram.AmplitudeWithMaxNormalizedCount()) << '\n'
		<< "ε_R=" << RayleighFitter::ErrorOfFit(histogram, _fitStart, _fitEnd, sigma, n);
	_page->SetFitText(str.str());
}

void HistogramPageController::addRayleighToPlot(const LogHistogram &histogram, double sigma, double n)
{
	const bool derivative = _derivative;
	double x = histogram.MinPositiveAmplitude();
	const double xend = sigma*5.0;
	const double sigmaP2 = sigma*sigma;
	while(x < xend) {
		const double logx = log10(x);
		if(derivative)
		{
			const double dc = -(exp10(2.0*x)-sigmaP2)/sigmaP2;
			if(std::isfinite(logx) && std::isfinite(dc))
				_plot.PushDataPoint(logx, dc);
		} else {
			const double c = n * x / (sigmaP2) * exp(-x*x/(2*sigmaP2));
			const double logc = log10(c);
			if(std::isfinite(logx) && std::isfinite(logc))
				_plot.PushDataPoint(logx, logc);
		}
		x *= 1.05;
	}
}

void HistogramPageController::addRayleighDifferenceToPlot(const LogHistogram &histogram, double sigma, double n)
{
	const double sigmaP2 = sigma*sigma;
	double minCount = histogram.MinPosNormalizedCount();
	for(LogHistogram::iterator i=histogram.begin();i!=histogram.end();++i)
	{
		const double x = i.value();
		
    const double c = n * x / (sigmaP2) * exp(-x*x/(2*sigmaP2));
		double diff = fabs(i.normalizedCount() - c);
		if(diff >= minCount)
		{
			const double logx = log10(x);
			const double logc = log10(diff);
			if(std::isfinite(logx) && std::isfinite(logc))
				_plot.PushDataPoint(logx, logc);
		}
	}
}

void HistogramPageController::addHistogramToPlot(const LogHistogram &histogram)
{
	const bool derivative = _derivative;
	const bool staircase = _staircaseFunction;
	const bool normalize = _normalize;
	double deltaS = _deltaS;
	if(deltaS <= 1.0001) deltaS = 1.0001;
	for(LogHistogram::iterator i=histogram.begin();i!=histogram.end();++i)
	{
		double x = i.value(), logxStart, logxEnd;
		if(staircase)
		{
			logxStart = log10(i.binStart());
			logxEnd = log10(i.binEnd());
		}
		else {
			logxStart = log10(x);
		}
		if(derivative)
		{
			const double cslope = histogram.NormalizedSlope(x/deltaS, x*deltaS);
			//if(std::isfinite(logxStart) && std::isfinite(cslope))
				_plot.PushDataPoint(logxStart, cslope);
			if(staircase)// && std::isfinite(logxEnd) && std::isfinite(cslope))
				_plot.PushDataPoint(logxEnd, cslope);
		} else {
			const double logc = log10(normalize ? i.normalizedCount() : i.unnormalizedCount());
			//if(std::isfinite(logxStart) && std::isfinite(logc))
				_plot.PushDataPoint(logxStart, logc);
			if(staircase)// && std::isfinite(logxEnd) && std::isfinite(logc))
				_plot.PushDataPoint(logxEnd, logc);
		}
	}
}

