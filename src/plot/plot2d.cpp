#include "plot2d.h"

#include <iostream>

#include "../util/logger.h"

Plot2D::Plot2D() :
	_width(640),
	_height(480),
	_topMargin(0.0),
	_logarithmicXAxis(false),
	_logarithmicYAxis(false),
	_showAxes(true),
	_showAxisDescriptions(true),
	_specifiedMinX(0.0), _specifiedMaxX(0.0), _specifiedMinY(0.0), _specifiedMaxY(0.0),
	_hRangeDetermination(MinMaxRange),
	_vRangeDetermination(MinMaxRange)
{
}

Plot2D::~Plot2D()
{
	Clear();
}

void Plot2D::Clear()
{
	_pointSets.clear();
	_system.Clear();
}

void Plot2D::Render(Gtk::DrawingArea &drawingArea)
{
	Glib::RefPtr<Gdk::Window> window = drawingArea.get_window();
	if(window)
	{
		Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();

		Gtk::Allocation allocation = drawingArea.get_allocation();
		_width = allocation.get_width();
		_height = allocation.get_height();
		render(cr);
	}
}

void Plot2D::SavePdf(const std::string &filename)
{
	Cairo::RefPtr<Cairo::PdfSurface> surface = Cairo::PdfSurface::create(filename, _width, _height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	Logger::Debug << "Saving PDF of " << _width << " x " << _height << "\n";
	render(cairo);
	cairo->show_page();
	surface->finish();
}

void Plot2D::SaveSvg(const std::string &filename)
{
	Cairo::RefPtr<Cairo::SvgSurface> surface = Cairo::SvgSurface::create(filename, _width, _height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	Logger::Debug << "Saving SVG of " << _width << " x " << _height << "\n";
	render(cairo);
	cairo->show_page();
	surface->finish();
}

void Plot2D::SavePng(const std::string &filename)
{
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, _width, _height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	Logger::Debug << "Saving PNG of " << _width << " x " << _height << "\n";
	render(cairo);
	surface->write_to_png(filename);
}

void Plot2D::render(Cairo::RefPtr<Cairo::Context>& cr)
{
	_system.Clear();
	for(std::unique_ptr<Plot2DPointSet>& set : _pointSets)
		_system.AddToSystem(*set);
	
	cr->set_line_width(2);

	cr->set_source_rgba(1, 1, 1, 1);
	cr->paint();
	cr->fill();

	if(!_pointSets.empty())
	{
		Plot2DPointSet &refPointSet = **_pointSets.begin();
		
		double verticalScaleWidth, horiScaleHeight;
	
		_topMargin = 0.0;
		
		double titleHeight = 0.0;
		if(!_title.Text().empty())
		{
			_title.SetPlotDimensions(_width, _height, _topMargin);
			titleHeight = _title.GetHeight(cr);
		}
		
		if(_showAxes)
			_topMargin += std::max(10.0, titleHeight);
		else
			_topMargin += titleHeight;
		
		if(_showAxes)
		{
			_horizontalScale.SetDrawWithDescription(_showAxisDescriptions);
			_horizontalScale.SetRotateUnits(refPointSet.RotateUnits());
			if(refPointSet.HasTickLabels())
				_horizontalScale.InitializeTextTicks(refPointSet.TickLabels());
			else if(refPointSet.XIsTime())
				_horizontalScale.InitializeTimeTicks(_system.XRangeMin(refPointSet), _system.XRangeMax(refPointSet));
			else if(_logarithmicXAxis) {
				auto range = RangePositiveX();
				_horizontalScale.InitializeLogarithmicTicks(range.first, range.second);
			}
			else {
				auto range = RangeX();
				_horizontalScale.InitializeNumericTicks(range.first, range.second);
			}
			_horizontalScale.SetUnitsCaption(_customHAxisDescription.empty() ? refPointSet.XUnits() : _customHAxisDescription);
			_horizontalScale.SetPlotDimensions(_width, _height, 0.0, _topMargin, false);
			horiScaleHeight = _horizontalScale.GetHeight(cr);
			
			double rightMargin = _horizontalScale.GetRightMargin(cr);
			_verticalScale.SetDrawWithDescription(_showAxisDescriptions);
			if(_logarithmicYAxis) {
				auto range = RangePositiveY();
				_verticalScale.InitializeLogarithmicTicks(range.first, range.second);
			}
			else {
				auto range = RangeY();
				_verticalScale.InitializeNumericTicks(range.first, range.second);
			}
			_verticalScale.SetUnitsCaption(_customVAxisDescription.empty() ? refPointSet.YUnits() : _customVAxisDescription);
			_verticalScale.SetPlotDimensions(_width - rightMargin, _height - horiScaleHeight - _topMargin, _topMargin, false);

			verticalScaleWidth =  _verticalScale.GetWidth(cr);
			_horizontalScale.SetPlotDimensions(_width - rightMargin, _height - horiScaleHeight, verticalScaleWidth, 0.0, false);
		}
		else {
			verticalScaleWidth = 0.0;
			horiScaleHeight = 0.0;
		}
		
		for(size_t i=0; i!=_pointSets.size(); ++i)
		{
			auto c = _pointSets[i]->GetColor();
			cr->set_source_rgba(c.r, c.g, c.b, c.a);
			render(cr, *_pointSets[i]);
		}
		
		double rightMargin;
		if(_showAxes)
		{
			_horizontalScale.Draw(cr);
			_verticalScale.Draw(cr, 0.0, 0.0);
			rightMargin = _horizontalScale.GetRightMargin(cr);
		} else {
			rightMargin = 0.0;
		}
		_legend.Initialize(cr, *this);
		// For top left:
		//_legend.SetPosition(verticalScaleWidth+10, _topMargin+10);
		_legend.SetPosition(_width - rightMargin-10 - _legend.Width(), _topMargin+10);
		
		if(!_title.Text().empty())
		{
			_title.Draw(cr);
		}
		
		cr->set_source_rgb(0.0, 0.0, 0.0);
		cr->rectangle(verticalScaleWidth, _topMargin, _width - verticalScaleWidth - rightMargin, _height - horiScaleHeight - _topMargin);
		cr->stroke();
		
		_legend.Draw(cr, *this);
	}
}

void Plot2D::render(Cairo::RefPtr<Cairo::Context>& cr, Plot2DPointSet& pointSet)
{
	pointSet.Sort();

	auto xRange = _logarithmicXAxis ? RangePositiveX() : RangeX();
	auto yRange = _logarithmicYAxis ? RangePositiveY() : RangeY();
	double xLeft = xRange.first, xRight = xRange.second;
	double yMin = yRange.first, yMax = yRange.second;
		
	if(!std::isfinite(xLeft) || !std::isfinite(xRight))
	{
		xLeft = -1;
		xRight = 1;
	}
	else if(xLeft == xRight)
	{
		xLeft -= 1;
		xRight += 1;
	}
	if(!std::isfinite(yMin) || !std::isfinite(yMax))
	{
		yMin = -1;
		yMax = 1;
	}
	else if(yMin == yMax)
	{
		yMin -= 1;
		yMax += 1;
	}

	const double rightMargin = _showAxes ? _horizontalScale.GetRightMargin(cr) : 0.0;
	const double bottomMargin = _showAxes ? _horizontalScale.GetHeight(cr) : 0.0;
	const double plotLeftMargin = _showAxes ? _verticalScale.GetWidth(cr) : 0.0;
	
	const double plotWidth = _width - rightMargin - plotLeftMargin;
	const double plotHeight = _height - bottomMargin - _topMargin;
	
	cr->rectangle(plotLeftMargin, _topMargin, plotWidth, plotHeight);
	cr->clip();
	
	double
		minXLog10 = log10(xLeft),
		maxXLog10 = log10(xRight),
		minYLog10 = log10(yMin),
		maxYLog10 = log10(yMax);

	bool hasPrevPoint = false;
	
	unsigned iterationCount = pointSet.Size();
	bool isNextPointRequired = pointSet.DrawingStyle() == Plot2DPointSet::DrawLines && iterationCount!=0;
	if(isNextPointRequired)
		--iterationCount;

	for(size_t i=0;i<iterationCount;++i)
	{
		double
			x1Val, x2Val,
			y1Val, y2Val;
			
		double
			px = pointSet.GetX(i),
			py = pointSet.GetY(i);
		if(_logarithmicXAxis)
		{
			if(px <= 0.0)
				x1Val = 0.0;
			else
				x1Val = (log10(px) - minXLog10) / (maxXLog10 - minXLog10);
		}
		else {
			x1Val = (px - xLeft) / (xRight - xLeft);
		}
		
		if(_logarithmicYAxis)
		{
			if(py <= 0.0)
				y1Val = 0.0;
			else
				y1Val = (log10(py) - minYLog10) / (maxYLog10 - minYLog10);
		} else {
			y1Val = (py - yMin) / (yMax - yMin);
		}
		if(y1Val < 0.0) y1Val = 0.0;
		if(y1Val > 1.0) y1Val = 1.0;
		
		if(isNextPointRequired)
		{
			double
				pxn = pointSet.GetX(i+1),
				pyn = pointSet.GetY(i+1);
			if(_logarithmicXAxis)
			{
				if(pxn <= 0.0)
					x2Val = 0.0;
				else
					x2Val = (log10(pxn) - minXLog10) / (maxXLog10 - minXLog10);
			}
			else {
				x2Val = (pxn - xLeft) / (xRight - xLeft);
			}
			
			if(_logarithmicYAxis)
			{
				if(pyn <= 0.0)
					y2Val = 0.0;
				else
					y2Val = (log10(pyn) - minYLog10) / (maxYLog10 - minYLog10);
			}
			else {
				y2Val = (pyn - yMin) / (yMax - yMin);
			}
			
			if(y2Val < 0.0) y2Val = 0.0;
			if(y2Val > 1.0) y2Val = 1.0;
		}
		else {
			x2Val = 0.0;
			y2Val = 0.0;
		}
		
		double
			x1 = x1Val * plotWidth + plotLeftMargin,
			x2 = x2Val * plotWidth + plotLeftMargin,
			y1 = (1.0 - y1Val) * plotHeight + _topMargin,
			y2 = (1.0 - y2Val) * plotHeight + _topMargin;

		if(std::isfinite(x1) && std::isfinite(y1))
		{
			switch(pointSet.DrawingStyle())
			{
				case Plot2DPointSet::DrawLines:
					if(std::isfinite(x2) && std::isfinite(y2))
					{
						if(!hasPrevPoint)
							cr->move_to(x1, y1);
						cr->line_to(x2, y2);
						hasPrevPoint = true;
					} else {
						hasPrevPoint = false;
					}
					break;
				case Plot2DPointSet::DrawPoints:
					cr->move_to(x1 + 2.0, y1);
					cr->arc(x1, y1, 2.0, 0.0, 2*M_PI);
					break;
				case Plot2DPointSet::DrawColumns:
					if(y1 <= _topMargin + plotHeight)
					{
						double
							width = 10.0,
							startX = x1 - width*0.5,
							endX = x1 + width*0.5;
						if(startX < plotLeftMargin)
							startX = plotLeftMargin;
						if(endX > plotWidth + plotLeftMargin)
							endX = plotWidth + plotLeftMargin;
						cr->rectangle(startX, y1, endX - startX, _topMargin + plotHeight - y1);
					}
					break;
			}
		} else {
		}
	}
	switch(pointSet.DrawingStyle())
	{
		case Plot2DPointSet::DrawLines:
			cr->stroke();
			break;
		case Plot2DPointSet::DrawPoints:
			cr->fill();
			break;
		case Plot2DPointSet::DrawColumns:
			cr->fill_preserve();
			Cairo::RefPtr<Cairo::Pattern> source = cr->get_source();
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->stroke();
			cr->set_source(source);
			break;
	}

	// Draw "zero y" x-axis
	if(yMin <= 0.0 && yMax >= 0.0 && !_logarithmicYAxis)
	{
		cr->set_source_rgba(0.5, 0.5, 0.5, 1);
		cr->move_to(plotLeftMargin, yMax * plotHeight / (yMax - yMin) + _topMargin);
		cr->line_to(plotWidth + plotLeftMargin, yMax * plotHeight / (yMax - yMin) + _topMargin);
		cr->stroke();
	}
	
	cr->reset_clip();
}
