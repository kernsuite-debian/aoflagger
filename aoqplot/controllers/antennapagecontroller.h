#ifndef ANTENNAE_PAGE_CONTROLLER_H
#define ANTENNAE_PAGE_CONTROLLER_H

#include "aoqplotpagecontroller.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../../quality/statisticscollection.h"

#include "../../structures/msmetadata.h"

class AntennaePageController final : public AOQPlotPageController {
 protected:
  void processStatistics(const StatisticsCollection* statCollection,
                         const std::vector<AntennaInfo>& antennas) override {
    _antennas = antennas;
    const BaselineStatisticsMap& map = statCollection->BaselineStatistics();

    std::vector<std::pair<unsigned, unsigned>> baselines = map.BaselineList();
    for (std::vector<std::pair<unsigned, unsigned>>::const_iterator i =
             baselines.begin();
         i != baselines.end(); ++i) {
      if (i->first != i->second) {
        const DefaultStatistics& stats = map.GetStatistics(i->first, i->second);
        addStatistic(i->first, stats);
        addStatistic(i->second, stats);
      }
    }
  }

  const std::map<double, class DefaultStatistics>& getStatistics()
      const override {
    return _statistics;
  }

  void startLine(XYPlot& plot, const std::string& name, int lineIndex,
                 const std::string& yAxisDesc, bool second_axis) override {
    XYPointSet& points = plot.StartLine(name, "Antenna index", yAxisDesc,
                                        XYPointSet::DrawColumns);
    points.SetUseSecondYAxis(second_axis);

    std::vector<std::pair<double, std::string>> labels;
    for (size_t i = 0; i != _antennas.size(); ++i)
      labels.emplace_back(static_cast<double>(i), _antennas[i].name);
    plot.XAxis().SetTickLabels(std::move(labels));
    plot.XAxis().SetRotateUnits(true);
  }

  void addStatistic(unsigned antIndex, const DefaultStatistics& stats) {
    std::map<double, DefaultStatistics>::iterator iter =
        _statistics.find(antIndex);
    if (iter == _statistics.end())
      _statistics.insert(std::pair<double, DefaultStatistics>(antIndex, stats));
    else
      iter->second += stats;
  }

 private:
  std::map<double, DefaultStatistics> _statistics;
  std::vector<AntennaInfo> _antennas;
};

#endif
