#ifndef ANTENNA_SELECTOR_H
#define ANTENNA_SELECTOR_H

class StatisticsCollection;
class DefaultStatistics;

#include <map>
#include <vector>

namespace algorithms {

class AntennaSelector {
 public:
  AntennaSelector() : _threshold(5.0) {}

  std::vector<std::size_t> Run(
      const StatisticsCollection& statisticsCollection);

 private:
  void addStatistic(unsigned antIndex, const DefaultStatistics& stats,
                    std::map<std::size_t, DefaultStatistics>& antStatistics);

  double _threshold;
};

}  // namespace algorithms

#endif
