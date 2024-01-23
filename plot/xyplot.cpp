#include "xyplot.h"

XYPlot::XYPlot() {
  x2_axis_.SetShow(false);
  y2_axis_.SetShow(false);
}

void XYPlot::Clear() {
  _pointSets.clear();
  _system.Clear();
}

XYPointSet& XYPlot::StartLine(const std::string& label,
                              const std::string& xDesc,
                              const std::string& yDesc,
                              XYPointSet::DrawingStyle drawingStyle) {
  _pointSets.emplace_back(std::make_unique<XYPointSet>(_pointSets.size()));
  XYPointSet& newSet = *_pointSets.back();
  newSet.SetLabel(label);
  newSet.SetXDesc(xDesc);
  newSet.SetYDesc(yDesc);
  newSet.SetDrawingStyle(drawingStyle);
  return newSet;
}

void XYPlot::SavePdf(const std::string& filename, size_t width, size_t height) {
  const Cairo::RefPtr<Cairo::PdfSurface> surface =
      Cairo::PdfSurface::create(filename, width, height);
  const Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  PlotBase::Draw(cairo, width, height);
  cairo->show_page();
  surface->finish();
}

void XYPlot::SaveSvg(const std::string& filename, size_t width, size_t height) {
  const Cairo::RefPtr<Cairo::SvgSurface> surface =
      Cairo::SvgSurface::create(filename, width, height);
  const Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  PlotBase::Draw(cairo, width, height);
  cairo->show_page();
  surface->finish();
}

void XYPlot::SavePng(const std::string& filename, size_t width, size_t height) {
  const Cairo::RefPtr<Cairo::ImageSurface> surface =
      Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  const Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  PlotBase::Draw(cairo, width, height);
  surface->write_to_png(filename);
}

void XYPlot::InitializeYAxis(bool second_axis, double horizontal_scale_height) {
  const XYPlotAxis& axis = second_axis ? y2_axis_ : y_axis_;
  VerticalPlotScale& scale = second_axis ? _vertScale2 : _verticalScale;
  if (axis.Show()) {
    const double axisRightMargin = _horizontalScale.RightMargin();
    scale.SetDrawWithDescription(_showAxisDescriptions);
    const std::pair<double, double> range =
        axis.Logarithmic() ? RangePositiveY(second_axis) : RangeY(second_axis);
    scale.SetTickRange(range.first, range.second);
    scale.SetLogarithmic(axis.Logarithmic());
    scale.InitializeTicks();
    const XYPointSet* refPointSet = GetFirstYAxisSet(second_axis);
    const std::string units =
        refPointSet ? refPointSet->YUnits() : (second_axis ? "y2" : "y");
    scale.SetUnitsCaption(axis.SpecifiedDescription().empty()
                              ? units
                              : axis.SpecifiedDescription());
    scale.SetPlotDimensions(Width() - axisRightMargin,
                            Height() - horizontal_scale_height - _topMargin,
                            _topMargin, second_axis);
  }
}

