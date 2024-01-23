#ifndef VERTICAL_PLOT_SCALE_H
#define VERTICAL_PLOT_SCALE_H

#include "linkable.h"
#include "axis.h"

#include <array>
#include <string>
#include <vector>
#include <memory>

#include <gtkmm/drawingarea.h>

typedef std::pair<double, std::string> Tick;

class VerticalPlotScale : public Linkable<VerticalPlotScale> {
 public:
  VerticalPlotScale();
  ~VerticalPlotScale();

  void SetPlotDimensions(double plotWidth, double plotHeight, double fromTop,
                         bool isSecondAxis) {
    _plotWidth = plotWidth;
    _plotHeight = plotHeight;
    _fromTop = fromTop;
    _isSecondAxis = isSecondAxis;
    _metricsAreInitialized = false;
  }

  void AlignTo(VerticalPlotScale& base) {
    _alignTo = &base;
    _metricsAreInitialized = false;
  }

  double GetWidth(const Cairo::RefPtr<Cairo::Context>& cairo) {
    initializeMetrics(cairo);
    return _width;
  }

  double GetTickTextHeight(const Cairo::RefPtr<Cairo::Context>& cairo);

  void SetDrawWithDescription(bool drawWithDescription) {
    _drawWithDescription = drawWithDescription;
    _metricsAreInitialized = false;
  }
  void SetUnitsCaption(const std::string& caption) {
    _unitsCaption = caption;
    _metricsAreInitialized = false;
  }
  void SetDescriptionFontSize(double fontSize) {
    _tickValuesFontSize = fontSize;
    _metricsAreInitialized = false;
  }
  void SetTickValuesFontSize(double fontSize) {
    _tickValuesFontSize = fontSize;
    _metricsAreInitialized = false;
  }

  void Draw(const Cairo::RefPtr<Cairo::Context>& cairo, double offsetX,
            double offsetY);
  void SetAxisType(AxisType axisType) { _axisType = axisType; }
  void SetTickRange(double min, double max) {
    _tickRange = std::array<double, 2>{min, max};
  }
  double GetTickRangeMin() const { return _tickRange[0]; }
  double GetTickRangeMax() const { return _tickRange[1]; }
  void SetLogarithmic(bool logarithmic) { _isLogarithmic = logarithmic; }
  bool IsLogarithmic() const { return _isLogarithmic; }
  void InitializeTicks();
  double UnitToAxis(double unitValue) const;
  double AxisToUnit(double axisValue) const;

 private:
  void drawDescription(const Cairo::RefPtr<Cairo::Context>& cairo,
                       double offsetX, double offsetY);
  bool ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo);
  void initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo);
  double getTickYPosition(const Tick& tick);

  // These are set through SetPlotDimensions()
  double _plotWidth, _plotHeight, _fromTop;
  bool _isSecondAxis;

  // These are calculated by initializeMetrics()
  double _width, _captionSize;

  bool _metricsAreInitialized;
  VerticalPlotScale* _alignTo;
  std::unique_ptr<class TickSet> _tickSet;
  AxisType _axisType;
  std::array<double, 2> _tickRange;
  bool _isLogarithmic;
  bool _drawWithDescription;
  std::string _unitsCaption;
  double _descriptionFontSize;
  double _tickValuesFontSize;
};

#endif
