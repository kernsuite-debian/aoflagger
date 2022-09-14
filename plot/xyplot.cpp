#include "xyplot.h"

XYPlot::XYPlot()
    : _width(640),
      _height(480),
      _topMargin(0.0),
      _logarithmicXAxis(false),
      _logarithmicYAxis(false),
      _showXAxis(true),
      _showYAxis(true),
      _showAxisDescriptions(true),
      _specifiedMinX(0.0),
      _specifiedMaxX(0.0),
      _specifiedMinY(0.0),
      _specifiedMaxY(0.0),
      _hRangeDetermination(MinMaxRange),
      _vRangeDetermination(MinMaxRange) {}

XYPlot::~XYPlot() { Clear(); }

void XYPlot::Clear() {
  _pointSets.clear();
  _system.Clear();
}

void XYPlot::draw(const Cairo::RefPtr<Cairo::Context>& cr, size_t width,
                  size_t height) {
  _width = width;
  _height = height;
  render(cr);
}

XYPointSet& XYPlot::StartLine(const std::string& label,
                              const std::string& xDesc,
                              const std::string& yDesc, bool xIsTime,
                              enum XYPointSet::DrawingStyle drawingStyle) {
  _pointSets.emplace_back(new XYPointSet(_pointSets.size()));
  XYPointSet& newSet = *_pointSets.back();
  newSet.SetLabel(label);
  newSet.SetXIsTime(xIsTime);
  newSet.SetXDesc(xDesc);
  newSet.SetYDesc(yDesc);
  newSet.SetDrawingStyle(drawingStyle);
  return newSet;
}

void XYPlot::SavePdf(const std::string& filename, size_t width, size_t height) {
  Cairo::RefPtr<Cairo::PdfSurface> surface =
      Cairo::PdfSurface::create(filename, width, height);
  Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  render(cairo);
  cairo->show_page();
  surface->finish();
}

void XYPlot::SaveSvg(const std::string& filename, size_t width, size_t height) {
  Cairo::RefPtr<Cairo::SvgSurface> surface =
      Cairo::SvgSurface::create(filename, width, height);
  Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  render(cairo);
  cairo->show_page();
  surface->finish();
}

void XYPlot::SavePng(const std::string& filename, size_t width, size_t height) {
  Cairo::RefPtr<Cairo::ImageSurface> surface =
      Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  render(cairo);
  surface->write_to_png(filename);
}

void XYPlot::render(const Cairo::RefPtr<Cairo::Context>& cr) {
  _system.Clear();
  for (std::unique_ptr<XYPointSet>& set : _pointSets) _system.AddToSystem(*set);

  cr->set_line_width(2);

  cr->set_source_rgba(1, 1, 1, 1);
  cr->paint();
  cr->fill();

  if (!_pointSets.empty()) {
    XYPointSet& refPointSet = **_pointSets.begin();

    double horiScaleHeight;

    _topMargin = 0.0;

    double titleHeight = 0.0;
    if (!_title.Text().empty()) {
      _title.SetPlotDimensions(_width, _height, _topMargin);
      titleHeight = _title.GetHeight(cr);
    }

    if (_showYAxis)
      _topMargin += std::max(10.0, titleHeight);
    else
      _topMargin += titleHeight;

    double leftMargin, plotWidth;
    if (_showXAxis) {
      _horizontalScale.SetDrawWithDescription(_showAxisDescriptions);
      _horizontalScale.SetRotateUnits(refPointSet.RotateUnits());
      if (_logarithmicXAxis) {
        const std::pair<double, double> range = RangePositiveX();
        _horizontalScale.SetTickRange(range.first, range.second);
      } else {
        const std::pair<double, double> range = RangeX();
        _horizontalScale.SetTickRange(range.first, range.second);
      }
      _horizontalScale.SetLogarithmic(_logarithmicXAxis);
      AxisType axisType = AxisType::kNumeric;
      if (refPointSet.HasTickLabels()) {
        _horizontalScale.SetTickLabels(refPointSet.TickLabels());
        axisType = AxisType::kText;
      } else if (refPointSet.XIsTime()) {
        axisType = AxisType::kTime;
      } else {
        axisType = AxisType::kNumeric;
      }
      _horizontalScale.SetAxisType(axisType);
      _horizontalScale.InitializeTicks();
      _horizontalScale.SetUnitsCaption(_customHAxisDescription.empty()
                                           ? refPointSet.XUnits()
                                           : _customHAxisDescription);
      _horizontalScale.SetPlotDimensions(_width, _height, 0.0, _topMargin,
                                         false);
      _horizontalScale.CalculateScales(cr);
      horiScaleHeight = _horizontalScale.CalculateHeight(cr);
    } else {
      horiScaleHeight = 0.0;
    }

    if (_showYAxis) {
      double axisRightMargin = _horizontalScale.RightMargin();
      _verticalScale.SetDrawWithDescription(_showAxisDescriptions);
      if (_logarithmicYAxis) {
        auto range = RangePositiveY();
        _verticalScale.SetTickRange(range.first, range.second);
      } else {
        auto range = RangeY();
        _verticalScale.SetTickRange(range.first, range.second);
      }
      _verticalScale.SetLogarithmic(_logarithmicYAxis);
      _verticalScale.InitializeTicks();
      _verticalScale.SetUnitsCaption(_customVAxisDescription.empty()
                                         ? refPointSet.YUnits()
                                         : _customVAxisDescription);
      _verticalScale.SetPlotDimensions(_width - axisRightMargin,
                                       _height - horiScaleHeight - _topMargin,
                                       _topMargin, false);
    }

    double verticalScaleWidth = _verticalScale.GetWidth(cr);
    _horizontalScale.SetPlotDimensions(_width, _height, verticalScaleWidth,
                                       _topMargin, false);
    _horizontalScale.CalculateScales(cr);
    leftMargin = _horizontalScale.FromLeft();
    plotWidth = _horizontalScale.PlotWidth();

    for (size_t i = 0; i != _pointSets.size(); ++i) {
      auto c = _pointSets[i]->GetColor();
      cr->set_source_rgba(c.r, c.g, c.b, c.a);
      render(cr, *_pointSets[i]);
    }

    if (_showXAxis) _horizontalScale.Draw(cr);
    if (_showYAxis)
      _verticalScale.Draw(cr, leftMargin - _verticalScale.GetWidth(cr), 0.0);

    _legend.Initialize(cr, *this);
    // For top left:
    //_legend.SetPosition(verticalScaleWidth+10, _topMargin+10);
    _legend.SetPosition(plotWidth + leftMargin - 10 - _legend.Width(),
                        _topMargin + 10);

    if (!_title.Text().empty()) {
      _title.Draw(cr);
    }

    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->rectangle(leftMargin, _topMargin, plotWidth,
                  _height - horiScaleHeight - _topMargin);
    cr->stroke();

    _legend.Draw(cr, *this);
  }
}

