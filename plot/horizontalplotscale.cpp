#include "horizontalplotscale.h"

#include "ticksets/logarithmictickset.h"
#include "ticksets/numerictickset.h"
#include "ticksets/texttickset.h"
#include "ticksets/timetickset.h"

HorizontalPlotScale::HorizontalPlotScale()
    : _widgetWidth(0.0),
      _widgetHeight(0.0),
      _minFromLeft(0.0),
      _fromTop(0.0),
      _isSecondAxis(false),
      _axisType(AxisType::kNumeric),
      _tickRange({0.0, 1.0}),
      _tickLabels(),
      _isLogarithmic(false),
      _rotateUnits(false),
      _drawWithDescription(true),
      _unitsCaption("x"),
      _descriptionFontSize(14),
      _tickValuesFontSize(14) {}

HorizontalPlotScale::~HorizontalPlotScale() { Unlink(); }

double HorizontalPlotScale::UnitToAxis(double unitValue) const {
  return _tickSet ? _tickSet->UnitToAxis(unitValue) : 0.0;
}

double HorizontalPlotScale::AxisToUnit(double axisValue) const {
  return _tickSet ? _tickSet->AxisToUnit(axisValue) : 0.0;
}

void HorizontalPlotScale::Draw(const Cairo::RefPtr<Cairo::Context>& cairo) {
  initializeMetrics(cairo);
  cairo->set_source_rgb(0.0, 0.0, 0.0);
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  double tickDisplacement = _isSecondAxis ? -3.0 : 3.0;
  const double height = CalculateHeight(cairo);
  // Y position of x-axis line
  double yPos = _isSecondAxis ? _fromTop + height : _widgetHeight - height;
  for (unsigned i = 0; i != _tickSet->Size(); ++i) {
    const Tick tick = _tickSet->GetTick(i);
    double x = tick.first * Data().plotWidth + Data().fromLeft;
    cairo->move_to(x, yPos);
    cairo->line_to(x, yPos + tickDisplacement);
    layout->set_text(tick.second);
    const Pango::Rectangle extents = layout->get_pixel_ink_extents();
    if (_rotateUnits) {
      double y = _isSecondAxis ? (yPos + extents.get_lbearing() - 8)
                               : (yPos + extents.get_width() + 8);
      cairo->move_to(x - extents.get_descent() - extents.get_height() / 2, y);
      cairo->save();
      cairo->rotate(-M_PI * 0.5);
      layout->show_in_cairo_context(cairo);
      cairo->restore();
    } else {
      // Room is reserved of size height between the text and the axis
      double y =
          _isSecondAxis ? yPos - extents.get_height() : yPos + tickDisplacement;
      cairo->move_to(x - extents.get_width() / 2, y);
      layout->show_in_cairo_context(cairo);
    }
  }
  cairo->stroke();

  if (_drawWithDescription) drawDescription(cairo);
}

void HorizontalPlotScale::drawDescription(
    const Cairo::RefPtr<Cairo::Context>& cairo) {
  double yPos = _isSecondAxis ? _fromTop : _widgetHeight;

  cairo->save();
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_descriptionFontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  layout->set_text(_unitsCaption);
  const Pango::Rectangle extents = layout->get_pixel_logical_extents();
  double y = _isSecondAxis ? (yPos - extents.get_descent() + 5)
                           : (yPos - extents.get_height() - 5);
  cairo->move_to(Data().fromLeft + 0.3 * Data().plotWidth, y);
  layout->show_in_cairo_context(cairo);
  cairo->stroke();
  cairo->restore();

  // Base of arrow
  y = _isSecondAxis ? (yPos + extents.get_height() + 5) : (yPos - 5);
  cairo->move_to(Data().fromLeft + 0.1 * Data().plotWidth,
                 y - 0.5 * extents.get_height());
  cairo->line_to(Data().fromLeft + 0.275 * Data().plotWidth,
                 y - 0.5 * extents.get_height());
  cairo->stroke();

  // The arrow
  cairo->move_to(Data().fromLeft + 0.275 * Data().plotWidth,
                 y - 0.5 * extents.get_height());
  cairo->line_to(Data().fromLeft + 0.25 * Data().plotWidth,
                 y - 0.1 * extents.get_height());
  cairo->line_to(Data().fromLeft + 0.26 * Data().plotWidth,
                 y - 0.5 * extents.get_height());
  cairo->line_to(Data().fromLeft + 0.25 * Data().plotWidth,
                 y - 0.9 * extents.get_height());
  cairo->close_path();
  cairo->fill();
}

