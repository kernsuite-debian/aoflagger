#ifndef HEAT_MAP_H
#define HEAT_MAP_H

#include <cairomm/surface.h>
#include <cairomm/context.h>

#include <sigc++/signal.h>

#include "colormap.h"
#include "colorscale.h"
#include "imageinterface.h"
#include "plotbase.h"
#include "title.h"

enum class Range { MinMax, Winsorized, Specified };

class HeatMap : public PlotBase {
 public:
  HeatMap();
  ~HeatMap();

  void Clear();

  void Invalidate() { _isImageInvalidated = true; }

  ColorMap::Type GetColorMap() const { return _colorMap; }
  void SetColorMap(ColorMap::Type colorMap) {
    _colorMap = colorMap;
    _isImageInvalidated = true;
  }

  const ImageInterface& Image() const { return *_image; }
  void SetImage(std::unique_ptr<ImageInterface> image) {
    _image = std::move(image);
    _isImageInvalidated = true;
  }

  bool HasImage() const { return _image != nullptr; }

  void SetZRange(enum Range zRange) {
    _zRange = zRange;
    _isImageInvalidated = true;
  }
  enum Range ZRange() const { return _zRange; }

  std::array<size_t, 2> ImageXRange() const;
  std::array<size_t, 2> ImageYRange() const;
  /**
   * For very large images (>30K), the data will be downsampled before
   * being drawn on the imagesurface. This is because image surfaces
   * have a limited size. These two functions return the factor by which
   * the image was downsampled. This is particularly important when implementing
   * @ref SignalDrawImage().
   * @{
   */
  size_t ImageToSurfaceXFactor() const;
  size_t ImageToSurfaceYFactor() const;
  /** @} */

  void SetLogXScale(bool logXScale) {
    _logXScale = logXScale;
    _isImageInvalidated = true;
  }
  bool LogXScale() const { return _logXScale; }
  void SetLogYScale(bool logYScale) {
    _logYScale = logYScale;
    _isImageInvalidated = true;
  }
  bool LogYScale() const { return _logYScale; }
  void SetLogZScale(bool logZScale) {
    _logZScale = logZScale;
    _isImageInvalidated = true;
  }
  bool LogZScale() const { return _logZScale; }
  void SetXAxisType(AxisType axisType) {
    _xAxisType = axisType;
    _isImageInvalidated = true;
  }
  AxisType XAxisType() const { return _xAxisType; }

  double Max() const { return _specifiedMax; }
  double Min() const { return _specifiedMin; }

  void SetMax(double max) {
    _specifiedMax = max;
    _isImageInvalidated = true;
  }
  void SetMin(double min) {
    _specifiedMin = min;
    _isImageInvalidated = true;
  }

  void SavePdf(const std::string& filename, size_t width,
               size_t height) final override;
  void SaveSvg(const std::string& filename, size_t width,
               size_t height) final override;
  void SavePng(const std::string& filename, size_t width,
               size_t height) final override;

  bool ShowTitle() const { return _showTitle; }
  void SetShowTitle(bool showTitle) {
    _showTitle = showTitle;
    _isImageInvalidated = true;
  }

  bool ShowXAxis() const { return _showXAxis; }
  void SetShowXAxis(bool showXAxis) {
    _showXAxis = showXAxis;
    _isImageInvalidated = true;
  }

  bool ShowYAxis() const { return _showYAxis; }
  void SetShowYAxis(bool showYAxis) {
    _showYAxis = showYAxis;
    _isImageInvalidated = true;
  }

