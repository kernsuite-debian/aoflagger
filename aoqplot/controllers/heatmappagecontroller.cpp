#include "heatmappagecontroller.h"

#include "../../quality/statisticscollection.h"
#include "../../quality/statisticsderivator.h"

#include "../grayscaleplotpage.h"

#include "../../structures/samplerow.h"

HeatMapPageController::HeatMapPageController()
    : _page(nullptr),
      _statisticKind(QualityTablesFormatter::StandardDeviationStatistic),
      _polarization(aocommon::Polarization::StokesI),
      _phase(TimeFrequencyData::AmplitudePart) {
  _heatMap.SetCairoFilter(Cairo::FILTER_NEAREST);
  _heatMap.SetColorMap(ColorMap::HotCold);
  _heatMap.SetZRange(FullRange());
  _heatMap.SetLogZScale(true);
  _heatMap.SetZAxisDescription("Statistical value");
  _heatMap.SetManualZAxisDescription(true);
}

void HeatMapPageController::updateImageImpl(
    QualityTablesFormatter::StatisticKind statisticKind,
    aocommon::PolarizationEnum polarisation,
    enum TimeFrequencyData::ComplexRepresentation phase) {
  std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> pair =
      constructImage(statisticKind);

  TimeFrequencyData& data = pair.first;

  if (!data.IsEmpty()) {
    setToPolarization(data, polarisation);

    setToPhase(data, phase);

    Image2DCPtr image = data.GetSingleImage();
    if (_page != nullptr) {
      if (_page->NormalizeXAxis())
        image = Image2D::MakePtr(normalizeXAxis(*image));
      if (_page->NormalizeYAxis())
        image = Image2D::MakePtr(normalizeYAxis(*image));
    }

    _heatMap.SetZAxisDescription(
        StatisticsDerivator::GetDescWithUnits(statisticKind));
    _heatMap.SetImage(std::unique_ptr<PlotImage>(new PlotImage(image)));
    _heatMap.SetOriginalMask(data.GetSingleMask());
    if (pair.second != nullptr) _heatMap.SetMetaData(pair.second);

    if (_page != nullptr) _page->Redraw();
  }
}

Image2D HeatMapPageController::normalizeXAxis(const Image2D& input) {
  Image2D output = Image2D::MakeUnsetImage(input.Width(), input.Height());
  for (size_t x = 0; x < input.Width(); ++x) {
    const SampleRow row = SampleRow::MakeFromColumn(&input, x);
    num_t norm;
    if (_normalization == Mean)
      norm = 1.0 / row.MeanWithMissings();
    else if (_normalization == Winsorized)
      norm = 1.0 / row.WinsorizedMeanWithMissings();
    else  // _medianNormButton
      norm = 1.0 / row.MedianWithMissings();
    for (size_t y = 0; y < input.Height(); ++y)
      output.SetValue(x, y, input.Value(x, y) * norm);
  }
  return output;
}

Image2D HeatMapPageController::normalizeYAxis(const Image2D& input) {
  Image2D output = Image2D::MakeUnsetImage(input.Width(), input.Height());
  for (size_t y = 0; y < input.Height(); ++y) {
    const SampleRow row = SampleRow::MakeFromRow(&input, y);
    num_t norm;
    if (_normalization == Mean)
      norm = 1.0 / row.MeanWithMissings();
    else if (_normalization == Winsorized)
      norm = 1.0 / row.WinsorizedMeanWithMissings();
    else  // _medianNormButton
      norm = 1.0 / row.MedianWithMissings();
    for (size_t x = 0; x < input.Width(); ++x)
      output.SetValue(x, y, input.Value(x, y) * norm);
  }
  return output;
}

void HeatMapPageController::setToPolarization(
    TimeFrequencyData& data, aocommon::PolarizationEnum polarisation) {
  if ((polarisation == aocommon::Polarization::StokesI &&
       data.HasPolarization(aocommon::Polarization::XX) &&
       data.HasPolarization(aocommon::Polarization::YY)) ||
      (polarisation != aocommon::Polarization::StokesI &&
       data.HasPolarization(polarisation))) {
    data = data.Make(polarisation);
    if (polarisation == aocommon::Polarization::StokesI)
      data.MultiplyImages(0.5);
  } else {
    data.SetImagesToZero();
  }
}

void HeatMapPageController::setToPhase(
    TimeFrequencyData& data,
    enum TimeFrequencyData::ComplexRepresentation phase) {
  data = data.Make(phase);
}
