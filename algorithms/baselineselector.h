#ifndef BASELINE_SELECTOR_H
#define BASELINE_SELECTOR_H

#include <string>
#include <set>
#include <vector>
#include <mutex>

#include "../structures/timefrequencymetadata.h"
#include "../structures/mask2d.h"

class DefaultStatistics;

namespace algorithms {

class BaselineSelector {
 public:
  struct SingleBaselineInfo {
    SingleBaselineInfo() : marked(false) {}

    bool operator<(const SingleBaselineInfo& rhs) const {
      return length < rhs.length;
    }

    int antenna1, antenna2;
    std::string antenna1Name, antenna2Name;
    int band;
    unsigned sequenceId;
    double length;
    unsigned long rfiCount, totalCount;
    bool marked;
  };

  BaselineSelector()
      : _threshold(8.0),
        _absThreshold(0.4),
        _smoothingSigma(0.6),
        _makePlot(false),
        _useLog(true) {}

  typedef std::vector<SingleBaselineInfo> BaselineVector;
  void Search(
      std::vector<BaselineSelector::SingleBaselineInfo>& markedBaselines);
  void ImplyStations(
      const std::vector<BaselineSelector::SingleBaselineInfo>& markedBaselines,
      double maxRatio, std::set<unsigned>& badStations) const;
  void Add(Mask2DCPtr mask, TimeFrequencyMetaDataCPtr metaData);
  void Add(class DefaultStatistics& baselineStat, class AntennaInfo& antenna1,
           class AntennaInfo& antenna2);

  std::mutex& Mutex() { return _mutex; }

  double Threshold() const { return _threshold; }
  double AbsThreshold() const { return _absThreshold; }

  void SetThreshold(double threshold) { _threshold = threshold; }
  void SetAbsThreshold(double absThreshold) { _absThreshold = absThreshold; }
  void SetSmoothingSigma(double smoothingSigma) {
    _smoothingSigma = smoothingSigma;
  }
  void SetUseLog(bool useLog) { _useLog = useLog; }

  size_t BaselineCount() const { return _baselines.size(); }

 private:
  std::mutex _mutex;
  BaselineVector _baselines;
  double _threshold, _absThreshold;
  double _smoothingSigma;
  bool _makePlot;
  bool _useLog;

  double smoothedValue(double length) const;
  double smoothedValue(
      const BaselineSelector::SingleBaselineInfo& baseline) const {
    return smoothedValue(baseline.length);
  }
};

}  // namespace algorithms

#endif
