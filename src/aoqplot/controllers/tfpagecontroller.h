#ifndef TF_PAGE_CONTROLLER_H
#define TF_PAGE_CONTROLLER_H

#include "heatmappagecontroller.h"

class TFPageController : public HeatMapPageController
{
public:
	TFPageController();
	
	virtual void SetStatistics(const StatisticsCollection *statCollection, const std::vector<class AntennaInfo>&) override final
	{
		_statCollection = statCollection;
		UpdateImage();
	}
	virtual void CloseStatistics() override final
	{
		_statCollection = 0;
	}
	bool HasStatistics() const
	{
		return _statCollection != 0;
	}
	
protected:
	virtual std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> constructImage(QualityTablesFormatter::StatisticKind kind) final override;
	
private:
	const StatisticsCollection *_statCollection;
};

#endif

