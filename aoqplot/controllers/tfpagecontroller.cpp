#include "tfpagecontroller.h"

#include "../../quality/statisticscollection.h"
#include "../../quality/statisticsderivator.h"

TFPageController::TFPageController() : _statCollection(nullptr) {
  HeatMap& map = Plot();
  map.SetXAxisDescription("Time index");
  map.SetXAxisType(AxisType::kTime);
  map.SetYAxisDescription("Frequency index");
}

std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr>
TFPageController::constructImage(QualityTablesFormatter::StatisticKind kind) {
  if (HasStatistics()) {
    StatisticsDerivator derivator(*_statCollection);

    std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> data =
        derivator.CreateTFData(kind);
    if (data.second == nullptr) {
      Plot().SetXAxisDescription("Time index");
      Plot().SetYAxisDescription("Frequency index");
    } else {
      Plot().SetXAxisDescription("Time");
      Plot().SetYAxisDescription("Frequency (MHz)");
    }
    return data;
  } else {
    return std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr>(
        TimeFrequencyData(), TimeFrequencyMetaDataCPtr());
  }
}
