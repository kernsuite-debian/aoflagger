#ifndef BASELINE_PAGE_CONTROLLER_H
#define BASELINE_PAGE_CONTROLLER_H

#include "heatmappagecontroller.h"

#include <string>
#include <utility>
#include <vector>

class AntennaInfo;

class BaselinePageController final : public HeatMapPageController {
 public:
  BaselinePageController() : _statCollection(nullptr), _antennas(nullptr) {}

  void SetStatistics(const StatisticsCollection* statCollection,
                     const std::vector<class AntennaInfo>& antennas) override {
    _statCollection = statCollection;
    _antennas = &antennas;
    HeatMap& map = Plot();
    map.SetXAxisDescription("Antenna 1 index");
    map.SetXAxisMin(0);
    map.SetXAxisMax(antennas.size() - 1);
    map.SetYAxisDescription("Antenna 2 index");
    map.SetYAxisMin(0);
    map.SetYAxisMax(antennas.size() - 1);
    UpdateImage();
  }
  void CloseStatistics() override {
    _statCollection = nullptr;
    _antennas = nullptr;
  }
  bool HasStatistics() const { return _statCollection != nullptr; }
  std::string AntennaName(size_t index) const;

 protected:
  std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> constructImage(
      QualityTablesFormatter::StatisticKind kind) override;

 private:
  const StatisticsCollection* _statCollection;
  const std::vector<AntennaInfo>* _antennas;
};

#endif
