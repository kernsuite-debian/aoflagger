#ifndef BLENGTH_PATH_CONTROLLER_H
#define BLENGTH_PATH_CONTROLLER_H

#include "aoqplotpagecontroller.h"

#include "../../quality/baselinestatisticsmap.h"
#include "../../quality/statisticscollection.h"

class BLengthPageController : public AOQPlotPageController
{
public:
	void SetIncludeAutoCorrelations(bool inclAutos) {
		_includeAutoCorrelations = inclAutos;
	}
protected:
	virtual void processStatistics(const StatisticsCollection *statCollection, const std::vector<AntennaInfo> &antennas) override final
	{
		_statisticsWithAutocorrelations.clear();
		_statisticsWithoutAutocorrelations.clear();
		
		const BaselineStatisticsMap& map = statCollection->BaselineStatistics();
		
		vector<std::pair<unsigned, unsigned> > baselines = map.BaselineList();
		for(vector<std::pair<unsigned, unsigned> >::const_iterator i=baselines.begin();i!=baselines.end();++i)
		{
			Baseline bline(antennas[i->first], antennas[i->second]);
			const DefaultStatistics &statistics = map.GetStatistics(i->first, i->second);
			_statisticsWithAutocorrelations.insert(std::pair<double, DefaultStatistics>(bline.Distance(), statistics));
			if(i->first != i->second)
				_statisticsWithoutAutocorrelations.insert(std::pair<double, DefaultStatistics>(bline.Distance(), statistics));
		}
	}
	
	virtual const std::map<double, class DefaultStatistics> &getStatistics() const override final
	{
		return _includeAutoCorrelations ? _statisticsWithAutocorrelations : _statisticsWithoutAutocorrelations;
	}
	
	virtual void startLine(Plot2D &plot, const std::string &name, int lineIndex, const std::string &yAxisDesc) override final
	{
		plot.StartLine(name, "Baseline length (m)", yAxisDesc, false, Plot2DPointSet::DrawPoints);
	}
	
private:
	bool _includeAutoCorrelations = false;
	std::map<double, DefaultStatistics> _statisticsWithAutocorrelations;
	std::map<double, DefaultStatistics> _statisticsWithoutAutocorrelations;
};

#endif

