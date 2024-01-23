#ifndef AOFLAGGER_STRUCTURES_H
#define AOFLAGGER_STRUCTURES_H

#include "../structures/image2d.h"
#include "../structures/mask2d.h"
#include "../structures/timefrequencymetadata.h"

#include "../quality/histogramcollection.h"
#include "../quality/statisticscollection.h"

#include <memory>
#include <vector>

namespace aoflagger {

class FlagMaskData {
 public:
  explicit FlagMaskData(Mask2DPtr theMask) : mask(theMask) {}

  Mask2DPtr mask;
};

class ImageSetData {
 public:
  explicit ImageSetData(size_t initialSize)
      : images(initialSize),
        hasAntennas(false),
        hasInterval(false),
        hasBand(false),
        antenna1(0),
        antenna2(0),
        interval(0),
        band(0) {}

  std::vector<Image2DPtr> images;
  bool hasAntennas, hasInterval, hasBand;
  size_t antenna1, antenna2, interval, band;
};

class QualityStatisticsDataImp {
 public:
  QualityStatisticsDataImp(const double* _scanTimes, size_t nScans,
                           size_t nPolarizations, bool _computeHistograms)
      : scanTimes(_scanTimes, _scanTimes + nScans),
        statistics(nPolarizations),
        histograms(nPolarizations),
        computeHistograms(_computeHistograms) {}
  std::vector<double> scanTimes;
  StatisticsCollection statistics;
  HistogramCollection histograms;
  bool computeHistograms;
};

class QualityStatisticsData {
 public:
  QualityStatisticsData(const double* _scanTimes, size_t nScans,
                        size_t nPolarizations, bool computeHistograms)
      : _implementation(new QualityStatisticsDataImp(
            _scanTimes, nScans, nPolarizations, computeHistograms)) {}
  explicit QualityStatisticsData(
      std::shared_ptr<QualityStatisticsDataImp> implementation)
      : _implementation(implementation) {}
  std::shared_ptr<QualityStatisticsDataImp> _implementation;
};

}  // namespace aoflagger

#endif
