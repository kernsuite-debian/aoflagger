#ifndef BASELINE_PAGE_CONTROLLER_H
#define BASELINE_PAGE_CONTROLLER_H

#include "heatmappagecontroller.h"

class BaselinePageController : public HeatMapPageController
{
public:
	BaselinePageController()	:
		_statCollection(nullptr),
		_antennas(nullptr)
	{ }
	
	virtual void SetStatistics(const StatisticsCollection* statCollection, const std::vector<class AntennaInfo>& antennas) override final
	{
		_statCollection = statCollection;
		_antennas = &antennas;
		UpdateImage();
	}
	virtual void CloseStatistics() override final
	{
		_statCollection = 0;
		_antennas = 0;
	}
	bool HasStatistics() const
	{
		return _statCollection != 0;
	}
	std::string AntennaName(size_t index) const;
protected:
	virtual std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> constructImage(QualityTablesFormatter::StatisticKind kind) override final;
private:
	const StatisticsCollection *_statCollection;
	const std::vector<class AntennaInfo> *_antennas;
};

#endif
