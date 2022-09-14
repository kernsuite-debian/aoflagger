#include "colorscale.h"

const double ColorScale::BAR_WIDTH = 15.0;

ColorScale::ColorScale()
    : _plotWidth(0.0),
      _plotHeight(0.0),
      _topMargin(0.0),
      _scaleWidth(0.0),
      _width(0.0),
      _textOnLeft(false),
      _verticalPlotScale() {
  _verticalPlotScale.SetUnitsCaption("z");
}

void ColorScale::initWidth(const Cairo::RefPtr<Cairo::Context>& cairo) {
  if (_width == 0.0) {
    _textHeight = _verticalPlotScale.GetTickTextHeight(cairo);
    const double scaleHeight = _plotHeight - 2.0 * _textHeight - _topMargin;
    _verticalPlotScale.SetPlotDimensions(_plotWidth, scaleHeight, 0.0,
                                         !_textOnLeft);
    _scaleWidth = _verticalPlotScale.GetWidth(cairo);
    _width = _scaleWidth + BAR_WIDTH;
  }
}

void ColorScale::Draw(const Cairo::RefPtr<Cairo::Context>& cairo) {
  initWidth(cairo);
  const double textHeight = _verticalPlotScale.GetTickTextHeight(cairo);
  const double scaleTop = _topMargin + textHeight;
  const double scaleHeight = _plotHeight - 2.0 * textHeight - _topMargin;
  ColorValue backValue;
  if (!_colorValues.empty()) {
    backValue = _colorValues.begin()->second;
  } else {
    backValue.red = 1.0;
    backValue.green = 1.0;
    backValue.blue = 1.0;
  }
  double barX =
      _textOnLeft ? (_plotWidth - _width + _scaleWidth) : (_plotWidth - _width);
  cairo->rectangle(barX, scaleTop, BAR_WIDTH, scaleHeight);
  cairo->set_source_rgb(backValue.red, backValue.green, backValue.blue);
  cairo->fill();

  const double min = _verticalPlotScale.GetTickRangeMin();
  const double max = _verticalPlotScale.GetTickRangeMax();
  for (std::pair<const double, ColorValue>& item : _colorValues) {
    double val;
    if (_verticalPlotScale.IsLogarithmic()) {
      const double minLog10 = std::log10(min);
      if (item.first <= 0.0)
        val = 0.0;
      else
        val =
            (std::log10(item.first) - minLog10) / (std::log10(max) - minLog10);
    } else {
      val = (item.first - min) / (max - min);
      if (val < 0.0) val = 0.0;
      if (val > 1.0) val = 1.0;
    }
    const double height = scaleHeight * (1.0 - val);
    const ColorValue& color = item.second;
    cairo->set_source_rgb(color.red, color.green, color.blue);
    cairo->rectangle(barX, scaleTop, BAR_WIDTH, height);
    cairo->fill();
  }

  cairo->rectangle(barX, scaleTop, BAR_WIDTH, scaleHeight);
  cairo->set_source_rgb(0.0, 0.0, 0.0);
  cairo->stroke();
  double scaleX =
      _textOnLeft ? (_plotWidth - _width) : (_plotWidth - _width + BAR_WIDTH);
  _verticalPlotScale.Draw(cairo, scaleX, _topMargin + textHeight);
}