void XYPlot::InitializeComponents(const Cairo::RefPtr<Cairo::Context>& cr) {
  _topMargin = 0.0;

  double titleHeight = 0.0;
  if (!_title.Text().empty()) {
    _title.SetPlotDimensions(Width(), Height(), _topMargin);
    titleHeight = _title.GetHeight(cr);
  }

  if (y_axis_.Show() || y2_axis_.Show())
    _topMargin += std::max(10.0, titleHeight);
  else
    _topMargin += titleHeight;

  if (x_axis_.Show()) {
    _horizontalScale.SetDrawWithDescription(_showAxisDescriptions);
    _horizontalScale.SetRotateUnits(x_axis_.RotateUnits());
    if (x_axis_.Logarithmic()) {
      const std::pair<double, double> range = RangePositiveX(false);
      _horizontalScale.SetTickRange(range.first, range.second);
    } else {
      const std::pair<double, double> range = RangeX(false);
      _horizontalScale.SetTickRange(range.first, range.second);
    }
    _horizontalScale.SetLogarithmic(x_axis_.Logarithmic());
    if (x_axis_.Type() == AxisType::kText) {
      _horizontalScale.SetTickLabels(x_axis_.TickLabels());
    }
    _horizontalScale.SetAxisType(x_axis_.Type());
    _horizontalScale.InitializeTicks();
    const XYPointSet& refPointSet = **_pointSets.begin();
    _horizontalScale.SetUnitsCaption(x_axis_.SpecifiedDescription().empty()
                                         ? refPointSet.XUnits()
                                         : x_axis_.SpecifiedDescription());
    _horizontalScale.SetPlotDimensions(Width(), Height(), 0.0, _topMargin,
                                       false);
    _horizontalScale.CalculateScales(cr);
  }

  // The scale dimensions are depending on each other. However, since the
  // height of the horizontal scale is practically not dependent on other
  // dimensions, it is assumed to be constant.
  const double horizontal_scale_height =
      x_axis_.Show() ? _horizontalScale.CalculateHeight(cr) : 0.0;

  InitializeYAxis(false, horizontal_scale_height);
  InitializeYAxis(true, horizontal_scale_height);

  const double verticalScaleWidth = _verticalScale.GetWidth(cr);
  const double y2_scale_width =
      y2_axis_.Show() ? _vertScale2.GetWidth(cr) : 0.0;
  _horizontalScale.SetPlotDimensions(Width() - y2_scale_width, Height(),
                                     verticalScaleWidth, _topMargin, false);
  _horizontalScale.CalculateScales(cr);
  const double leftMargin = _horizontalScale.FromLeft();
  const double plotWidth = _horizontalScale.PlotWidth();

  _legend.Initialize(cr, *this);
  // For top left:
  //_legend.SetPosition(verticalScaleWidth+10, _topMargin+10);
  _legend.SetPosition(plotWidth + leftMargin - 10 - _legend.Width(),
                      _topMargin + 10);
}

void XYPlot::DrawFrame(const Cairo::RefPtr<Cairo::Context>& cr) {
  const double left_margin = _horizontalScale.FromLeft();
  const double plot_width = _horizontalScale.PlotWidth();
  const double x_scale_height =
      x_axis_.Show() ? _horizontalScale.CalculateHeight(cr) : 0.0;
  cr->set_source_rgb(0.0, 0.0, 0.0);
  cr->rectangle(left_margin, _topMargin, plot_width,
                Height() - x_scale_height - _topMargin);
  cr->stroke();
}

void XYPlot::Draw(const Cairo::RefPtr<Cairo::Context>& cr) {
  // Clear the surface
  cr->set_source_rgba(1, 1, 1, 1);
  cr->paint();
  cr->fill();

  _system.Clear();
  for (std::unique_ptr<XYPointSet>& set : _pointSets) _system.AddToSystem(*set);

  if (!_pointSets.empty()) {
    InitializeComponents(cr);

    for (size_t i = 0; i != _pointSets.size(); ++i) {
      auto c = _pointSets[i]->GetColor();
      cr->set_source_rgba(c.r, c.g, c.b, c.a);
      DrawPointSet(cr, *_pointSets[i]);
    }

    if (x_axis_.Show()) _horizontalScale.Draw(cr);
    if (y_axis_.Show())
      _verticalScale.Draw(
          cr, _horizontalScale.FromLeft() - _verticalScale.GetWidth(cr), 0.0);
    if (y2_axis_.Show())
      _vertScale2.Draw(
          cr, _horizontalScale.FromLeft() + _horizontalScale.PlotWidth(), 0.0);

    if (!_title.Text().empty()) {
      _title.Draw(cr);
    }
    DrawFrame(cr);
    _legend.Draw(cr, *this);
  }
}

