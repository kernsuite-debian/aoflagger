#ifndef HEAT_MAP_PLOT_H
#define HEAT_MAP_PLOT_H

#include <cairomm/context.h>

#include <sigc++/signal.h>

#include "../structures/colormap.h"
#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"
#include "../structures/segmentedimage.h"

#include "plotbase.h"

class HeatMapPlot : public PlotBase {
 public:
  enum Range { MinMax, Winsorized, Specified };
  enum ScaleOption { NormalScale, LogScale, ZeroSymmetricScale };

  HeatMapPlot();
  ~HeatMapPlot();

  void Clear();

  bool ShowOriginalMask() const { return _showOriginalMask; }
  void SetShowOriginalMask(bool newValue) {
    _showOriginalMask = newValue;
    _isImageInvalidated = true;
  }

  bool ShowAlternativeMask() const { return _showAlternativeMask; }
  void SetShowAlternativeMask(bool newValue) {
    _showAlternativeMask = newValue;
    _isImageInvalidated = true;
  }

  ColorMap::Type GetColorMap() const { return _colorMap; }
  void SetColorMap(ColorMap::Type colorMap) {
    _colorMap = colorMap;
    _isImageInvalidated = true;
  }

  void Draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
            size_t height) final override;

  Image2DCPtr Image() const { return _image; }
  void SetImage(Image2DCPtr image) {
    _image = image;
    _isImageInvalidated = true;
  }

  Mask2DCPtr OriginalMask() const { return _originalMask; }
  void SetOriginalMask(Mask2DCPtr mask) {
    _originalMask = mask;
    _isImageInvalidated = true;
  }

  Mask2DCPtr AlternativeMask() const { return _alternativeMask; }
  void SetAlternativeMask(Mask2DCPtr mask) {
    _alternativeMask = mask;
    _isImageInvalidated = true;
  }

  Mask2DCPtr GetActiveMask() const;

  void SetRange(enum Range range) {
    _range = range;
    _isImageInvalidated = true;
  }
  enum Range Range() const { return _range; }
  void SetScaleOption(ScaleOption option) {
    _scaleOption = option;
    _isImageInvalidated = true;
  }
  enum ScaleOption ScaleOption() const { return _scaleOption; }

  void ZoomFit();
  void ZoomIn();
  void ZoomInOn(size_t x, size_t y);
  void ZoomOut();
  void ZoomTo(size_t x1, size_t y1, size_t x2, size_t y2);
  void Pan(int xDisplacement, int yDisplacement);

  double StartHorizontal() const { return _startHorizontal; }
  double EndHorizontal() const { return _endHorizontal; }
  double StartVertical() const { return _startVertical; }
  double EndVertical() const { return _endVertical; }
  void SetSegmentedImage(SegmentedImageCPtr segmentedImage) {
    _segmentedImage = segmentedImage;
  }

  void SetHighlighting(bool newValue) {
    _highlighting = newValue;
    _isImageInvalidated = true;
  }
  class ThresholdConfig& HighlightConfig() {
    return *_highlightConfig;
  }

  bool HasImage() const { return _image != nullptr; }

  TimeFrequencyMetaDataCPtr GetSelectedMetaData() const;
  const TimeFrequencyMetaDataCPtr& GetFullMetaData() const { return _metaData; }
  void SetMetaData(TimeFrequencyMetaDataCPtr metaData) { _metaData = metaData; }

  num_t Max() const { return _specifiedMax; }
  num_t Min() const { return _specifiedMin; }

  void SetMax(num_t max) {
    _specifiedMax = max;
    _isImageInvalidated = true;
  }
  void SetMin(num_t min) {
    _specifiedMin = min;
    _isImageInvalidated = true;
  }

  void SaveByExtension(const std::string& filename, size_t width,
                       size_t height);
  void SavePdf(const std::string& filename, size_t width, size_t height);
  void SaveSvg(const std::string& filename, size_t width, size_t height);
  void SavePng(const std::string& filename, size_t width, size_t height);
  void SaveText(const std::string& filename);

  bool ShowTitle() const { return _showTitle; }
  void SetShowTitle(bool showTitle) {
    _showTitle = showTitle;
    _isImageInvalidated = true;
  }

  bool ShowXYAxes() const { return _showXYAxes; }
  void SetShowXYAxes(bool showXYAxes) {
    _showXYAxes = showXYAxes;
    _isImageInvalidated = true;
  }

  bool ShowColorScale() const { return _showColorScale; }
  void SetShowColorScale(bool showColorScale) {
    _showColorScale = showColorScale;
    _isImageInvalidated = true;
  }

  bool ShowXAxisDescription() const { return _showXAxisDescription; }
  void SetShowXAxisDescription(bool showXAxisDescription) {
    _showXAxisDescription = showXAxisDescription;
    _isImageInvalidated = true;
  }

  bool ShowYAxisDescription() const { return _showYAxisDescription; }
  void SetShowYAxisDescription(bool showYAxisDescription) {
    _showYAxisDescription = showYAxisDescription;
    _isImageInvalidated = true;
  }

  bool ShowZAxisDescription() const { return _showZAxisDescription; }
  void SetShowZAxisDescription(bool showZAxisDescription) {
    _showZAxisDescription = showZAxisDescription;
    _isImageInvalidated = true;
  }

  void SetCairoFilter(Cairo::Filter filter) {
    _cairoFilter = filter;
    _isImageInvalidated = true;
  }
  Cairo::Filter CairoFilter() const { return _cairoFilter; }
  void SetTitleText(const std::string& title) {
    _titleText = title;
    _isImageInvalidated = true;
  }

  const std::string& XAxisDescription() const { return _xAxisDescription; }
  void SetXAxisDescription(const std::string& description) {
    _xAxisDescription = description;
    _isImageInvalidated = true;
  }
  const std::string& YAxisDescription() const { return _yAxisDescription; }
  void SetYAxisDescription(const std::string& description) {
    _yAxisDescription = description;
    _isImageInvalidated = true;
  }
  const std::string& ZAxisDescription() const { return _zAxisDescription; }
  void SetZAxisDescription(const std::string& description) {
    _zAxisDescription = description;
    _isImageInvalidated = true;
  }

  bool ManualTitle() const { return _manualTitle; }
  void SetManualTitle(bool manualTitle) {
    _manualTitle = manualTitle;
    _isImageInvalidated = true;
  }

  const std::string& ManualTitleText() { return _manualTitleText; }
  void SetManualTitleText(const std::string& manualTitle) {
    _manualTitleText = manualTitle;
    _isImageInvalidated = true;
  }

  bool ManualXAxisDescription() const { return _manualXAxisDescription; }
  void SetManualXAxisDescription(bool manualDesc) {
    _manualXAxisDescription = manualDesc;
    _isImageInvalidated = true;
  }
  bool ManualYAxisDescription() const { return _manualYAxisDescription; }
  void SetManualYAxisDescription(bool manualDesc) {
    _manualYAxisDescription = manualDesc;
    _isImageInvalidated = true;
  }
  bool ManualZAxisDescription() const { return _manualZAxisDescription; }
  void SetManualZAxisDescription(bool manualDesc) {
    _manualZAxisDescription = manualDesc;
    _isImageInvalidated = true;
  }

  bool IsZoomedOut() const {
    return _startHorizontal == 0.0 && _endHorizontal == 1.0 &&
           _startVertical == 0.0 && _endVertical == 1.0;
  }

  sigc::signal<void>& OnZoomChanged() { return _onZoomChanged; }

  bool ConvertToUnits(double mouseX, double mouseY, int& posX, int& posY) const;
  bool ConvertToScreen(int posX, int posY, double& mouseX,
                       double& mouseY) const;

 private:
  void redrawWithoutChanges(const Cairo::RefPtr<Cairo::Context>& cairo,
                            size_t width, size_t height);

  bool _isInitialized, _isImageInvalidated;
  size_t _initializedWidth, _initializedHeight;
  bool _showOriginalMask, _showAlternativeMask;
  ColorMap::Type _colorMap;
  TimeFrequencyMetaDataCPtr _metaData;
  Image2DCPtr _image;
  Mask2DCPtr _originalMask, _alternativeMask;
  bool _highlighting;
  double _leftBorderSize, _rightBorderSize, _topBorderSize, _bottomBorderSize;

  double _startHorizontal, _endHorizontal;
  double _startVertical, _endVertical;
  SegmentedImageCPtr _segmentedImage;
  std::unique_ptr<class ColorScale> _colorScale;
  std::unique_ptr<class Title> _plotTitle;
  enum ScaleOption _scaleOption;
  bool _showXYAxes;
  bool _showColorScale;
  bool _showXAxisDescription;
  bool _showYAxisDescription;
  bool _showZAxisDescription;
  bool _showTitle;
  num_t _specifiedMax, _specifiedMin;
  num_t _derivedMax, _derivedMin;
  std::string _titleText, _manualTitleText;
  enum Range _range;
  Cairo::Filter _cairoFilter;
  std::string _xAxisDescription, _yAxisDescription, _zAxisDescription;
  bool _manualTitle;
  bool _manualXAxisDescription;
  bool _manualYAxisDescription;
  bool _manualZAxisDescription;
  sigc::signal<void> _onZoomChanged;
  std::unique_ptr<class ThresholdConfig> _highlightConfig;

  void findMinMax(const Image2D* image, const Mask2D* mask, num_t& min,
                  num_t& max);
  void update(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
              size_t height);
  void downsampleImageBuffer(size_t newWidth, size_t newHeight);
  std::string actualTitleText() const {
    if (_manualTitle)
      return _manualTitleText;
    else
      return _titleText;
  }

  Cairo::RefPtr<Cairo::ImageSurface> _imageSurface;
};

#endif
