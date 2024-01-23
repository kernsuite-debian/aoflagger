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

enum class RangeDetermination { MinMaxRange, WinsorizedRange, SpecifiedRange };

class XYPlotAxis : public Axis {
 public:
  bool Show() const { return show_; }
  void SetShow(bool show) { show_ = show; }

  void SetMax(double max) {
    range_determination_ = RangeDetermination::SpecifiedRange;
    specified_max_ = max;
  }
  double SpecifiedMax() const { return specified_max_; }
  void SetMin(double min) {
    range_determination_ = RangeDetermination::SpecifiedRange;
    specified_min_ = min;
  }
  double SpecifiedMin() const { return specified_min_; }
  std::string SpecifiedDescription() const { return specified_description_; }
  void SetCustomDescription(const std::string& description) {
    specified_description_ = description;
  }
  void SetAutomaticDescription() { specified_description_ = std::string(); }
  void SetLogarithmic(bool logarithmic) { logarithmic_ = logarithmic; }
  bool Logarithmic() const { return logarithmic_; }
  void SetRangeDetermination(RangeDetermination range) {
    range_determination_ = range;
  }
  RangeDetermination GetRangeDetermination() const {
    return range_determination_;
  }

 private:
  bool logarithmic_ = false;
  bool show_ = true;
  double specified_min_ = 0.0;
  double specified_max_ = 0.0;
  RangeDetermination range_determination_ = RangeDetermination::MinMaxRange;
  std::string specified_description_;
};

class XYPlot final : public PlotBase {
 public:
  XYPlot();

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
      const std::string& yDesc = "y",
      enum XYPointSet::DrawingStyle drawingStyle = XYPointSet::DrawLines);
  XYPointSet& StartLine(const std::string& label,
                        enum XYPointSet::DrawingStyle drawingStyle) {
    return StartLine(label, "x", "y", drawingStyle);
  }
  XYPointSet& StartLine() {
    return StartLine("", "x", "y", XYPointSet::DrawLines);
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
  void SetIncludeZeroYAxis(bool includeZeroAxis) {
    _system.SetIncludeZeroYAxis(includeZeroAxis);
    if (includeZeroAxis) y_axis_.SetLogarithmic(false);
  }

  std::pair<double, double> RangeX(bool second_axis) const;
  std::pair<double, double> RangePositiveX(bool second_axis) const;
  std::pair<double, double> RangeY(bool second_axis) const;
  std::pair<double, double> RangePositiveY(bool second_axis) const;

  void SetShowAxisDescriptions(bool showAxisDescriptions) {
    _showAxisDescriptions = showAxisDescriptions;
  }
  bool ShowAxisDescriptions() const { return _showAxisDescriptions; }
  void SetTitle(const std::string& title) { _title.SetText(title); }
  void SavePdf(const std::string& filename, size_t width,
               size_t height) override;
  void SaveSvg(const std::string& filename, size_t width,
               size_t height) override;
  void SavePng(const std::string& filename, size_t width,
               size_t height) override;

  const std::string& GetTitle() const { return _title.Text(); }

  XYPlotAxis& XAxis() { return x_axis_; }
  const XYPlotAxis& XAxis() const { return x_axis_; }

  XYPlotAxis& YAxis() { return y_axis_; }
  const XYPlotAxis& YAxis() const { return y_axis_; }

  XYPlotAxis& X2Axis() { return x2_axis_; }
  const XYPlotAxis& X2Axis() const { return x2_axis_; }

  XYPlotAxis& Y2Axis() { return y2_axis_; }
  const XYPlotAxis& Y2Axis() const { return y2_axis_; }

 protected:
  void Draw(const Cairo::RefPtr<Cairo::Context>& cairo) override;

 private:
  void InitializeComponents(const Cairo::RefPtr<Cairo::Context>& cr);
  void InitializeYAxis(bool second_axis, double horizontal_scale_height);
  void DrawFrame(const Cairo::RefPtr<Cairo::Context>& cr);
  void DrawPointSet(const Cairo::RefPtr<Cairo::Context>& cr,
                    XYPointSet& pointSet);

  static std::pair<double, double> CheckRange(
      const std::pair<double, double>& range) {
    if (!std::isfinite(range.first) || !std::isfinite(range.second)) {
      return {-1.0, 1.0};
    } else if (range.first == range.second) {
      return {range.first - 1.0, range.second + 1.0};
    } else {
      return range;
    }
  }

  // TODO
  Rectangle getPlotArea(size_t width, size_t height) const override {
    return Rectangle();
  }

  const XYPointSet* GetFirstXAxisSet(bool second_axis) const {
    for (const std::unique_ptr<XYPointSet>& set : _pointSets) {
      if (set->UseSecondXAxis() == second_axis) {
        return set.get();
      }
    }
    return nullptr;
  }

  const XYPointSet* GetFirstYAxisSet(bool second_axis) const {
    for (const std::unique_ptr<XYPointSet>& set : _pointSets) {
      if (set->UseSecondYAxis() == second_axis) {
        return set.get();
      }
    }
    return nullptr;
  }

  double _topMargin = 0;
  bool _showAxisDescriptions = true;
  std::vector<std::unique_ptr<XYPointSet>> _pointSets;
  XYPlotAxis x_axis_;
  XYPlotAxis x2_axis_;
  XYPlotAxis y_axis_;
  XYPlotAxis y2_axis_;
  Legend _legend;
  System _system;
  Title _title;
};

#endif
