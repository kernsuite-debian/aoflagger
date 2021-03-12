#ifndef HORIZONTALPLOTSCALE_H
#define HORIZONTALPLOTSCALE_H

#include <memory>
#include <string>
#include <vector>

#include <gtkmm/drawingarea.h>

#include "linkable.h"

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
  double Height() { return _height; }
  double RightMargin() { return Data().rightMargin; }
  double FromLeft() const { return Data().fromLeft; }
  double PlotWidth() const { return Data().plotWidth; }
  void Draw(const Cairo::RefPtr<Cairo::Context>& cairo);
  void InitializeNumericTicks(double min, double max);
  void InitializeTimeTicks(double timeMin, double timeMax);
  void InitializeTextTicks(const std::vector<std::string>& labels);
  void InitializeLogarithmicTicks(double min, double max);
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
  void drawDescription(const Cairo::RefPtr<Cairo::Context>& cairo);
  bool ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo);
  void initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo);
  void initializeLocalMetrics(const Cairo::RefPtr<Cairo::Context>& cairo,
                              double& plotWidth, double& fromLeft,
                              double& rightMargin);

  // These are set through SetDimensions()
  double _widgetWidth, _widgetHeight, _minFromLeft, _fromTop;
  bool _isSecondAxis;

  // These are set in initializeMetrics()
  double _height;

  std::unique_ptr<class TickSet> _tickSet;
  bool _drawWithDescription;
  std::string _unitsCaption;
  double _descriptionFontSize;
  double _tickValuesFontSize;
  bool _rotateUnits, _isLogarithmic;
  sigc::signal<void> _signalLinkedRedraw;
};

#endif
