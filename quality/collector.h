#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <complex>
#include <set>
#include <string>
#include <vector>
#include <thread>

#include <aocommon/lane.h>
#include <aocommon/uvector.h>

class Collector {
 public:
  enum CollectingMode {
    CollectDefault,
    CollectHistograms,
    CollectTimeFrequency
  };

  Collector()
      : _mode(CollectDefault),
        _dataColumnName("DATA"),
        _intervalStart(0),
        _intervalEnd(0),
        _flaggedTimesteps(0) {}

  void Collect(const std::string& filename,
               class StatisticsCollection& statisticsCollection,
               class HistogramCollection& histogramCollection,
               class ProgressListener& progressListener);

  void SetMode(CollectingMode mode) { _mode = mode; }
  void SetDataColumn(const std::string& dataColumnName) {
    _dataColumnName = dataColumnName;
  }
  void SetFlaggedTimesteps(size_t flaggedTimesteps) {
    _flaggedTimesteps = flaggedTimesteps;
  }
  void SetInterval(size_t start, size_t end) {
    _intervalStart = start;
    _intervalEnd = end;
  }
  void SetFlaggedAntennae(std::set<size_t>&& flaggedAntennae) {
    _flaggedAntennae = flaggedAntennae;
  }

 private:
  struct Work {
    std::vector<aocommon::UVector<std::complex<float>>> samples;
    std::vector<aocommon::UVector<bool>> isRFI;
    size_t antenna1Index, antenna2Index, bandIndex, timestepIndex, nChan;
    double time;
    bool hasFlaggedAntenna;
  };

  void process(aocommon::Lane<Work>* workLane, StatisticsCollection* stats,
               size_t polarizationCount);

  std::mutex _mutex;
  CollectingMode _mode;
  std::string _dataColumnName;
  size_t _intervalStart, _intervalEnd;
  size_t _flaggedTimesteps;
  std::set<size_t> _flaggedAntennae;
  aocommon::UVector<bool> _correlatorFlags;
  aocommon::UVector<bool> _correlatorFlagsForBadAntenna;
  StatisticsCollection* _statisticsCollection;
  HistogramCollection* _histogramCollection;
  std::vector<StatisticsCollection> _threadStats;
};

#endif
