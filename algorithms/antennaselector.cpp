#include "antennaselector.h"

#include <utility>

#include "../quality/statisticscollection.h"
#include "../quality/statisticsderivator.h"

using std::size_t;

namespace algorithms {

std::vector<size_t> AntennaSelector::Run(
    const StatisticsCollection& statisticsCollection) {
  const std::map<size_t, DefaultStatistics> antStatistics =
      statisticsCollection.GetAntennaStatistics();

  std::vector<double> stddevs;
  std::set<size_t> badAntennas;
  for (size_t p = 0; p != 4; ++p) {
    double meanStddev = 0.0;
    stddevs.clear();
    for (const std::pair<const size_t, DefaultStatistics>& antenna :
         antStatistics) {
      const double stddev = StatisticsDerivator::GetStatisticAmplitude(
          QualityTablesFormatter::StandardDeviationStatistic, antenna.second,
          p);
      stddevs.emplace_back(stddev);
      meanStddev += stddev;
    }
    double stddevOfStddev = 0.0;
    meanStddev /= stddevs.size();
    for (const double& s : stddevs) {
      stddevOfStddev += (s - meanStddev) * (s - meanStddev);
    }
    stddevOfStddev = sqrt(stddevOfStddev / stddevs.size());

    size_t index = 0;
    const double limit = _threshold * stddevOfStddev;
    for (const std::pair<const size_t, DefaultStatistics>& antenna :
         antStatistics) {
      if (std::fabs(stddevs[index] - meanStddev) > limit ||
          stddevs[index] == 0.0) {
        if (antenna.second.count[p] != 0) badAntennas.insert(antenna.first);
      }
      ++index;
    }
  }
  return std::vector<size_t>(badAntennas.begin(), badAntennas.end());
}

}  // namespace algorithms
