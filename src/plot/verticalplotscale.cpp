#include "verticalplotscale.h"
#include "tickset.h"

VerticalPlotScale::VerticalPlotScale() :
	_plotWidth(0.0),
	_plotHeight(0.0),
	_isSecondAxis(false),
	_metricsAreInitialized(false), _width(0.0),
	_captionSize(0.0),
	_tickSet(), _isLogarithmic(false), _drawWithDescription(true),
	_unitsCaption("y"),
	_descriptionFontSize(14), _tickValuesFontSize(14)
{ }

VerticalPlotScale::~VerticalPlotScale()
{ }

double VerticalPlotScale::GetTextHeight(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	Cairo::TextExtents extents;
	cairo->get_text_extents("M", extents);
	return extents.height;
}

double VerticalPlotScale::UnitToAxis(double unitValue) const
{
	return _tickSet->UnitToAxis(unitValue);
}

double VerticalPlotScale::AxisToUnit(double axisValue) const
{
	return _tickSet->AxisToUnit(axisValue);
}

void VerticalPlotScale::Draw(const Cairo::RefPtr<Cairo::Context>& cairo, double offsetX, double offsetY)
{
	offsetY += _fromTop;
	initializeMetrics(cairo);
	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->set_font_size(_tickValuesFontSize);
	double x = _isSecondAxis ? offsetX : _width + offsetX;
	double tickX = _isSecondAxis ? 3 : -3;
	for(unsigned i=0;i!=_tickSet->Size();++i)
	{
		const Tick tick = _tickSet->GetTick(i);
		double y = getTickYPosition(tick);
		cairo->move_to(x + tickX, y + offsetY);
		cairo->line_to(x, y + offsetY);
		Cairo::TextExtents extents;
		cairo->get_text_extents(tick.second, extents);
		double textX;
		if(_isSecondAxis)
			textX = 8.0;
		else
			textX = -extents.width - 8.0;
		cairo->move_to(x + textX, y - extents.height/2 - extents.y_bearing + offsetY);
		cairo->show_text(tick.second);
	}
	cairo->stroke();
	
	if(_drawWithDescription)
		drawUnits(cairo, offsetX, offsetY);
}

void VerticalPlotScale::drawUnits(const Cairo::RefPtr<Cairo::Context>& cairo, double offsetX, double offsetY)
{
	cairo->save();
	cairo->set_font_size(_descriptionFontSize);
	Cairo::TextExtents extents;
	cairo->get_text_extents(_unitsCaption, extents);
	double x = _isSecondAxis ? offsetX+2+_width-_captionSize : offsetX+2;
	cairo->translate(x - extents.y_bearing, offsetY + 0.7 * _plotHeight);
	cairo->rotate(M_PI * 1.5);
	cairo->move_to(0.0, 0.0);
	cairo->show_text(_unitsCaption);
	cairo->stroke();
	cairo->restore();

	// Base of arrow
	cairo->move_to(x + extents.height/2.0, offsetY + _plotHeight * 0.9);
	cairo->line_to(x + extents.height/2.0, offsetY + _plotHeight * 0.725);
	cairo->stroke();

	// The arrow
	cairo->move_to(x + extents.height/2.0, offsetY + _plotHeight * 0.725);
	cairo->line_to(x + 0.1*extents.height, offsetY + _plotHeight * 0.75);
	cairo->line_to(x + 0.5*extents.height, offsetY + _plotHeight * 0.74);
	cairo->line_to(x + 0.9*extents.height, offsetY + _plotHeight * 0.75);
	cairo->close_path();
	cairo->fill();
}

void VerticalPlotScale::initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	if(!_metricsAreInitialized)
	{
		if(_tickSet != nullptr)
		{
			_tickSet->Reset();
			while(!ticksFit(cairo) && _tickSet->Size() > 2)
			{
				_tickSet->DecreaseTicks();
			}
			cairo->set_font_size(_tickValuesFontSize);
			double maxWidth = 0;
			for(unsigned i=0;i!=_tickSet->Size();++i)
			{
				Tick tick = _tickSet->GetTick(i);
				Cairo::TextExtents extents;
				cairo->get_text_extents(tick.second, extents);
				if(maxWidth < extents.width)
					maxWidth = extents.width;
			}
			_width = maxWidth + 10;
			if(_drawWithDescription)
			{
				cairo->set_font_size(_descriptionFontSize);
				Cairo::TextExtents extents;
				cairo->get_text_extents(_unitsCaption, extents);
				_captionSize = extents.height;
				_width += _captionSize;
			}
			else {
				_captionSize = 0.0;
			}
			_metricsAreInitialized = true;
		}
	}
} 

void VerticalPlotScale::InitializeNumericTicks(double min, double max)
{
	_tickSet.reset(new NumericTickSet(min, max, 25));
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

void VerticalPlotScale::InitializeLogarithmicTicks(double min, double max)
{
	_tickSet.reset(new LogarithmicTickSet(min, max, 25));
	_isLogarithmic = true;
	_metricsAreInitialized = false;
}

double VerticalPlotScale::getTickYPosition(const Tick& tick)
{
	return (1.0-tick.first) * _plotHeight;
}

bool VerticalPlotScale::ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	cairo->set_font_size(16.0);
	double prevTopY = _plotHeight*2.0;
	for(unsigned i=0;i!=_tickSet->Size();++i)
	{
		const Tick tick = _tickSet->GetTick(i);
		Cairo::TextExtents extents;
		cairo->get_text_extents(tick.second, extents);
		// we want a distance of at least one x height between the text, hence height
		const double
			bottomY = getTickYPosition(tick) + extents.height,
			topY = bottomY - extents.height*2;
		if(bottomY > prevTopY)
			return false;
		prevTopY = topY;
	}
	return true;
}
