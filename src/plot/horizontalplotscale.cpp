#include "horizontalplotscale.h"

#include "tickset.h"

HorizontalPlotScale::HorizontalPlotScale() :
	_plotWidth(0.0), _plotHeight(0.0),
	_fromLeft(0.0), _fromTop(0.0), 
	_metricsAreInitialized(false), _height(0.0), _rightMargin(0.0),
	_drawWithDescription(true), _unitsCaption("x"),
	_descriptionFontSize(14),
	_tickValuesFontSize(14),
	_rotateUnits(false), _isLogarithmic(false)
{
}

HorizontalPlotScale::~HorizontalPlotScale()
{ }

double HorizontalPlotScale::GetHeight(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	initializeMetrics(cairo);
	return _height;
}

double HorizontalPlotScale::GetRightMargin(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	initializeMetrics(cairo);
	return _rightMargin;
}

double HorizontalPlotScale::UnitToAxis(double unitValue) const
{
	return _tickSet->UnitToAxis(unitValue);
}

double HorizontalPlotScale::AxisToUnit(double axisValue) const
{
	return _tickSet->AxisToUnit(axisValue);
}

void HorizontalPlotScale::Draw(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	initializeMetrics(cairo);
	cairo->set_source_rgb(0.0, 0.0, 0.0);
	cairo->set_font_size(_tickValuesFontSize);
	double tickDisplacement = _isSecondAxis ? -3.0 : 3.0;
	// Y position of x-axis line
	double yPos = _isSecondAxis ? _fromTop + _height : _fromTop + _plotHeight;
	for(unsigned i=0;i!=_tickSet->Size();++i)
	{
		const Tick tick = _tickSet->GetTick(i);
		double x = tick.first * (_plotWidth - _fromLeft) + _fromLeft;
		cairo->move_to(x, yPos);
		cairo->line_to(x, yPos + tickDisplacement);
		Cairo::TextExtents extents;
		cairo->get_text_extents(tick.second, extents);
		if(_rotateUnits)
		{
			double y = _isSecondAxis ? (yPos + extents.x_bearing - 8) : (yPos + extents.width + 8);
			cairo->move_to(x - extents.y_bearing - extents.height/2, y);
			cairo->save();
			cairo->rotate(-M_PI*0.5);
			cairo->show_text(tick.second);
			cairo->restore();
		}
		else
		{
			// Room is reserved of size height between the text and the axis
			double y = _isSecondAxis ? yPos - extents.height : (yPos - extents.y_bearing + extents.height);
			cairo->move_to(x - extents.width/2, y);
			cairo->show_text(tick.second);
		}
	}
	cairo->stroke();
	
	if(_drawWithDescription)
		drawDescription(cairo);
}

void HorizontalPlotScale::drawDescription(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	double yPos = _isSecondAxis ? _fromTop : _fromTop + _plotHeight + _height;
	
	cairo->save();
	cairo->set_font_size(_descriptionFontSize);
	Cairo::TextExtents extents;
	cairo->get_text_extents(_unitsCaption, extents);
	double y = _isSecondAxis ? (yPos - extents.y_bearing + 5) : (yPos - extents.y_bearing - extents.height - 5);
	cairo->move_to(_fromLeft + 0.3 * _plotWidth, y);
	cairo->show_text(_unitsCaption);
	cairo->stroke();
	cairo->restore();

	// Base of arrow
	y = _isSecondAxis ? (yPos + extents.height + 5) : (yPos - 5);
	cairo->move_to(_fromLeft + 0.1 * _plotWidth, y - 0.5*extents.height);
	cairo->line_to(_fromLeft + 0.275 * _plotWidth, y - 0.5*extents.height);
	cairo->stroke();

	// The arrow
	cairo->move_to(_fromLeft + 0.275 * _plotWidth, y - 0.5*extents.height);
	cairo->line_to(_fromLeft + 0.25 * _plotWidth, y - 0.1*extents.height);
	cairo->line_to(_fromLeft + 0.26 * _plotWidth, y - 0.5*extents.height);
	cairo->line_to(_fromLeft + 0.25 * _plotWidth, y - 0.9*extents.height);
	cairo->close_path();
	cairo->fill();
}