void XYPlot::DrawPointSet(const Cairo::RefPtr<Cairo::Context>& cr,
                          XYPointSet& pointSet) {
  pointSet.Sort();
  const double bottomMargin =
      x_axis_.Show() ? _horizontalScale.CalculateHeight(cr) : 0.0;
  const double plotLeftMargin =
      y_axis_.Show() ? _horizontalScale.FromLeft() : 0.0;

  const double plotWidth = _horizontalScale.PlotWidth();
  const double plotHeight = Height() - bottomMargin - _topMargin;

  cr->rectangle(plotLeftMargin, _topMargin, plotWidth, plotHeight);
  cr->clip();

  const std::pair<double, double> x_range = CheckRange(
      x_axis_.Logarithmic() ? RangePositiveX(pointSet.UseSecondXAxis())
                            : RangeX(pointSet.UseSecondXAxis()));
  const std::pair<double, double> y_range = CheckRange(
      y_axis_.Logarithmic() ? RangePositiveY(pointSet.UseSecondYAxis())
                            : RangeY(pointSet.UseSecondYAxis()));

  const double xLeft = x_range.first;
  const double xRight = x_range.second;
  const double yMin = y_range.first;
  const double yMax = y_range.second;

  const double minXLog10 = std::log10(xLeft);
  const double maxXLog10 = std::log10(xRight);
  const double minYLog10 = std::log10(yMin);
  const double maxYLog10 = std::log10(yMax);

  bool hasPrevPoint = false;

  unsigned iterationCount = pointSet.Size();
  const bool isNextPointRequired =
      pointSet.GetDrawingStyle() == XYPointSet::DrawLines &&
      iterationCount != 0;
  if (isNextPointRequired) --iterationCount;

  for (size_t i = 0; i < iterationCount; ++i) {
    double x1Val, x2Val, y1Val, y2Val;

    const double px = pointSet.GetX(i);
    if (x_axis_.Logarithmic()) {
      if (px <= 0.0)
        x1Val = 0.0;
      else
        x1Val = (std::log10(px) - minXLog10) / (maxXLog10 - minXLog10);
    } else {
      x1Val = (px - xLeft) / (xRight - xLeft);
    }

    const double py = pointSet.GetY(i);
    if (y_axis_.Logarithmic()) {
      if (py <= 0.0)
        y1Val = 0.0;
      else
        y1Val = (std::log10(py) - minYLog10) / (maxYLog10 - minYLog10);
    } else {
      y1Val = (py - yMin) / (yMax - yMin);
    }
    if (y1Val < 0.0) y1Val = 0.0;
    if (y1Val > 1.0) y1Val = 1.0;

    if (isNextPointRequired) {
      const double pxn = pointSet.GetX(i + 1);
      if (x_axis_.Logarithmic()) {
        if (pxn <= 0.0)
          x2Val = 0.0;
        else
          x2Val = (std::log10(pxn) - minXLog10) / (maxXLog10 - minXLog10);
      } else {
        x2Val = (pxn - xLeft) / (xRight - xLeft);
      }

      const double pyn = pointSet.GetY(i + 1);
      if (y_axis_.Logarithmic()) {
        if (pyn <= 0.0)
          y2Val = 0.0;
        else
          y2Val = (std::log10(pyn) - minYLog10) / (maxYLog10 - minYLog10);
      } else {
        y2Val = (pyn - yMin) / (yMax - yMin);
      }

      if (y2Val < 0.0) y2Val = 0.0;
      if (y2Val > 1.0) y2Val = 1.0;
    } else {
      x2Val = 0.0;
      y2Val = 0.0;
    }

    const double x1 = x1Val * plotWidth + plotLeftMargin;
    const double x2 = x2Val * plotWidth + plotLeftMargin;
    const double y1 = (1.0 - y1Val) * plotHeight + _topMargin;
    const double y2 = (1.0 - y2Val) * plotHeight + _topMargin;

    if (std::isfinite(x1) && std::isfinite(y1)) {
      switch (pointSet.GetDrawingStyle()) {
        case XYPointSet::DrawLines:
          if (std::isfinite(x2) && std::isfinite(y2)) {
            if (!hasPrevPoint) cr->move_to(x1, y1);
            cr->line_to(x2, y2);
            hasPrevPoint = true;
          } else {
            hasPrevPoint = false;
          }
          break;
        case XYPointSet::DrawPoints:
          cr->move_to(x1 + 2.0, y1);
          cr->arc(x1, y1, 2.0, 0.0, 2 * M_PI);
          break;
        case XYPointSet::DrawColumns:
          if (y1 <= _topMargin + plotHeight) {
            double width = 10.0, startX = x1 - width * 0.5,
                   endX = x1 + width * 0.5;
            if (startX < plotLeftMargin) startX = plotLeftMargin;
            if (endX > plotWidth + plotLeftMargin)
              endX = plotWidth + plotLeftMargin;
            cr->rectangle(startX, y1, endX - startX,
                          _topMargin + plotHeight - y1);
          }
          break;
      }
    }
  }
  switch (pointSet.GetDrawingStyle()) {
    case XYPointSet::DrawLines:
      cr->stroke();
      break;
    case XYPointSet::DrawPoints:
      cr->fill();
      break;
    case XYPointSet::DrawColumns:
      cr->fill_preserve();
      const Cairo::RefPtr<Cairo::Pattern> source = cr->get_source();
      cr->set_source_rgb(0.0, 0.0, 0.0);
      cr->stroke();
      cr->set_source(source);
      break;
  }

  // Draw "zero y" x-axis
  if (yMin <= 0.0 && yMax >= 0.0 && !y_axis_.Logarithmic()) {
    cr->set_source_rgba(0.5, 0.5, 0.5, 1);
    cr->move_to(plotLeftMargin, yMax * plotHeight / (yMax - yMin) + _topMargin);
    cr->line_to(plotWidth + plotLeftMargin,
                yMax * plotHeight / (yMax - yMin) + _topMargin);
    cr->stroke();
  }

  cr->reset_clip();
}

