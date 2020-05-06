#ifndef ANTENNAE_PAGE_CONTROLLER_H
#define ANTENNAE_PAGE_CONTROLLER_H

#include "../../quality/statisticscollection.h"

#include "../../structures/msmetadata.h"

#include "aoqplotpagecontroller.h"

class AntennaePageController : public AOQPlotPageController
{
protected:
	virtual void processStatistics(const StatisticsCollection *statCollection, const std::vector<AntennaInfo> &antennas) override final
	{
		_antennas = antennas;
		const BaselineStatisticsMap &map = statCollection->BaselineStatistics();
		
		std::vector<std::pair<unsigned, unsigned> > baselines = map.BaselineList();
		for(std::vector<std::pair<unsigned, unsigned> >::const_iterator i=baselines.begin();i!=baselines.end();++i)
		{
			if(i->first != i->second)
			{
				const DefaultStatistics& stats = map.GetStatistics(i->first, i->second);
				addStatistic(i->first, stats);
				addStatistic(i->second, stats);
			}
		}
	}
	
	virtual const std::map<double, class DefaultStatistics> &getStatistics() const override final
	{
		return _statistics;
	}
	
	virtual void startLine(Plot2D& plot, const std::string &name, int lineIndex, const std::string &yAxisDesc) override final
	{
		Plot2DPointSet& pointSet = plot.StartLine(name, "Antenna index", yAxisDesc, false, Plot2DPointSet::DrawColumns);
		
		std::vector<std::string> labels;
		for(std::vector<AntennaInfo>::const_iterator i=_antennas.begin();i!=_antennas.end();++i)
			labels.push_back(i->name);
		pointSet.SetTickLabels(labels);
		pointSet.SetRotateUnits(true);
	}
	
	void addStatistic(unsigned antIndex, const DefaultStatistics& stats)
	{
		std::map<double, DefaultStatistics>::iterator iter = _statistics.find(antIndex);
		if(iter == _statistics.end())
			_statistics.insert(std::pair<double, DefaultStatistics>(antIndex, stats));
		else
			iter->second += stats;
	}
	
private:
	std::map<double, DefaultStatistics> _statistics;
	std::vector<AntennaInfo> _antennas;
};

#endif