void HorizontalPlotScale::initializeMetrics(
    const Cairo::RefPtr<Cairo::Context>& cairo) {
  if (!Data().metricsAreInitialized) {
    Data().metricsAreInitialized = true;
    double globalPlotWidth = 0.0, globalFromLeft = 0.0, globalRightMargin = 0.0;

    for (HorizontalPlotScale* scale : *this) {
      double lPlotWidth, lFromLeft, lRightMargin;
      scale->initializeLocalMetrics(cairo, lPlotWidth, lFromLeft, lRightMargin);
      if (lPlotWidth != 0.0) {
        if (globalPlotWidth == 0.0 || globalPlotWidth > lPlotWidth)
          globalPlotWidth = lPlotWidth;
      }
      globalFromLeft = std::max(globalFromLeft, lFromLeft);
      globalRightMargin = std::max(globalRightMargin, lRightMargin);
    }
    if (Data().plotWidth != globalPlotWidth ||
        Data().fromLeft != globalFromLeft ||
        Data().rightMargin != globalRightMargin) {
      Data().plotWidth = globalPlotWidth;
      Data().fromLeft = globalFromLeft;
      Data().rightMargin = globalRightMargin;
      for (HorizontalPlotScale* scale : *this) scale->_signalLinkedRedraw();
    }
  }
}

double HorizontalPlotScale::CalculateHeight(
    const Cairo::RefPtr<Cairo::Context>& cairo) {
  std::unique_ptr<TickSet> lTickSet = _tickSet->Clone();

  double height;
  if (_drawWithDescription) {
    Glib::RefPtr<Pango::Layout> descTextLayout = Pango::Layout::create(cairo);
    Pango::FontDescription fontDescription;
    fontDescription.set_size(_descriptionFontSize * PANGO_SCALE);
    descTextLayout->set_text(_unitsCaption);
    descTextLayout->set_font_description(fontDescription);
    height = descTextLayout->get_pixel_logical_extents().get_height();
  } else {
    height = 0;
  }

  int maxTickTextHeight = 0;
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  for (size_t i = 0; i != _tickSet->Size(); ++i) {
    const Tick tick = _tickSet->GetTick(i);
    layout->set_text(tick.second);
    if (_rotateUnits) {
      maxTickTextHeight = std::max(
          maxTickTextHeight, layout->get_pixel_logical_extents().get_width());
    } else {
      maxTickTextHeight = std::max(
          maxTickTextHeight, layout->get_pixel_logical_extents().get_height());
    }
  }

  if (_rotateUnits)
    height += maxTickTextHeight + 15;
  else
    height += maxTickTextHeight + 10;
  return height;
}

void HorizontalPlotScale::initializeLocalMetrics(
    const Cairo::RefPtr<Cairo::Context>& cairo, double& plotWidth,
    double& fromLeft, double& rightMargin) {
  fromLeft = _minFromLeft;
  if (_tickSet == nullptr) {
    rightMargin = 0.0;
    plotWidth = 0.0;
  } else {
    _tickSet->Reset();
    while (!ticksFit(cairo) && _tickSet->Size() > 2) {
      _tickSet->DecreaseTicks();
    }

    if (_tickSet->Size() != 0) {
      Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
      Pango::FontDescription fontDescription;
      fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
      layout->set_font_description(fontDescription);
      const Tick lastTick = _tickSet->GetTick(_tickSet->Size() - 1);
      layout->set_text(lastTick.second);
      const int width = layout->get_pixel_logical_extents().get_width();
      double approxOversize =
          _widgetWidth * lastTick.first + width / 2 - _widgetWidth;
      rightMargin = std::max(10.0, approxOversize + 5.0);
    } else {
      rightMargin = 0.0;
    }

    plotWidth = _widgetWidth - _minFromLeft - rightMargin;
  }
}

void HorizontalPlotScale::InitializeTicks() {
  if (_isLogarithmic)
    _tickSet.reset(new LogarithmicTickSet(_tickRange[0], _tickRange[1], 25));
  else {
    switch (_axisType) {
      case AxisType::kNumeric:
        _tickSet.reset(new NumericTickSet(_tickRange[0], _tickRange[1], 25));
        break;
      case AxisType::kText:
        _tickSet.reset(new TextTickSet(_tickLabels, 100));
        break;
      case AxisType::kTime:
        _tickSet.reset(new TimeTickSet(_tickRange[0], _tickRange[1], 25));
        break;
    }
  }
  Data().metricsAreInitialized = false;
}

bool HorizontalPlotScale::ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo) {
  cairo->set_font_size(16.0);
  double prevEndX = 0.0;
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  for (unsigned i = 0; i != _tickSet->Size(); ++i) {
    const Tick tick = _tickSet->GetTick(i);
    // Use "M" to get at least an "M" of distance between axis
    layout->set_text(tick.second + "M");
    const Pango::Rectangle extents = layout->get_pixel_logical_extents();
    const double midX =
        tick.first * (Data().plotWidth - Data().fromLeft) + Data().fromLeft;
    double startX, endX;
    if (_rotateUnits) {
      startX = midX - extents.get_height() / 2;
      endX = startX + extents.get_height();
    } else {
      startX = midX - extents.get_width() / 2;
      endX = startX + extents.get_width();
    }
    if (startX < prevEndX && i != 0) return false;
    prevEndX = endX;
  }
  return true;
}
