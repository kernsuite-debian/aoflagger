#ifndef QUALITY_STATISTICS_DATA_H
#define QUALITY_STATISTICS_DATA_H

#include "aoflagger.h"
#include "structures.h"

namespace aoflagger {

QualityStatistics::QualityStatistics() : _data(nullptr) {}

QualityStatistics::QualityStatistics(const double* scanTimes, size_t nScans,
                                     const double* channelFrequencies,
                                     size_t nChannels, size_t nPolarizations,
                                     bool computeHistograms)
    : _data(new QualityStatisticsData(scanTimes, nScans, nPolarizations,
                                      computeHistograms)) {
  _data->_implementation->statistics.InitializeBand(0, channelFrequencies,
                                                    nChannels);
}

QualityStatistics::QualityStatistics(const QualityStatistics& sourceQS)
    : _data(sourceQS._data == nullptr
                ? nullptr
                : new QualityStatisticsData(sourceQS._data->_implementation)) {}

QualityStatistics::QualityStatistics(QualityStatistics&& sourceQS)
    : _data(std::move(sourceQS._data)) {
  sourceQS._data = nullptr;
}

QualityStatistics::~QualityStatistics() {}

QualityStatistics& QualityStatistics::operator=(
    const QualityStatistics& sourceQS) {
  if (_data == nullptr) {
    if (sourceQS._data != nullptr)
      _data.reset(new QualityStatisticsData(sourceQS._data->_implementation));
  } else {
    if (sourceQS._data != nullptr) {
      _data->_implementation = sourceQS._data->_implementation;
    } else {
      _data.reset();
    }
  }
  return *this;
}

QualityStatistics& QualityStatistics::operator=(QualityStatistics&& sourceQS) {
  if (_data == nullptr || sourceQS._data == nullptr) {
    std::swap(_data, sourceQS._data);
  } else {
    _data->_implementation = std::move(sourceQS._data->_implementation);
  }
  return *this;
}

QualityStatistics& QualityStatistics::operator+=(const QualityStatistics& rhs) {
  _data->_implementation->statistics.Add(
      rhs._data->_implementation->statistics);
  _data->_implementation->histograms.Add(
      rhs._data->_implementation->histograms);
  return *this;
}

void QualityStatistics::CollectStatistics(const ImageSet& imageSet,
                                          const FlagMask& rfiFlags,
                                          const FlagMask& correlatorFlags,
                                          size_t antenna1, size_t antenna2) {
  StatisticsCollection& stats(_data->_implementation->statistics);
  HistogramCollection& histograms(_data->_implementation->histograms);
  const std::vector<double>& times(_data->_implementation->scanTimes);

  if (imageSet.ImageCount() == 1) {
    stats.AddImage(antenna1, antenna2, &times[0], 0, 0,
                   imageSet._data->images[0], imageSet._data->images[0],
                   rfiFlags._data->mask, correlatorFlags._data->mask);
    if (_data->_implementation->computeHistograms) {
      histograms.Add(antenna1, antenna2, 0, imageSet._data->images[0],
                     rfiFlags._data->mask, correlatorFlags._data->mask);
    }
  } else {
    const size_t polarizationCount = imageSet.ImageCount() / 2;
    for (size_t polarization = 0; polarization != polarizationCount;
         ++polarization) {
      stats.AddImage(antenna1, antenna2, &times[0], 0, polarization,
                     imageSet._data->images[polarization * 2],
                     imageSet._data->images[polarization * 2 + 1],
                     rfiFlags._data->mask, correlatorFlags._data->mask);
      if (_data->_implementation->computeHistograms) {
        histograms.Add(antenna1, antenna2, polarization,
                       imageSet._data->images[polarization * 2],
                       imageSet._data->images[polarization * 2 + 1],
                       rfiFlags._data->mask, correlatorFlags._data->mask);
      }
    }
  }
}

void QualityStatistics::WriteStatistics(
    const std::string& measurementSetPath) const {
  QualityTablesFormatter qFormatter(measurementSetPath);
  _data->_implementation->statistics.Save(qFormatter);

  HistogramCollection& histograms(_data->_implementation->histograms);
  if (!histograms.Empty()) {
    HistogramTablesFormatter hFormatter(measurementSetPath);
    histograms.Save(hFormatter);
  }
}

}  // end of namespace aoflagger

#endif