void HorizontalPlotScale::initializeMetrics(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	if(!_metricsAreInitialized)
	{
		if(_tickSet == nullptr)
		{
			_rightMargin = 0.0;
			_height = 0.0;
		}
		else {
			_tickSet->Reset();
			while(!ticksFit(cairo) && _tickSet->Size()>2)
			{
				_tickSet->DecreaseTicks();
			}
			cairo->set_font_size(_tickValuesFontSize);
			double maxHeight = 0;
			for(unsigned i=0;i!=_tickSet->Size();++i)
			{
				const Tick tick = _tickSet->GetTick(i);
				Cairo::TextExtents extents;
				cairo->get_text_extents(tick.second, extents);
				if(_rotateUnits)
				{
					if(maxHeight < extents.width)
						maxHeight = extents.width;
				} else {
					if(maxHeight < extents.height)
						maxHeight = extents.height;
				}
			}
			if(_rotateUnits)
				_height = maxHeight + 15;
			else
				_height = maxHeight*2 + 10;
			if(_drawWithDescription)
			{
				cairo->set_font_size(_descriptionFontSize);
				Cairo::TextExtents extents;
				cairo->get_text_extents(_unitsCaption, extents);
				_height += extents.height;
			}
			
			if(_tickSet->Size() != 0)
			{
				cairo->set_font_size(_tickValuesFontSize);
				Cairo::TextExtents extents;
				cairo->get_text_extents(_tickSet->GetTick(_tickSet->Size()-1).second, extents);
				
				/// TODO this is TOO MUCH, caption is often not in the rightmost position.
				_rightMargin = extents.width/2+5 > 10 ? extents.width/2+5 : 10;
			} else {
				_rightMargin = 0.0;
			}
			
			_metricsAreInitialized = true;
		}
	}
} 

void HorizontalPlotScale::InitializeNumericTicks(double min, double max)
{
	_tickSet.reset(new NumericTickSet(min, max, 25));
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

void HorizontalPlotScale::InitializeLogarithmicTicks(double min, double max)
{
	_tickSet.reset(new LogarithmicTickSet(min, max, 25));
	_isLogarithmic = true;
	_metricsAreInitialized = false;
}

void HorizontalPlotScale::InitializeTimeTicks(double timeMin, double timeMax)
{
	_tickSet.reset(new TimeTickSet(timeMin, timeMax, 25));
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

void HorizontalPlotScale::InitializeTextTicks(const std::vector<std::string>& labels)
{
	_tickSet.reset(new TextTickSet(labels, 100));
	_isLogarithmic = false;
	_metricsAreInitialized = false;
}

bool HorizontalPlotScale::ticksFit(const Cairo::RefPtr<Cairo::Context>& cairo)
{
	cairo->set_font_size(16.0);
	double prevEndX = 0.0;
	for(unsigned i=0;i!=_tickSet->Size();++i)
	{
		const Tick tick = _tickSet->GetTick(i);
		Cairo::TextExtents extents;
		cairo->get_text_extents(tick.second + "M", extents);
		const double
			midX = tick.first * (_plotWidth - _fromLeft) + _fromLeft;
		double startX, endX;
		if(_rotateUnits)
		{
			// Use "M" to get at least an "M" of distance between axis
			startX = midX - extents.height/2,
			endX = startX + extents.height;
		} else
		{
			// Use "M" to get at least an "M" of distance between ticks
			cairo->get_text_extents(tick.second + "M", extents);
			startX = midX - extents.width/2,
			endX = startX + extents.width;
		}
		if(startX < prevEndX && i!=0)
			return false;
		prevEndX = endX;
	}
	return true;
}
