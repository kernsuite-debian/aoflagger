#ifndef AOQPLOT_CONTROLLER_H
#define AOQPLOT_CONTROLLER_H

#include "../../structures/antennainfo.h"

#include "../../quality/qualitytablesformatter.h"

#include <string>
#include <vector>

class AOQPlotController
{
public:
	AOQPlotController();
	~AOQPlotController();
	
	void Attach(class AOQPlotWindow* window) { _window = window; }
	
	void ReadStatistics(const std::vector<std::string>& files)
	{
		ReadStatistics(files, true, true, 1000, 1000, false);
	}
	
	void ReadStatistics(const std::vector<std::string>& files, bool downsampleTime, bool downsampleFreq, size_t timeSize, size_t freqSize, bool correctHistograms);
	
	struct PlotSavingData
	{
		QualityTablesFormatter::StatisticKind statisticKind;
		std::string filenamePrefix;
	};
	
	void Save(const PlotSavingData& data, size_t width, size_t height);
	
	void Initialize(class AOQPageController* controller, bool averagedStats);
private:
	void close();
	void readDistributedObservation(const std::string& filename, bool correctHistograms);
	void readMetaInfoFromMS(const std::string& filename);
	void readAndCombine(const std::string& filename);
	
	bool _isOpen;
	std::unique_ptr<class StatisticsCollection> _statCollection;
	std::unique_ptr<class HistogramCollection> _histCollection;
	std::unique_ptr<class StatisticsCollection> _fullStats;
	std::vector<class AntennaInfo> _antennas;
	size_t _polarizationCount;
	class AOQPlotWindow* _window;
};

#endif
