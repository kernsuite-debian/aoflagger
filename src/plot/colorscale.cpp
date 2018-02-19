#include "colorscale.h"

const double ColorScale::BAR_WIDTH = 15.0;

ColorScale::ColorScale()
: _plotWidth(0.0), _plotHeight(0.0), _topMargin(0.0),
	_scaleWidth(0.0), _width(0.0), 
	_textOnLeft(false),
	_min(0.0), _max(0.0),
	_verticalPlotScale(),
	_isLogaritmic(false)
{
	_verticalPlotScale.SetUnitsCaption("z");
}

void ColorScale::initWidth(Cairo::RefPtr<Cairo::Context> cairo)
{
	if(_width == 0.0)
	{
		_textHeight = _verticalPlotScale.GetTextHeight(cairo);
		const double scaleHeight = _plotHeight - 2.0*_textHeight - _topMargin;
		_verticalPlotScale.SetPlotDimensions(_plotWidth, scaleHeight, 0.0, !_textOnLeft);
		_scaleWidth = _verticalPlotScale.GetWidth(cairo);
		_width = _scaleWidth + BAR_WIDTH;
	}
}

void ColorScale::Draw(Cairo::RefPtr<Cairo::Context> cairo)
{
	initWidth(cairo);
	const double textHeight = _verticalPlotScale.GetTextHeight(cairo);
	const double scaleTop = _topMargin + textHeight;
	const double scaleHeight = _plotHeight - 2.0*textHeight - _topMargin;
	ColorValue backValue;
	if(!_colorValues.empty())
	{
		backValue = _colorValues.begin()->second;
	} else {
		backValue.red = 1.0;
		backValue.green = 1.0;
		backValue.blue = 1.0;
	}
	double barX = _textOnLeft ? (_plotWidth - _width + _scaleWidth) : (_plotWidth - _width);
	cairo->rectangle(barX, scaleTop, BAR_WIDTH, scaleHeight);
	cairo->set_source_rgb(backValue.red, backValue.green, backValue.blue);
	cairo->fill();
	
	for(std::map<double, ColorValue>::const_iterator i=_colorValues.begin();
		i!=_colorValues.end();++i)
	{
		double val;
		if(_isLogaritmic)
		{
			const double minLog10 = log10(_min);
			if(i->first <= 0.0)
				val = 0.0;
			else
				val = (log10(i->first) - minLog10) / (log10(_max) - minLog10);
		} else {
			val = (i->first - _min) / (_max - _min);
			if(val < 0.0) val = 0.0;
			if(val > 1.0) val = 1.0;
		}
		const double height = scaleHeight * (1.0 - val);
		const ColorValue &color = i->second;
		cairo->set_source_rgb(color.red, color.green, color.blue);
		cairo->rectangle(barX, scaleTop, BAR_WIDTH, height);
		cairo->fill();
	}
	
	cairo->rectangle(barX, scaleTop, BAR_WIDTH, scaleHeight);
	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->stroke();
	double scaleX = _textOnLeft  ? (_plotWidth - _width) : (_plotWidth - _width + BAR_WIDTH);
	_verticalPlotScale.Draw(cairo, scaleX, _topMargin + textHeight);
}
