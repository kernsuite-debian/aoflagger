#ifndef TF_PAGE_CONTROLLER_H
#define TF_PAGE_CONTROLLER_H

#include <utility>
#include <vector>

#include "heatmappagecontroller.h"

class TFPageController : public HeatMapPageController {
 public:
  TFPageController();

  void SetStatistics(const StatisticsCollection* statCollection,
                     const std::vector<class AntennaInfo>&) override final {
    _statCollection = statCollection;
    UpdateImage();
  }
  void CloseStatistics() override final { _statCollection = nullptr; }
  bool HasStatistics() const { return _statCollection != nullptr; }

 protected:
  std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> constructImage(
      QualityTablesFormatter::StatisticKind kind) final override;

 private:
  const StatisticsCollection* _statCollection;
};

#endif