std::pair<double, double> XYPlot::RangeX(bool second_axis) const {
  const XYPlotAxis& axis = second_axis ? x2_axis_ : x_axis_;
  if (axis.GetRangeDetermination() == RangeDetermination::SpecifiedRange) {
    return std::make_pair(axis.SpecifiedMin(), axis.SpecifiedMax());
  } else if (_pointSets.empty()) {
    return std::make_pair(0.0, 1.0);
  } else {
    const double maxX = _system.XRangeMax(second_axis);
    const double minX = _system.XRangeMin(second_axis);
    return std::make_pair(minX, maxX);
  }
}

std::pair<double, double> XYPlot::RangePositiveX(bool second_axis) const {
  if (x_axis_.GetRangeDetermination() == RangeDetermination::SpecifiedRange) {
    return std::make_pair(x_axis_.SpecifiedMin(), x_axis_.SpecifiedMax());
  } else if (_pointSets.empty()) {
    return std::make_pair(0.1, 1.0);
  } else {
    const double maxX = _system.XRangePositiveMax(second_axis);
    const double minX = _system.XRangePositiveMin(second_axis);
    return std::make_pair(minX, maxX);
  }
}

std::pair<double, double> XYPlot::RangeY(bool second_axis) const {
  const XYPlotAxis& axis = second_axis ? y2_axis_ : y_axis_;
  if (axis.GetRangeDetermination() == RangeDetermination::SpecifiedRange) {
    return std::make_pair(axis.SpecifiedMin(), axis.SpecifiedMax());
  } else if (_pointSets.empty()) {
    return std::make_pair(0.0, 1.0);
  } else {
    double minY = _system.YRangeMin(second_axis),
           maxY = _system.YRangeMax(second_axis),
           extMin = minY * 1.07 - maxY * 0.07,
           extMax = maxY * 1.07 - minY * 0.07;
    if (extMin < 0.0 && minY >= 0.0) {
      extMax -= extMin;
      extMin = 0.0;
    }
    return std::make_pair(extMin, extMax);
  }
}

std::pair<double, double> XYPlot::RangePositiveY(bool second_axis) const {
  if (y_axis_.GetRangeDetermination() == RangeDetermination::SpecifiedRange) {
    return std::make_pair(y_axis_.SpecifiedMin(), y_axis_.SpecifiedMax());
  } else if (_pointSets.empty()) {
    return std::make_pair(0.1, 1.0);
  } else {
    const double maxY = std::log(_system.YRangePositiveMax(second_axis));
    const double minY = std::log(_system.YRangePositiveMin(second_axis));
    const double extMin = std::exp(minY * 1.07 - maxY * 0.07);
    const double extMax = std::exp(maxY * 1.07 - minY * 0.07);
    return std::make_pair(extMin, extMax);
  }
}
