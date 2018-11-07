#include "legend.h"

#include "plot2d.h"
#include "plot2dpointset.h"

Legend::Legend() :
	_sizeOfM(0.0), _xBearing(0.0), _yBearing(0.0), _textAdvance(0.0),
	_x(0.0), _y(0.0), _width(0.0), _height(0.0)
{
}

void Legend::Initialize(Cairo::RefPtr<Cairo::Context>& cairo, const Plot2D& plot)
{
	_width = 0.0;
	_height = 0.0;
	_textAdvance = 0.0;
	cairo->set_font_size(14);
	Cairo::TextExtents extents;
	cairo->get_text_extents("M", extents);
	_sizeOfM = extents.width;
	for(size_t i=0; i!=plot.PointSetCount(); ++i)
	{
		const Plot2DPointSet& pointSet = plot.GetPointSet(i);
		const std::string &label = pointSet.Label();
		cairo->get_text_extents(label, extents);
		_width = std::max(_width, extents.width - extents.x_bearing);
		_textAdvance = std::max(_textAdvance, extents.height);
		if(i == 0)
		{
			_yBearing = extents.y_bearing;
		}
	}
	_width += _sizeOfM*2.8;
	_height = _textAdvance * (0.4 + plot.PointSetCount());
}

void Legend::Draw(Cairo::RefPtr<Cairo::Context>& cairo, const Plot2D& plot) const
{
	cairo->rectangle(_x, _y, _width, _height);
	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->stroke_preserve();
	cairo->set_source_rgba(0.9, 0.9, 1.0, 0.5);
	cairo->fill();
	const double symbolLeft = _x + _sizeOfM*0.4;
	const double textLeft = _x + _sizeOfM*1.8 - _xBearing;
	double curY = _y - _yBearing + 0.2 * _textAdvance;
	cairo->set_font_size(14);
	cairo->set_line_width(2);
	for(size_t i=0; i!=plot.PointSetCount(); ++i)
	{
		const Plot2DPointSet& pointSet = plot.GetPointSet(i);
		const std::string& label = pointSet.Label();
		cairo->set_source_rgb(0.0, 0.0, 0.0);
		cairo->move_to(textLeft, curY);
		cairo->show_text(label);
		auto color = pointSet.GetColor();
		cairo->set_source_rgba(color.r, color.g, color.b, color.a);
		switch(pointSet.DrawingStyle())
		{
			case Plot2DPointSet::DrawColumns:
				cairo->rectangle(symbolLeft+0.1*_sizeOfM, curY - _textAdvance*0.9, 0.9*_sizeOfM, 0.9*_sizeOfM);
				cairo->fill();
				break;
			case Plot2DPointSet::DrawLines:
				cairo->move_to(symbolLeft, curY - _textAdvance*0.5);
				cairo->rel_line_to(_sizeOfM, 0.0);
				cairo->stroke();
				break;
			case Plot2DPointSet::DrawPoints:
				cairo->move_to(symbolLeft + 2.0 + _sizeOfM*0.5, curY - _textAdvance-0.5);
				cairo->arc(symbolLeft + _sizeOfM*0.5, curY - _textAdvance*0.5, 2.0, 0.0, 2.0*M_PI);
				cairo->fill();
				break;
		}
		curY += _textAdvance;
	}
}

