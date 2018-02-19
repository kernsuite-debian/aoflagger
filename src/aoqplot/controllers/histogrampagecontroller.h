#ifndef HISTOGRAM_PAGE_CONTROLLER_H
#define HISTOGRAM_PAGE_CONTROLLER_H

#include "aoqplotpagecontroller.h"

#include "../../plot/plot2d.h"

class HistogramPageController : public AOQPageController
{
public:
	HistogramPageController();
	~HistogramPageController();
	
	void Attach(class HistogramPage* page) { _page = page; }
	
	virtual void SetHistograms(const HistogramCollection *histograms) override final;
	
	void SetHistograms(const std::string &filename)
	{
		_statFilename = filename;
		readFromFile();
		updatePlot();
	}
	
	virtual void CloseStatistics() override final;
	
	bool HasStatistics() const
	{
		return _histograms != 0;
	}
	
	Plot2D& Plot() { return _plot; }
	
	std::string SlopeText(const class LogHistogram &histogram);
	
	void SetAutomaticFitRange(bool automaticFitRange) { _automaticFitRange = automaticFitRange; }
	void SetFitStart(double fitStart) { _fitStart = fitStart; }
	void SetFitEnd(double fitEnd) { _fitEnd = fitEnd; }
	void SetFitLogarithmic(bool fitLog) { _fitLogarithmic = fitLog; }
	
	double FitStart() { return _fitStart; }
	double FitEnd() { return _fitEnd; }
	
	void SetAutomaticSlopeRange(bool automaticSlopeRange) { _automaticSlopeRange = automaticSlopeRange; }
	void SetSlopeStart(double start) { _slopeStart = start; }
	void SetSlopeEnd(double end) { _slopeEnd = end; }
	void SetSlopeRFIRatio(double rfiRatio) { _slopeRFIRatio = rfiRatio; }
	
	double SlopeStart() { return _slopeStart; }
	double SlopeEnd() { return _slopeEnd; }
	
	void SetDrawTotal(bool drawTotal) { _totalHistogram = drawTotal; }
	void SetDrawFit(bool drawFit) { _fit = drawFit; }
	void SetDrawSubtractedFit(bool drawSubtrFit) { _subtractFit = drawSubtrFit; }
	void SetDrawSlope(bool drawSlope) { _drawSlope = drawSlope; }
	void SetDrawSlope2(bool drawSlope2) { _drawSlope2 = drawSlope2; }
	void SetDrawRFI(bool drawRFI) { _rfiHistogram = drawRFI; }
	void SetDrawNonRFI(bool drawNonRFI) { _notRFIHistogram = drawNonRFI; }
	
	void SetDerivative(bool derivative) { _derivative = derivative; }
	void SetStaircase(bool staircaseFunction) { _staircaseFunction = staircaseFunction; }
	void SetNormalize(bool normalize) { _normalize = normalize; }
	void SetDeltaS(double deltaS) { _deltaS = deltaS; }
	
	void SetDrawXX(bool drawXX) { _drawXX = drawXX; }
	void SetDrawXY(bool drawXY) { _drawXY = drawXY; }
	void SetDrawYX(bool drawYX) { _drawYX = drawYX; }
	void SetDrawYY(bool drawYY) { _drawYY = drawYY; }
	void SetDrawSum(bool drawSum) { _drawSum = drawSum; }
private:
	void updatePlot();
	void addHistogramToPlot(const class LogHistogram &histogram);
	void addRayleighToPlot(const class LogHistogram &histogram, double sigma, double n);
	void addRayleighDifferenceToPlot(const LogHistogram &histogram, double sigma, double n);
	void plotPolarization(const HistogramCollection &histogramCollection, unsigned polarization);
	void plotPolarization(const class LogHistogram &totalHistogram, const class LogHistogram &rfiHistogram);
	void plotFit(const class LogHistogram &histogram, const std::string &title);
	void plotSlope(const class LogHistogram &histogram, const std::string &title, bool useLowerLimit2);
	void readFromFile();
	
	class HistogramPage* _page;
	std::string _statFilename;
	Plot2D _plot;
	class HistogramCollection *_histograms;
	class HistogramCollection *_summedPolarizationHistograms;
	
	bool _drawXX, _drawXY, _drawYX, _drawYY, _drawSum;
	
	bool _automaticFitRange;
	double _fitStart, _fitEnd;
	bool _fitLogarithmic, _automaticSlopeRange;
	double _slopeStart, _slopeEnd, _slopeRFIRatio;
	bool _totalHistogram, _fit, _subtractFit, _drawSlope, _drawSlope2, _rfiHistogram, _notRFIHistogram;
	bool _derivative, _staircaseFunction, _normalize;
	double _deltaS;
};

#endif

