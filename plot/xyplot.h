#ifndef XYPLOT_H
#define XYPLOT_H

#include <gtkmm/drawingarea.h>

#include <stdexcept>
#include <string>

#include "legend.h"
#include "plotbase.h"
#include "system.h"
#include "title.h"
#include "xypointset.h"

class XYPlot : public PlotBase {
 public:
  enum RangeDetermination { MinMaxRange, WinsorizedRange, SpecifiedRange };

  XYPlot();
  ~XYPlot();

  void Clear();

  // TODO
  bool ConvertToPlot(double screenX, double screenY, double& posX,
                     double& posY) const override {
    return false;
  }
  // TODO
  bool ConvertToScreen(double posX, double posY, double& screenX,
                       double& screenY) const override {
    return false;
  }

  XYPointSet& StartLine(
      const std::string& label, const std::string& xDesc = "x",
      const std::string& yDesc = "y", bool xIsTime = false,
      enum XYPointSet::DrawingStyle drawingStyle = XYPointSet::DrawLines);
  XYPointSet& StartLine(const std::string& label,
                        enum XYPointSet::DrawingStyle drawingStyle) {
    return StartLine(label, "x", "y", false, drawingStyle);
  }
  XYPointSet& StartLine() {
    return StartLine("", "x", "y", false, XYPointSet::DrawLines);
  }
  void PushDataPoint(double x, double y) {
    if (_pointSets.size() > 0)
      (*_pointSets.rbegin())->PushDataPoint(x, y);
    else
      throw std::runtime_error(
          "Trying to push a data point into a plot without point sets (call "
          "StartLine first).");
  }
  size_t PointSetCount() const { return _pointSets.size(); }
  XYPointSet& GetPointSet(size_t index) { return *_pointSets[index]; }
  const XYPointSet& GetPointSet(size_t index) const {
    return *_pointSets[index];
  }
  void SetLogarithmicXAxis(bool logarithmicXAxis) {
    _logarithmicXAxis = logarithmicXAxis;
  }
  bool LogarithmicXAxis() const { return _logarithmicXAxis; }
  void SetIncludeZeroYAxis(bool includeZeroAxis) {
    _system.SetIncludeZeroYAxis(includeZeroAxis);
    if (includeZeroAxis) _logarithmicYAxis = false;
  }
  void SetLogarithmicYAxis(bool logarithmicYAxis) {
    _logarithmicYAxis = logarithmicYAxis;
    if (_logarithmicYAxis) _system.SetIncludeZeroYAxis(false);
  }
  bool LogarithmicYAxis() const { return _logarithmicYAxis; }
  void SetHRangeDetermination(enum RangeDetermination range) {
    _hRangeDetermination = range;
  }
  enum RangeDetermination HRangeDetermination() const {
    return _hRangeDetermination;
  }
  void SetVRangeDetermination(enum RangeDetermination range) {
    _vRangeDetermination = range;
  }
  enum RangeDetermination VRangeDetermination() const {
    return _vRangeDetermination;
  }
  void SetMaxX(double maxX) {
    _hRangeDetermination = SpecifiedRange;
    _specifiedMaxX = maxX;
  }

  std::pair<double, double> RangeX() const;
  std::pair<double, double> RangePositiveX() const;
  std::pair<double, double> RangeY() const;
  std::pair<double, double> RangePositiveY() const;

  void SetMinX(double minX) {
    _hRangeDetermination = SpecifiedRange;
    _specifiedMinX = minX;
  }
  void SetMaxY(double maxY) {
    _vRangeDetermination = SpecifiedRange;
    _specifiedMaxY = maxY;
  }
  void SetMinY(double minY) {
    _vRangeDetermination = SpecifiedRange;
    _specifiedMinY = minY;
  }
  void SetShowXAxis(bool showAxis) { _showXAxis = showAxis; }
  void SetShowYAxis(bool showYAxis) { _showYAxis = showYAxis; }
  bool ShowXAxis() const { return _showXAxis; }
  bool ShowYAxis() const { return _showYAxis; }
  void SetShowAxisDescriptions(bool showAxisDescriptions) {
    _showAxisDescriptions = showAxisDescriptions;
  }
  bool ShowAxisDescriptions() const { return _showAxisDescriptions; }
  void SetTitle(const std::string& title) { _title.SetText(title); }
  void SetCustomHorizontalAxisDescription(const std::string& description) {
    _customHAxisDescription = description;
  }
  void SetCustomVerticalAxisDescription(const std::string& description) {
    _customVAxisDescription = description;
  }
  void SetAutomaticHorizontalAxisDescription() {
    _customHAxisDescription = std::string();
  }
  void SetAutomaticVerticalAxisDescription() {
    _customVAxisDescription = std::string();
  }
  void SavePdf(const std::string& filename, size_t width,
               size_t height) final override;
  void SaveSvg(const std::string& filename, size_t width,
               size_t height) final override;
  void SavePng(const std::string& filename, size_t width,
               size_t height) final override;
  void SavePdf(const std::string& filename) {
    SavePdf(filename, _width, _height);
  }
  void SaveSvg(const std::string& filename) {
    SaveSvg(filename, _width, _height);
  }
  void SavePng(const std::string& filename) {
    SavePng(filename, _width, _height);
  }

  const std::string& Title() const { return _title.Text(); }

 protected:
  void draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
            size_t height) final override;

 private:
  void render(const Cairo::RefPtr<Cairo::Context>& cr);
  void render(const Cairo::RefPtr<Cairo::Context>& cr, XYPointSet& pointSet);
  // TODO
  Rectangle getPlotArea(size_t width, size_t height) const override {
    return Rectangle();
  }

  Legend _legend;
  std::vector<std::unique_ptr<XYPointSet>> _pointSets;
  int _width, _height;
  double _topMargin;
  System _system;
  bool _logarithmicXAxis, _logarithmicYAxis, _showXAxis, _showYAxis,
      _showAxisDescriptions;
  double _specifiedMinX, _specifiedMaxX, _specifiedMinY, _specifiedMaxY;
  enum RangeDetermination _hRangeDetermination, _vRangeDetermination;
  class Title _title;
  std::string _customHAxisDescription, _customVAxisDescription;
};

#endif
