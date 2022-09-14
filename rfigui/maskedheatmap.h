#ifndef MASKED_HEAT_MAP_H
#define MASKED_HEAT_MAP_H

#include "plotimage.h"

#include "../plot/heatmap.h"

#include "../structures/mask2d.h"
#include "../structures/segmentedimage.h"
#include "../structures/timefrequencymetadata.h"

namespace algorithms {
class ThresholdConfig;
}

class MaskedHeatMap : public HeatMap {
 public:
  MaskedHeatMap();
  ~MaskedHeatMap();

  void Clear();

  TimeFrequencyMetaDataCPtr GetSelectedMetaData() const;
  const TimeFrequencyMetaDataCPtr& GetFullMetaData() const { return _metaData; }
  void SetMetaData(const TimeFrequencyMetaDataCPtr& metaData);

  Mask2DCPtr OriginalMask() const { return _originalMask; }
  void SetOriginalMask(Mask2DCPtr mask) {
    _originalMask = mask;
    Invalidate();
  }

  Mask2DCPtr AlternativeMask() const { return _alternativeMask; }
  void SetAlternativeMask(Mask2DCPtr mask) {
    _alternativeMask = mask;
    Invalidate();
  }

  bool ShowOriginalMask() const { return _showOriginalMask; }
  void SetShowOriginalMask(bool newValue) {
    _showOriginalMask = newValue;
    Invalidate();
  }

  bool ShowAlternativeMask() const { return _showAlternativeMask; }
  void SetShowAlternativeMask(bool newValue) {
    _showAlternativeMask = newValue;
    Invalidate();
  }

  Mask2DCPtr GetActiveMask() const;

  void SetHighlighting(bool newValue) {
    _highlighting = newValue;
    Invalidate();
  }
  algorithms::ThresholdConfig& HighlightConfig() { return *_highlightConfig; }

  SegmentedImagePtr GetSegmentedImage() const { return _segmentedImage; }

  void SetSegmentedImage(SegmentedImagePtr segmentedImage) {
    _segmentedImage = segmentedImage;
  }

  bool ManualXAxisDescription() const { return _manualXAxisDescription; }
  void SetManualXAxisDescription(bool manualDesc) {
    _manualXAxisDescription = manualDesc;
    Invalidate();
  }
  bool ManualYAxisDescription() const { return _manualYAxisDescription; }
  void SetManualYAxisDescription(bool manualDesc) {
    _manualYAxisDescription = manualDesc;
    Invalidate();
  }
  bool ManualZAxisDescription() const { return _manualZAxisDescription; }
  void SetManualZAxisDescription(bool manualDesc) {
    _manualZAxisDescription = manualDesc;
    Invalidate();
  }

  void SaveText(const std::string& filename) const;
  void SaveByExtension(const std::string& filename, size_t width,
                       size_t height);

  Image2DCPtr GetImage2D() const {
    return static_cast<const PlotImage&>(Image()).Get();
  }

 private:
  TimeFrequencyMetaDataCPtr _metaData;
  Mask2DCPtr _originalMask;
  Mask2DCPtr _alternativeMask;
  SegmentedImagePtr _segmentedImage;
  bool _showOriginalMask;
  bool _showAlternativeMask;
  bool _highlighting;
  std::unique_ptr<algorithms::ThresholdConfig> _highlightConfig;
  bool _manualXAxisDescription;
  bool _manualYAxisDescription;
  bool _manualZAxisDescription;

  void signalDrawImage(const Cairo::RefPtr<Cairo::ImageSurface>& surface) const;
};

#endif