  void SetShowX2Axis(bool showX2Axis) {
    _showX2Axis = showX2Axis;
    _isImageInvalidated = true;
  }
  void SetShowY2Axis(bool showY2Axis) {
    _showY2Axis = showY2Axis;
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

  const std::string& TitleText() const { return _titleText; }
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

  bool ConvertToPlot(double screenX, double screenY, double& posX,
                     double& posY) const final override;
  bool ConvertToScreen(double posX, double posY, double& screenX,
                       double& screenY) const final override;
  bool UnitToImage(double posX, double posY, size_t& x, size_t& y) const;
  void ImageToUnit(size_t x, size_t y, double& posX, double& posY) const;

  void SetXAxisMin(double xAxisMin) { _xAxisMin = xAxisMin; }
  void SetXAxisMax(double xAxisMax) { _xAxisMax = xAxisMax; }
  void SetYAxisMin(double yAxisMin) { _yAxisMin = yAxisMin; }
  void SetYAxisMax(double yAxisMax) { _yAxisMax = yAxisMax; }
  void SetX2AxisMin(double x2AxisMin) { _x2AxisMin = x2AxisMin; }
  void SetX2AxisMax(double x2AxisMax) { _x2AxisMax = x2AxisMax; }
  void SetY2AxisMin(double y2AxisMin) { _y2AxisMin = y2AxisMin; }
  void SetY2AxisMax(double y2AxisMax) { _y2AxisMax = y2AxisMax; }

  /**
   * Called after drawing the plot on to the cairo context. This allows the user
   * to draw on the plot. The cairo context will be set up such that the plotted
   * units can be used. The exception is when the axes are logarithmic; in that
   * case the natural logarithm should be taken of the plot unit before drawing.
   */
  sigc::signal<void(const Cairo::RefPtr<Cairo::Context>&)>& SignalDraw() {
    return _signalDraw;
  }
  /**
   * Called after having converted the plot data to a heat map image, before it
   * is shown. This can be used to modify the heat map before it is displayed,
   * e.g. to add a mask.
   */
  sigc::signal<void(const Cairo::RefPtr<Cairo::ImageSurface>&)>&
  SignalDrawImage() {
    return _signalDrawImage;
  }

 protected:
  void draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
            size_t height) final override;

 private:
  void findMinMax(const ImageInterface& image, float& min, float& max);
  void postRender(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
                  size_t height);
  void redrawWithoutChanges(const Cairo::RefPtr<Cairo::Context>& cairo,
                            size_t width, size_t height);
  void drawAll(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
               size_t height);
  /**
   * Sets the trivial fields of the components (axes, title, etc.).
   * It does not yet initialize their size.
   */
  void initializeComponents();
  void downsampleImageBuffer(size_t newWidth, size_t newHeight);
  Rectangle getPlotArea(size_t width, size_t height) const final override;

  bool _isInitialized;
  bool _isImageInvalidated;
  size_t _initializedWidth, _initializedHeight;
  Cairo::RefPtr<Cairo::ImageSurface> _imageSurface;

  ColorMap::Type _colorMap;
  std::unique_ptr<ImageInterface> _image;
  double _leftBorderSize, _rightBorderSize;
  double _topBorderSize, _bottomBorderSize;
  double _topAxisHeight;

  ColorScale _colorScale;
  Title _plotTitle;
  bool _logXScale, _logYScale, _logZScale;
  bool _showXAxis;
  bool _showYAxis;
  bool _showX2Axis;
  bool _showY2Axis;
  bool _showColorScale;
  bool _showXAxisDescription;
  bool _showYAxisDescription;
  bool _showZAxisDescription;
  bool _showTitle;
  AxisType _xAxisType;
  AxisType _yAxisType;
  float _specifiedMax, _specifiedMin;
  float _derivedMax, _derivedMin;
  double _xAxisMin, _xAxisMax;
  double _yAxisMin, _yAxisMax;
  double _x2AxisMin, _x2AxisMax;
  double _y2AxisMin, _y2AxisMax;
  std::string _titleText;
  enum Range _zRange;
  Cairo::Filter _cairoFilter;
  std::string _xAxisDescription;
  std::string _x2AxisDescription;
  std::string _yAxisDescription;
  std::string _y2AxisDescription;
  std::string _zAxisDescription;

  sigc::signal<void(const Cairo::RefPtr<Cairo::Context>&)> _signalDraw;
  sigc::signal<void(const Cairo::RefPtr<Cairo::ImageSurface>&)>
      _signalDrawImage;
};

#endif
