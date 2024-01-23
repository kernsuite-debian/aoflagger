#ifndef TIME_PAGE_CONTROLLER_H
#define TIME_PAGE_CONTROLLER_H

#include <map>
#include <string>

#include "aoqplotpagecontroller.h"

#include "../../quality/statisticscollection.h"

class TimePageController final : public AOQPlotPageController {
 protected:
  const std::map<double, DefaultStatistics>& getStatistics() const override {
    if (getStatCollection()->AllTimeStatistics().empty()) {
      static std::map<double, DefaultStatistics> empty_set;
      return empty_set;
    } else {
      return getStatCollection()->TimeStatistics();
    }
  }

  void startLine(XYPlot& plot, const std::string& name, int lineIndex,
                 const std::string& yAxisDesc, bool second_axis) override {
    XYPointSet& points = plot.StartLine(name, "Time", yAxisDesc);
    points.SetUseSecondYAxis(second_axis);
  }
};

#endif
