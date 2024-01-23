#ifndef BLENGTH_PATH_CONTROLLER_H
#define BLENGTH_PATH_CONTROLLER_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "aoqplotpagecontroller.h"

#include "../../quality/baselinestatisticsmap.h"
#include "../../quality/statisticscollection.h"

class BLengthPageController final : public AOQPlotPageController {
 public:
  void SetIncludeAutoCorrelations(bool inclAutos) {
    _includeAutoCorrelations = inclAutos;
  }

 protected:
  void processStatistics(const StatisticsCollection* statCollection,
                         const std::vector<AntennaInfo>& antennas) override {
    _statisticsWithAutocorrelations.clear();
    _statisticsWithoutAutocorrelations.clear();

    const BaselineStatisticsMap& map = statCollection->BaselineStatistics();

    std::vector<std::pair<unsigned, unsigned>> baselines = map.BaselineList();
    for (std::vector<std::pair<unsigned, unsigned>>::const_iterator i =
             baselines.begin();
         i != baselines.end(); ++i) {
      Baseline bline(antennas[i->first], antennas[i->second]);
      const DefaultStatistics& statistics =
          map.GetStatistics(i->first, i->second);
      _statisticsWithAutocorrelations.insert(
          std::pair<double, DefaultStatistics>(bline.Distance(), statistics));
      if (i->first != i->second)
        _statisticsWithoutAutocorrelations.insert(
            std::pair<double, DefaultStatistics>(bline.Distance(), statistics));
    }
  }

  const std::map<double, class DefaultStatistics>& getStatistics()
      const override {
    return _includeAutoCorrelations ? _statisticsWithAutocorrelations
                                    : _statisticsWithoutAutocorrelations;
  }

  void startLine(XYPlot& plot, const std::string& name, int lineIndex,
                 const std::string& yAxisDesc, bool second_axis) override {
    XYPointSet& points = plot.StartLine(name, "Baseline length (m)", yAxisDesc,
                                        XYPointSet::DrawPoints);
    points.SetUseSecondYAxis(second_axis);
  }

 private:
  bool _includeAutoCorrelations = false;
  std::map<double, DefaultStatistics> _statisticsWithAutocorrelations;
  std::map<double, DefaultStatistics> _statisticsWithoutAutocorrelations;
};

#endif
