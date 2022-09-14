#ifndef HORIZONTAL_PLOT_SCALE_H
#define HORIZONTAL_PLOT_SCALE_H

#include "axistype.h"
#include "linkable.h"

#include <gtkmm/drawingarea.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

struct HorizontalPlotScaleData {
  // These are set in initializeMetrics()
  double plotWidth = 0.0, fromLeft = 0.0, rightMargin = 0.0;
  bool metricsAreInitialized = false;
};

class HorizontalPlotScale
    : public Linkable<HorizontalPlotScale, HorizontalPlotScaleData> {
 public:
  HorizontalPlotScale();
  ~HorizontalPlotScale();

  void SetPlotDimensions(double widgetWidth, double widgetHeight,
                         double minFromLeft, double fromTop,
                         bool isSecondAxis) {
    if (_widgetWidth != widgetWidth || _widgetHeight != widgetHeight ||
        _minFromLeft != minFromLeft || _fromTop != fromTop ||
        _isSecondAxis != isSecondAxis) {
      _widgetWidth = widgetWidth;
      _widgetHeight = widgetHeight;
      _minFromLeft = minFromLeft;
      _fromTop = fromTop;
      _isSecondAxis = isSecondAxis;
      Data().metricsAreInitialized = false;
    }
  }
  double CalculateHeight(const Cairo::RefPtr<Cairo::Context>& cairo);
  double RightMargin() { return Data().rightMargin; }
  double FromLeft() const { return Data().fromLeft; }
  double PlotWidth() const { return Data().plotWidth; }
  void Draw(const Cairo::RefPtr<Cairo::Context>& cairo);
  void SetAxisType(AxisType axisType) { _axisType = axisType; }
  void SetTickLabels(const std::vector<std::string> labels) {
    _tickLabels = std::move(labels);
  }
  void SetTickRange(double min, double max) {
    _tickRange = std::array<double, 2>{min, max};
  }
  double GetTickRangeMin() const { return _tickRange[0]; }
  double GetTickRangeMax() const { return _tickRange[1]; }
  void SetLogarithmic(bool logarithmic) { _isLogarithmic = logarithmic; }
  void InitializeTicks();
  void SetDrawWithDescription(bool drawWithDescription) {
    _drawWithDescription = drawWithDescription;
    Data().metricsAreInitialized = false;
  }
  void SetUnitsCaption(const std::string& caption) {
    _unitsCaption = caption;
    Data().metricsAreInitialized = false;
  }
  void SetDescriptionFontSize(double fontSize) {
    _tickValuesFontSize = fontSize;
    Data().metricsAreInitialized = false;
  }
  void SetTickValuesFontSize(double fontSize) {
    _tickValuesFontSize = fontSize;
    Data().metricsAreInitialized = false;
  }
  void SetRotateUnits(bool rotate) {
    _rotateUnits = rotate;
    Data().metricsAreInitialized = false;
  }
  double UnitToAxis(double unitValue) const;
  double AxisToUnit(double axisValue) const;
  void CalculateScales(const Cairo::RefPtr<Cairo::Context>& cairo) {
    initializeMetrics(cairo);
  }
  sigc::signal<void>& SignalLinkedRedraw() { return _signalLinkedRedraw; }

 private:
  void initializeNumericTicks(double min, double max);
  void initializeTimeTicks(double timeMin, double timeMax);
  void initializeTextTicks(const std::vector<std::string>& labels);
  void drawDescription(const Cairo::RefPtr<Cairo::Context>& cairo);
  bool ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo);
  void initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo);
  void initializeLocalMetrics(const Cairo::RefPtr<Cairo::Context>& cairo,
                              double& plotWidth, double& fromLeft,
                              double& rightMargin);

  // These are set through SetDimensions()
  double _widgetWidth, _widgetHeight, _minFromLeft, _fromTop;
  bool _isSecondAxis;

  std::unique_ptr<class TickSet> _tickSet;
  AxisType _axisType;
  std::array<double, 2> _tickRange;
  std::vector<std::string> _tickLabels;
  bool _isLogarithmic;
  bool _rotateUnits;

  bool _drawWithDescription;
  std::string _unitsCaption;
  double _descriptionFontSize;
  double _tickValuesFontSize;
  sigc::signal<void> _signalLinkedRedraw;
};

#endif