void XYPlot::render(const Cairo::RefPtr<Cairo::Context>& cr,
                    XYPointSet& pointSet) {
  pointSet.Sort();

  auto xRange = _logarithmicXAxis ? RangePositiveX() : RangeX();
  auto yRange = _logarithmicYAxis ? RangePositiveY() : RangeY();
  double xLeft = xRange.first, xRight = xRange.second;
  double yMin = yRange.first, yMax = yRange.second;

  if (!std::isfinite(xLeft) || !std::isfinite(xRight)) {
    xLeft = -1;
    xRight = 1;
  } else if (xLeft == xRight) {
    xLeft -= 1;
    xRight += 1;
  }
  if (!std::isfinite(yMin) || !std::isfinite(yMax)) {
    yMin = -1;
    yMax = 1;
  } else if (yMin == yMax) {
    yMin -= 1;
    yMax += 1;
  }

  const double bottomMargin =
      _showXAxis ? _horizontalScale.CalculateHeight(cr) : 0.0;
  const double plotLeftMargin = _showYAxis ? _horizontalScale.FromLeft() : 0.0;

  const double plotWidth = _horizontalScale.PlotWidth();
  const double plotHeight = _height - bottomMargin - _topMargin;

  cr->rectangle(plotLeftMargin, _topMargin, plotWidth, plotHeight);
  cr->clip();

  double minXLog10 = log10(xLeft), maxXLog10 = log10(xRight),
         minYLog10 = log10(yMin), maxYLog10 = log10(yMax);

  bool hasPrevPoint = false;

  unsigned iterationCount = pointSet.Size();
  bool isNextPointRequired =
      pointSet.DrawingStyle() == XYPointSet::DrawLines && iterationCount != 0;
  if (isNextPointRequired) --iterationCount;

  for (size_t i = 0; i < iterationCount; ++i) {
    double x1Val, x2Val, y1Val, y2Val;

    double px = pointSet.GetX(i), py = pointSet.GetY(i);
    if (_logarithmicXAxis) {
      if (px <= 0.0)
        x1Val = 0.0;
      else
        x1Val = (log10(px) - minXLog10) / (maxXLog10 - minXLog10);
    } else {
      x1Val = (px - xLeft) / (xRight - xLeft);
    }

    if (_logarithmicYAxis) {
      if (py <= 0.0)
        y1Val = 0.0;
      else
        y1Val = (log10(py) - minYLog10) / (maxYLog10 - minYLog10);
    } else {
      y1Val = (py - yMin) / (yMax - yMin);
    }
    if (y1Val < 0.0) y1Val = 0.0;
    if (y1Val > 1.0) y1Val = 1.0;

    if (isNextPointRequired) {
      double pxn = pointSet.GetX(i + 1), pyn = pointSet.GetY(i + 1);
      if (_logarithmicXAxis) {
        if (pxn <= 0.0)
          x2Val = 0.0;
        else
          x2Val = (log10(pxn) - minXLog10) / (maxXLog10 - minXLog10);
      } else {
        x2Val = (pxn - xLeft) / (xRight - xLeft);
      }

      if (_logarithmicYAxis) {
        if (pyn <= 0.0)
          y2Val = 0.0;
        else
          y2Val = (log10(pyn) - minYLog10) / (maxYLog10 - minYLog10);
      } else {
        y2Val = (pyn - yMin) / (yMax - yMin);
      }

      if (y2Val < 0.0) y2Val = 0.0;
      if (y2Val > 1.0) y2Val = 1.0;
    } else {
      x2Val = 0.0;
      y2Val = 0.0;
    }

    double x1 = x1Val * plotWidth + plotLeftMargin,
           x2 = x2Val * plotWidth + plotLeftMargin,
           y1 = (1.0 - y1Val) * plotHeight + _topMargin,
           y2 = (1.0 - y2Val) * plotHeight + _topMargin;

    if (std::isfinite(x1) && std::isfinite(y1)) {
      switch (pointSet.DrawingStyle()) {
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
    } else {
    }
  }
  switch (pointSet.DrawingStyle()) {
    case XYPointSet::DrawLines:
      cr->stroke();
      break;
    case XYPointSet::DrawPoints:
      cr->fill();
      break;
    case XYPointSet::DrawColumns:
      cr->fill_preserve();
      Cairo::RefPtr<Cairo::Pattern> source = cr->get_source();
      cr->set_source_rgb(0.0, 0.0, 0.0);
      cr->stroke();
      cr->set_source(source);
      break;
  }

  // Draw "zero y" x-axis
  if (yMin <= 0.0 && yMax >= 0.0 && !_logarithmicYAxis) {
    cr->set_source_rgba(0.5, 0.5, 0.5, 1);
    cr->move_to(plotLeftMargin, yMax * plotHeight / (yMax - yMin) + _topMargin);
    cr->line_to(plotWidth + plotLeftMargin,
                yMax * plotHeight / (yMax - yMin) + _topMargin);
    cr->stroke();
  }

  cr->reset_clip();
}

std::pair<double, double> XYPlot::RangeX() const {
  if (_hRangeDetermination == SpecifiedRange)
    return std::make_pair(_specifiedMinX, _specifiedMaxX);
  else if (_pointSets.empty())
    return std::make_pair(0.0, 1.0);
  else {
    double maxX = _system.XRangeMax(*_pointSets.front()),
           minX = _system.XRangeMin(*_pointSets.front());
    return std::make_pair(minX, maxX);
  }
}

std::pair<double, double> XYPlot::RangePositiveX() const {
  if (_hRangeDetermination == SpecifiedRange)
    return std::make_pair(_specifiedMinX, _specifiedMaxX);
  else if (_pointSets.empty())
    return std::make_pair(0.1, 1.0);
  else {
    double maxX = _system.XRangePositiveMax(*_pointSets.front()),
           minX = _system.XRangePositiveMin(*_pointSets.front());
    return std::make_pair(minX, maxX);
  }
}

std::pair<double, double> XYPlot::RangeY() const {
  if (_vRangeDetermination == SpecifiedRange)
    return std::make_pair(_specifiedMinY, _specifiedMaxY);
  else if (_pointSets.empty())
    return std::make_pair(0.0, 1.0);
  else {
    double minY = _system.YRangeMin(*_pointSets.front()),
           maxY = _system.YRangeMax(*_pointSets.front()),
           extMin = minY * 1.07 - maxY * 0.07,
           extMax = maxY * 1.07 - minY * 0.07;
    if (extMin < 0.0 && minY >= 0.0) {
      extMax -= extMin;
      extMin = 0.0;
    }
    return std::make_pair(extMin, extMax);
  }
}

std::pair<double, double> XYPlot::RangePositiveY() const {
  if (_vRangeDetermination == SpecifiedRange)
    return std::make_pair(_specifiedMinY, _specifiedMaxY);
  else if (_pointSets.empty())
    return std::make_pair(0.0, 1.0);
  else {
    double maxY = log(_system.YRangePositiveMax(*_pointSets.front())),
           minY = log(_system.YRangePositiveMin(*_pointSets.front())),
           extMin = exp(minY * 1.07 - maxY * 0.07),
           extMax = exp(maxY * 1.07 - minY * 0.07);
    return std::make_pair(extMin, extMax);
  }
}
