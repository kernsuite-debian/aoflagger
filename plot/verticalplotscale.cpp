#include "verticalplotscale.h"

#include "ticksets/logarithmictickset.h"
#include "ticksets/numerictickset.h"

namespace {
const std::string kFontName = "Sans";
}

VerticalPlotScale::VerticalPlotScale()
    : _plotWidth(0.0),
      _plotHeight(0.0),
      _fromTop(0.0),
      _isSecondAxis(false),
      _width(0.0),
      _captionSize(0.0),
      _metricsAreInitialized(false),
      _alignTo(nullptr),
      _tickSet(),
      _axisType(AxisType::kNumeric),
      _tickRange({0.0, 1.0}),
      _isLogarithmic(false),
      _drawWithDescription(true),
      _unitsCaption("y"),
      _descriptionFontSize(14),
      _tickValuesFontSize(14) {}

VerticalPlotScale::~VerticalPlotScale() {}

double VerticalPlotScale::GetTickTextHeight(
    const Cairo::RefPtr<Cairo::Context>& cairo) {
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  layout->set_font_description(fontDescription);
  layout->set_text("M");
  return layout->get_pixel_logical_extents().get_height();
}

double VerticalPlotScale::UnitToAxis(double unitValue) const {
  return _tickSet ? _tickSet->UnitToAxis(unitValue) : 0.0;
}

double VerticalPlotScale::AxisToUnit(double axisValue) const {
  return _tickSet ? _tickSet->AxisToUnit(axisValue) : 0.0;
}

void VerticalPlotScale::Draw(const Cairo::RefPtr<Cairo::Context>& cairo,
                             double offsetX, double offsetY) {
  offsetY += _fromTop;
  initializeMetrics(cairo);
  cairo->set_source_rgb(0.0, 0.0, 0.0);
  double x = _isSecondAxis ? offsetX : _width + offsetX;
  double tickX = _isSecondAxis ? 3 : -3;
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  for (unsigned i = 0; i != _tickSet->Size(); ++i) {
    const Tick tick = _tickSet->GetTick(i);
    double y = getTickYPosition(tick);
    cairo->move_to(x + tickX, y + offsetY);
    cairo->line_to(x, y + offsetY);
    layout->set_text(tick.second);
    const Pango::Rectangle extents = layout->get_pixel_logical_extents();
    double textX;
    if (_isSecondAxis)
      textX = 8.0;
    else
      textX = -extents.get_width() - 8.0;
    cairo->move_to(x + textX, y - extents.get_height() / 2 -
                                  extents.get_ascent() + offsetY);
    layout->show_in_cairo_context(cairo);
  }
  cairo->stroke();

  if (_drawWithDescription) drawDescription(cairo, offsetX, offsetY);
}

void VerticalPlotScale::drawDescription(
    const Cairo::RefPtr<Cairo::Context>& cairo, double offsetX,
    double offsetY) {
  cairo->save();
  Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
  Pango::FontDescription fontDescription;
  fontDescription.set_size(_descriptionFontSize * PANGO_SCALE);
  layout->set_font_description(fontDescription);
  layout->set_text(_unitsCaption);
  const Pango::Rectangle extents = layout->get_pixel_logical_extents();
  double x = _isSecondAxis ? offsetX + 2 + _width - _captionSize : offsetX + 2;
  cairo->translate(x - extents.get_ascent(), offsetY + 0.7 * _plotHeight);
  cairo->rotate(M_PI * 1.5);
  cairo->move_to(0.0, 0.0);
  layout->show_in_cairo_context(cairo);
  cairo->stroke();
  cairo->restore();

  // Base of arrow
  cairo->move_to(x + extents.get_height() / 2.0, offsetY + _plotHeight * 0.9);
  cairo->line_to(x + extents.get_height() / 2.0, offsetY + _plotHeight * 0.725);
  cairo->stroke();

  // The arrow
  cairo->move_to(x + extents.get_height() / 2.0, offsetY + _plotHeight * 0.725);
  cairo->line_to(x + 0.1 * extents.get_height(), offsetY + _plotHeight * 0.75);
  cairo->line_to(x + 0.5 * extents.get_height(), offsetY + _plotHeight * 0.74);
  cairo->line_to(x + 0.9 * extents.get_height(), offsetY + _plotHeight * 0.75);
  cairo->close_path();
  cairo->fill();
}

void VerticalPlotScale::initializeMetrics(
    const Cairo::RefPtr<Cairo::Context>& cairo) {
  if (!_metricsAreInitialized) {
    if (_alignTo != nullptr) {
      _alignTo->initializeMetrics(cairo);
      _plotHeight = _alignTo->_plotHeight;
      _fromTop = _alignTo->_fromTop;
    }

    if (_tickSet == nullptr) {
      _width = 0.0;
    } else {
      _tickSet->Reset();
      while (!ticksFit(cairo) && _tickSet->Size() > 2) {
        _tickSet->DecreaseTicks();
      }
      Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
      Pango::FontDescription fontDescription;
      fontDescription.set_size(_tickValuesFontSize * PANGO_SCALE);
      layout->set_font_description(fontDescription);
      int maxWidth = 0;
      for (unsigned i = 0; i != _tickSet->Size(); ++i) {
        Tick tick = _tickSet->GetTick(i);
        layout->set_text(tick.second);
        maxWidth =
            std::max(maxWidth, layout->get_pixel_logical_extents().get_width());
      }
      _width = maxWidth + 10;
    }
    if (_drawWithDescription) {
      Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(cairo);
      Pango::FontDescription fontDescription;
      fontDescription.set_size(_descriptionFontSize * PANGO_SCALE);
      layout->set_font_description(fontDescription);
      layout->set_text(_unitsCaption);
      _captionSize = layout->get_pixel_logical_extents().get_height();
      _width += _captionSize;
    } else {
      _captionSize = 0.0;
    }
    _metricsAreInitialized = true;
  }
}

void VerticalPlotScale::InitializeTicks() {
  if (_isLogarithmic)
    _tickSet.reset(new LogarithmicTickSet(_tickRange[0], _tickRange[1], 25));
  else
    _tickSet.reset(new NumericTickSet(_tickRange[0], _tickRange[1], 25));
  _metricsAreInitialized = false;
}

double VerticalPlotScale::getTickYPosition(const Tick& tick) {
  return (1.0 - tick.first) * _plotHeight;
}

bool VerticalPlotScale::ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo) {
  cairo->set_font_size(16.0);
  double prevTopY = _plotHeight * 2.0;
  for (unsigned i = 0; i != _tickSet->Size(); ++i) {
    const Tick tick = _tickSet->GetTick(i);
    Cairo::TextExtents extents;
    cairo->get_text_extents(tick.second, extents);
    // we want a distance of at least one x height between the text, hence
    // height
    const double bottomY = getTickYPosition(tick) + extents.height,
                 topY = bottomY - extents.height * 2;
    if (bottomY > prevTopY) return false;
    prevTopY = topY;
  }
  return true;
}
