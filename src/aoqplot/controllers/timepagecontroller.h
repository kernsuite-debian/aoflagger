#ifndef TIME_PAGE_CONTROLLER_H
#define TIME_PAGE_CONTROLLER_H

#include "aoqplotpagecontroller.h"

#include "../../quality/statisticscollection.h"

class TimePageController : public AOQPlotPageController
{
protected:
	virtual const std::map<double, class DefaultStatistics> &getStatistics() const override final
	{
		return getStatCollection()->TimeStatistics();
	}
	
	virtual void startLine(Plot2D &plot, const std::string &name, int lineIndex, const std::string &yAxisDesc) override final
	{
		plot.StartLine(name, "Time", yAxisDesc, true);
	}
};

#endif


