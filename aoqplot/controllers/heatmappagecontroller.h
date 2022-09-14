#ifndef HEATMAP_PAGE_CONTROLLER_H
#define HEATMAP_PAGE_CONTROLLER_H

#include "aoqplotpagecontroller.h"

#include "../../rfigui/maskedheatmap.h"
#include "../../structures/timefrequencydata.h"

#include <aocommon/polarization.h>

class HeatMapPageController : public AOQPageController {
 public:
  HeatMapPageController();

  void Attach(class GrayScalePlotPage* page) { _page = page; }

  void SavePdf(const std::string& filename,
               QualityTablesFormatter::StatisticKind kind, unsigned width,
               unsigned height) {
    updateImageImpl(kind, aocommon::Polarization::StokesI,
                    TimeFrequencyData::AmplitudePart);
    _heatMap.SavePdf(filename, width, height);
  }

  HeatMap& Plot() { return _heatMap; }

  void UpdateImage() { updateImageImpl(_statisticKind, _polarization, _phase); }

  void SetKind(QualityTablesFormatter::StatisticKind statisticKind) {
    _statisticKind = statisticKind;
  }

  void SetPolarization(aocommon::PolarizationEnum polarization) {
    _polarization = polarization;
  }

  void SetPhase(enum TimeFrequencyData::ComplexRepresentation phase) {
    _phase = phase;
  }
  enum Normalization { Mean, Winsorized, Median };
  void SetNormalization(Normalization normalization) {
    _normalization = normalization;
  }

 protected:
  virtual std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr>
  constructImage(QualityTablesFormatter::StatisticKind kind) = 0;

 private:
  Image2D normalizeXAxis(const Image2D& input);
  Image2D normalizeYAxis(const Image2D& input);
  void updateImageImpl(QualityTablesFormatter::StatisticKind statisticKind,
                       aocommon::PolarizationEnum polarisation,
                       enum TimeFrequencyData::ComplexRepresentation phase);
  void setToPolarization(TimeFrequencyData& data,
                         aocommon::PolarizationEnum polarisation);
  void setToPhase(TimeFrequencyData& data,
                  enum TimeFrequencyData::ComplexRepresentation phase);

  MaskedHeatMap _heatMap;
  class GrayScalePlotPage* _page;

  QualityTablesFormatter::StatisticKind _statisticKind;
  aocommon::PolarizationEnum _polarization;
  enum TimeFrequencyData::ComplexRepresentation _phase;

  enum Normalization _normalization;
};

#endif
