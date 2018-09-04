#include "heatmapplot.h"

#include "colorscale.h"
#include "horizontalplotscale.h"
#include "verticalplotscale.h"
#include "title.h"

#include <cmath>
#include <iostream>
#include <fstream>

#include "../structures/colormap.h"

#include "../strategy/algorithms/thresholdconfig.h"
#include "../strategy/algorithms/thresholdtools.h"

#include "../util/logger.h"

#include <boost/algorithm/string.hpp>

#ifndef HAVE_EXP10
#define exp10(x) exp( (2.3025850929940456840179914546844) * (x) )
#endif

HeatMapPlot::HeatMapPlot() :
	_isInitialized(false),
	_initializedWidth(0),
	_initializedHeight(0),
	_showOriginalMask(true),
	_showAlternativeMask(true),
	_colorMap(BWMap),
	_image(),
	_highlighting(false),
	_startHorizontal(0.0),
	_endHorizontal(1.0),
	_startVertical(0.0),
	_endVertical(1.0),
	_segmentedImage(),
	_horiScale(),
	_vertScale(),
	_colorScale(),
	_plotTitle(),
	_scaleOption(NormalScale),
	_showXYAxes(true),
	_showColorScale(true),
	_showXAxisDescription(true),
	_showYAxisDescription(true),
	_showZAxisDescription(true),
	_showTitle(true),
	_max(1.0), _min(0.0),
	_range(Winsorized),
	_cairoFilter(Cairo::FILTER_NEAREST),
	_manualTitle(false),
	_manualXAxisDescription(false),
	_manualYAxisDescription(false),
	_manualZAxisDescription(false),
	_highlightConfig(new ThresholdConfig())
{
	_highlightConfig->InitializeLengthsSingleSample();
}

HeatMapPlot::~HeatMapPlot()
{
	Clear();
}

void HeatMapPlot::Clear()
{
  if(HasImage())
	{
		_originalMask.reset();
		_alternativeMask.reset();
		_highlightConfig.reset(new ThresholdConfig());
		_highlightConfig->InitializeLengthsSingleSample();
		_segmentedImage.reset();
		_image.reset();
	}
	_horiScale.reset();
	_vertScale.reset();
	_colorScale.reset();
	_plotTitle.reset();
	_isInitialized = false;
}

void HeatMapPlot::redrawWithoutChanges(const Cairo::RefPtr<Cairo::Context>& cairo, unsigned width, unsigned height)
{
	cairo->set_source_rgb(1.0, 1.0, 1.0);
	cairo->set_line_width(1.0);
	cairo->rectangle(0, 0, width, height);
	cairo->fill();
		
	if(_isInitialized) {
		int
			destWidth = width - (int) floor(_leftBorderSize + _rightBorderSize),
			destHeight = height - (int) floor(_topBorderSize + _bottomBorderSize),
			sourceWidth = _imageSurface->get_width(),
			sourceHeight = _imageSurface->get_height();
		cairo->save();
		cairo->translate((int) round(_leftBorderSize), (int) round(_topBorderSize));
		cairo->scale((double) destWidth / (double) sourceWidth, (double) destHeight / (double) sourceHeight);
		Cairo::RefPtr<Cairo::SurfacePattern> pattern = Cairo::SurfacePattern::create(_imageSurface);
		pattern->set_filter(_cairoFilter);
		cairo->set_source(pattern);
		cairo->rectangle(0, 0, sourceWidth, sourceHeight);
		cairo->clip();
		cairo->paint();
		cairo->restore();
		cairo->set_source_rgb(0.0, 0.0, 0.0);
		cairo->rectangle(round(_leftBorderSize), round(_topBorderSize), destWidth, destHeight);
		cairo->stroke();

		if(_showColorScale)
			_colorScale->Draw(cairo);
		if(_showXYAxes)
		{
			_vertScale->Draw(cairo, 0.0, 0.0);
			_horiScale->Draw(cairo);
		}
		if(_plotTitle != 0)
			_plotTitle->Draw(cairo);
	}
}

void HeatMapPlot::ZoomFit()
{
	_startHorizontal = 0.0;
	_endHorizontal = 1.0;
	_startVertical = 0.0;
	_endVertical = 1.0;
	_onZoomChanged.emit();
}

void HeatMapPlot::ZoomIn()
{
	double distX = (_endHorizontal-_startHorizontal)*0.25;
	_startHorizontal += distX;
	_endHorizontal -= distX;
	double distY = (_endVertical-_startVertical)*0.25;
	_startVertical += distY;
	_endVertical -= distY;
	_onZoomChanged.emit();
}

void HeatMapPlot::ZoomInOn(size_t x, size_t y)
{
	double xr = double(x) / _image->Width(), yr = double(y) / _image->Height();
	double distX = (_endHorizontal-_startHorizontal)*0.25;
	_startHorizontal = xr - distX;
	_endHorizontal = xr + distX;
	if(_startHorizontal < 0.0)
	{
		_endHorizontal -= _startHorizontal;
		_startHorizontal = 0.0;
	}
	if(_endHorizontal > 1.0)
	{
		_startHorizontal -= _endHorizontal - 1.0;
		_endHorizontal = 1.0;
	}
	double distY = (_endVertical-_startVertical)*0.25;
	_startVertical = yr - distY;
	_endVertical = yr + distY;
	if(_startVertical < 0.0)
	{
		_endVertical -= _startVertical;
		_startVertical = 0.0;
	}
	if(_endVertical > 1.0)
	{
		_startVertical -= _endVertical - 1.0;
		_endVertical = 1.0;
	}
	_onZoomChanged.emit();
}

void HeatMapPlot::ZoomOut()
{
	if(!IsZoomedOut())
	{
		double distX = (_endHorizontal-_startHorizontal)*0.5;
		_startHorizontal -= distX;
		_endHorizontal += distX;
		if(_startHorizontal < 0.0) {
			_endHorizontal -= _startHorizontal;
			_startHorizontal = 0.0;
		}
		if(_endHorizontal > 1.0) {
			_startHorizontal -= _endHorizontal-1.0;
			_endHorizontal = 1.0;
		}
		if(_startHorizontal < 0.0) _startHorizontal = 0.0;
		
		double distY = (_endVertical-_startVertical)*0.5;
		_startVertical -= distY;
		_endVertical += distY;
		if(_startVertical < 0.0) {
			_endVertical -= _startVertical;
			_startVertical = 0.0;
		}
		if(_endVertical > 1.0) {
			_startVertical -= _endVertical-1.0;
			_endVertical = 1.0;
		}
		if(_startVertical < 0.0) _startVertical = 0.0;
		_onZoomChanged.emit();
	}
}

void HeatMapPlot::ZoomTo(size_t x1, size_t y1, size_t x2, size_t y2)
{
	if(x1 > x2)
		std::swap(x1, x2);
	if(y1 > y2)
		std::swap(y1, y2);
	_startHorizontal = std::max(0.0, std::min(1.0, double(x1) / _image->Width()));
	_endHorizontal = std::max(0.0, std::min(1.0, double(x2+1) / _image->Width()));
	_startVertical = std::max(0.0, std::min(1.0, double(y1) / _image->Height()));
	_endVertical = std::max(0.0, std::min(1.0, double(y2+1) / _image->Height()));
	_onZoomChanged.emit();
}
	
void HeatMapPlot::Pan(int xDisplacement, int yDisplacement)
{
	double
		dx = double(xDisplacement) / _image->Width(),
		dy = double(yDisplacement) / _image->Height();
	if(_startHorizontal + dx < 0.0)
		dx = -_startHorizontal;
	if(_startVertical + dy < 0.0)
		dy = -_startVertical;
	if(_endHorizontal + dx > 1.0)
		dx = 1.0 - _endHorizontal;
	if(_endVertical + dy > 1.0)
		dy = 1.0 - _endVertical;
	_startHorizontal += dx;
	_endHorizontal += dx;
	_startVertical += dy;
	_endVertical += dy;
	_onZoomChanged.emit();
}

void HeatMapPlot::Draw(const Cairo::RefPtr<Cairo::Context>& cairo, unsigned width, unsigned height, bool isInvalidated)
{
	if(!isInvalidated && width == _initializedWidth && height == _initializedHeight)
		redrawWithoutChanges(cairo, width, height);
	else {
		if(HasImage())
		{
			update(cairo, width, height);
		}
		else {
			redrawWithoutChanges(cairo, width, height);
		}
	}
}

void HeatMapPlot::SaveByExtension(const std::string& filename, unsigned width, unsigned height)
{
	const char* eMsg = "Saving image to file failed: could not determine file type from filename extension -- maybe the type is not supported. Supported types are .png, .svg or .pdf.";
	if(filename.size() < 4)
		throw std::runtime_error(eMsg);
	std::string ext = filename.substr(filename.size()-4);
	boost::to_lower<std::string>(ext);
	if(ext == ".png")
		SavePng(filename, width, height);
	else if(ext == ".svg")
		SaveSvg(filename, width, height);
	else if(ext == ".pdf")
		SavePdf(filename, width, height);
	else throw std::runtime_error(eMsg);
}

void HeatMapPlot::SavePdf(const std::string &filename, unsigned width, unsigned height)
{
	Cairo::RefPtr<Cairo::PdfSurface> surface = Cairo::PdfSurface::create(filename, width, height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	if(HasImage())
	{
		Logger::Debug << "Saving PDF of " << width << " x " << height << "\n";
		update(cairo, width, height);
	}
	cairo->show_page();
	// We finish the surface. This might be required, because some of the subclasses store the cairo context. In that
	// case, it won't be written.
	surface->finish();
}

void HeatMapPlot::SaveSvg(const std::string &filename, unsigned width, unsigned height)
{
	Cairo::RefPtr<Cairo::SvgSurface> surface = Cairo::SvgSurface::create(filename, width, height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	if(HasImage())
	{
		Logger::Debug << "Saving SVG of " << width << " x " << height << "\n";
		update(cairo, width, height);
	}
	cairo->show_page();
	surface->finish();
}

void HeatMapPlot::SavePng(const std::string &filename, unsigned width, unsigned height)
{
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
	Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
	if(HasImage())
	{
		Logger::Debug << "Saving PNG of " << width << " x " << height << "\n";
		update(cairo, width, height);
	}
	surface->write_to_png(filename);
}

void HeatMapPlot::SaveText(const std::string &filename)
{
	if(HasImage())
	{
		unsigned int
			startX = (unsigned int) round(_startHorizontal * _image->Width()),
			startY = (unsigned int) round(_startVertical * _image->Height()),
			endX = (unsigned int) round(_endHorizontal * _image->Width()),
			endY = (unsigned int) round(_endVertical * _image->Height());
		size_t
			imageWidth = endX - startX,
			imageHeight = endY - startY;
		Logger::Debug << "Saving text file for " << imageWidth << " x " << imageHeight << " values.\n";
		std::ofstream file(filename.c_str());
		file << imageWidth << '\n' << imageHeight << '\n';
		for(size_t y=startY; y!=endY; ++y)
		{
			for(size_t x=startX; x!=endX; ++x)
			{
				file << _image->Value(x, y) << '\n';
			}
		}
	}
}

void HeatMapPlot::update(const Cairo::RefPtr<Cairo::Context>& cairo, unsigned width, unsigned height)
{
	Mask2DCPtr mask = GetActiveMask(), originalMask = _originalMask, alternativeMask = _alternativeMask;
	
	unsigned int
		startX = (unsigned int) round(_startHorizontal * _image->Width()),
		startY = (unsigned int) round(_startVertical * _image->Height()),
		endX = (unsigned int) round(_endHorizontal * _image->Width()),
		endY = (unsigned int) round(_endVertical * _image->Height());
	if(startX >= endX)
	{
		endX = startX+1;
		if(endX >= _image->Width())
		{
			startX -= endX - _image->Width();
			endX = _image->Width();
		}
	}
	if(startY >= endY)
	{
		endY = startY+1;
		if(endY >= _image->Height())
		{
			startY -= endY - _image->Height();
			endY = _image->Height();
		}
	}
	unsigned int
		startTimestep = startX,
		endTimestep = endX;
	size_t
		imageWidth = endX - startX,
		imageHeight = endY - startY;
		
	Image2DCPtr image = _image;
	if(imageWidth > 30000)
	{
		int shrinkFactor = (imageWidth + 29999) / 30000;
		image = Image2D::MakePtr(image->ShrinkHorizontally(shrinkFactor));
		mask = Mask2D::MakePtr(mask->ShrinkHorizontally(shrinkFactor));
		if(originalMask != 0)
			originalMask = Mask2D::MakePtr(originalMask->ShrinkHorizontally(shrinkFactor));
		if(alternativeMask != 0)
			alternativeMask = Mask2D::MakePtr(alternativeMask->ShrinkHorizontally(shrinkFactor));
		startX /= shrinkFactor;
		endX /= shrinkFactor;
		imageWidth = endX - startX;
	}

	num_t min, max;
	findMinMax(image.get(), mask.get(), min, max);
	
	_horiScale.reset();
	_vertScale.reset();
	_colorScale.reset();
	_plotTitle.reset();
		
	if(_showXYAxes)
	{
		_vertScale.reset(new VerticalPlotScale());
		_vertScale->SetDrawWithDescription(_showYAxisDescription);
		_horiScale.reset(new HorizontalPlotScale());
		_horiScale->SetDrawWithDescription(_showXAxisDescription);
	}
	if(_showColorScale)
	{
		_colorScale.reset(new ColorScale());
		_colorScale->SetDrawWithDescription(_showZAxisDescription);
	} else {
		_colorScale = 0;
	}
	if(_showXYAxes)
	{
		if(_metaData != 0 && _metaData->HasBand()) {
			_vertScale->InitializeNumericTicks(_metaData->Band().channels[startY].frequencyHz / 1e6, _metaData->Band().channels[endY-1].frequencyHz / 1e6);
			_vertScale->SetUnitsCaption("Frequency (MHz)");
		} else {
			_vertScale->InitializeNumericTicks(-0.5 + startY, 0.5 + endY - 1.0);
		}
		if(_metaData != 0 && _metaData->HasObservationTimes())
		{
			_horiScale->InitializeTimeTicks(_metaData->ObservationTimes()[startTimestep], _metaData->ObservationTimes()[endTimestep-1]);
			_horiScale->SetUnitsCaption("Time (UTC, hh:mm:ss)");
		} else {
			_horiScale->InitializeNumericTicks(-0.5 + startTimestep, 0.5 + endTimestep - 1.0);
		}
		if(_manualXAxisDescription)
			_horiScale->SetUnitsCaption(_xAxisDescription);
		if(_manualYAxisDescription)
			_vertScale->SetUnitsCaption(_yAxisDescription);
	}
	if(_metaData != 0) {
		if(_showColorScale && _metaData->ValueDescription()!="")
		{
			if(_metaData->ValueUnits()!="")
				_colorScale->SetUnitsCaption(_metaData->ValueDescription() + " (" + _metaData->ValueUnits() + ")");
			else
				_colorScale->SetUnitsCaption(_metaData->ValueDescription());
		}
	}
	if(_showColorScale)
	{
		if(_scaleOption == LogScale)
			_colorScale->InitializeLogarithmicTicks(min, max);
		else
			_colorScale->InitializeNumericTicks(min, max);
		if(_manualZAxisDescription)
			_colorScale->SetUnitsCaption(_zAxisDescription);
	}

	if(_showTitle && !actualTitleText().empty())
	{
		_plotTitle.reset(new Title());
		_plotTitle->SetText(actualTitleText());
		_plotTitle->SetPlotDimensions(width, height, 0.0);
		_topBorderSize = _plotTitle->GetHeight(cairo);
	} else {
		_plotTitle = 0;
		_topBorderSize = 10.0;
	}
	// The scale dimensions are depending on each other. However, since the height of the horizontal scale is practically
	// not dependent on other dimensions, we give the horizontal scale temporary width/height, so that we can calculate its height:
	if(_showXYAxes)
	{
		_horiScale->SetPlotDimensions(width, height, 0.0, 0.0, false);
		_bottomBorderSize = _horiScale->GetHeight(cairo);
		_rightBorderSize = _horiScale->GetRightMargin(cairo);
	
		_vertScale->SetPlotDimensions(width - _rightBorderSize + 5.0, height - _topBorderSize - _bottomBorderSize, _topBorderSize, false);
		_leftBorderSize = _vertScale->GetWidth(cairo);
	} else {
		_bottomBorderSize = 0.0;
		_rightBorderSize = 0.0;
		_leftBorderSize = 0.0;
	}
	if(_showColorScale)
	{
		_colorScale->SetPlotDimensions(width - _rightBorderSize, height - _topBorderSize, _topBorderSize, false);
		_rightBorderSize += _colorScale->GetWidth(cairo) + 5.0;
	}
	if(_showXYAxes)
	{
		_horiScale->SetPlotDimensions(width - _rightBorderSize + 5.0, height -_topBorderSize - _bottomBorderSize, _vertScale->GetWidth(cairo), _topBorderSize, false);
	}

	std::unique_ptr<ColorMap> colorMap(createColorMap());
	
	const double
		minLog10 = min>0.0 ? log10(min) : 0.0,
		maxLog10 = max>0.0 ? log10(max) : 0.0;
	if(_showColorScale)
	{
		for(unsigned x=0;x<256;++x)
		{
			num_t colorVal = (2.0 / 256.0) * x - 1.0;
			num_t imageVal;
			if(_scaleOption == LogScale)
				imageVal = exp10((x / 256.0) * (log10(max) - minLog10) + minLog10);
			else 
				imageVal = (max-min) * x / 256.0 + min;
			double
				r = colorMap->ValueToColorR(colorVal),
				g = colorMap->ValueToColorG(colorVal),
				b = colorMap->ValueToColorB(colorVal);
			_colorScale->SetColorValue(imageVal, r/255.0, g/255.0, b/255.0);
		}
	}
	
	_imageSurface.clear();
	_imageSurface =
		Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, imageWidth, imageHeight);

	_imageSurface->flush();
	unsigned char *data = _imageSurface->get_data();
	size_t rowStride = _imageSurface->get_stride();

	Mask2DPtr highlightMask;
	if(_highlighting)
	{
		highlightMask = Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
		_highlightConfig->Execute(image.get(), highlightMask.get(), true, 10.0, 10.0);
	}
	const bool
		originalActive = _showOriginalMask && originalMask != 0,
		altActive = _showAlternativeMask && alternativeMask != 0;
	int orMaskR=255, orMaskG=0, orMaskB=255;
	int altMaskR=255, altMaskG=255, altMaskB=0;
	if(_colorMap == ViridisMap)
	{
		orMaskR=0; orMaskG=0; orMaskB=0;
		altMaskR=255; altMaskG=255; altMaskB=255;
	}
	for(unsigned long y=startY;y<endY;++y) {
		guint8* rowpointer = data + rowStride * (endY - y - 1);
		for(unsigned long x=startX;x<endX;++x) {
			int xa = (x-startX) * 4;
			unsigned char r,g,b,a;
			if(_highlighting && highlightMask->Value(x, y) != 0) {
				r = 255; g = 0; b = 0; a = 255;
			} else if(originalActive && originalMask->Value(x, y)) {
				r = orMaskR; g = orMaskG; b = orMaskB; a = 255;
			} else if(altActive && alternativeMask->Value(x, y)) {
				r = altMaskR; g = altMaskG; b = altMaskB; a = 255;
			} else {
				num_t val = image->Value(x, y);
				if(val > max) val = max;
				else if(val < min) val = min;

				if(_scaleOption == LogScale)
				{
					if(image->Value(x, y) <= 0.0)
						val = -1.0;
					else
						val = (log10(image->Value(x, y)) - minLog10) * 2.0 / (maxLog10 - minLog10) - 1.0;
				}
				else
					val = (image->Value(x, y) - min) * 2.0 / (max - min) - 1.0;
				if(val < -1.0) val = -1.0;
				else if(val > 1.0) val = 1.0;
				r = colorMap->ValueToColorR(val);
				g = colorMap->ValueToColorG(val);
				b = colorMap->ValueToColorB(val);
				a = colorMap->ValueToColorA(val);
			}
			rowpointer[xa]=b;
			rowpointer[xa+1]=g;
			rowpointer[xa+2]=r;
			rowpointer[xa+3]=a;
		}
	}
	colorMap.reset();

	if(_segmentedImage != 0)
	{
		for(unsigned long y=startY;y<endY;++y) {
			guint8* rowpointer = data + rowStride * (y - startY);
			for(unsigned long x=startX;x<endX;++x) {
				if(_segmentedImage->Value(x,y) != 0)
				{
					int xa = (x-startX) * 4;
					rowpointer[xa]=IntMap::R(_segmentedImage->Value(x,y));
					rowpointer[xa+1]=IntMap::G(_segmentedImage->Value(x,y));
					rowpointer[xa+2]=IntMap::B(_segmentedImage->Value(x,y));
					rowpointer[xa+3]=IntMap::A(_segmentedImage->Value(x,y));
				}
			}
		}
	}
	_imageSurface->mark_dirty();

	while(_imageSurface->get_width() > (int) width || _imageSurface->get_height() > (int) height)
	{
		unsigned
			newWidth = _imageSurface->get_width(),
			newHeight = _imageSurface->get_height();
		if(newWidth > width)
			newWidth = width;
		if(newHeight > height)
			newHeight = height;
		downsampleImageBuffer(newWidth, newHeight);
	}

	_isInitialized = true;
	_initializedWidth = width;
	_initializedHeight = height;
	redrawWithoutChanges(cairo, width, height);
} 

std::unique_ptr<ColorMap> HeatMapPlot::createColorMap()
{
	using CM=std::unique_ptr<ColorMap>;
	switch(_colorMap) {
		case BWMap:
			return CM(new MonochromeMap());
		case InvertedMap:
			return CM(new class InvertedMap());
		case HotColdMap:
			return CM(new ColdHotMap());
		case RedBlueMap:
			return CM(new class RedBlueMap());
		case RedYellowBlueMap:
			return CM(new class RedYellowBlueMap());
		case FireMap:
			return CM(new class FireMap());
		case BlackRedMap:
			return CM(new class BlackRedMap());
		case ViridisMap:
			return CM(new class ViridisMap());
		default:
			return 0;
	}
}

void HeatMapPlot::findMinMax(const Image2D* image, const Mask2D* mask, num_t &min, num_t &max)
{
	switch(_range)
	{
		case MinMax:
			max = ThresholdTools::MaxValue(image, mask);
			min = ThresholdTools::MinValue(image, mask);
		break;
		case Winsorized:
		{
			num_t mean, stddev, genMax, genMin;
			ThresholdTools::WinsorizedMeanAndStdDev(image, mask, mean, stddev);
			genMax = ThresholdTools::MaxValue(image, mask);
			genMin = ThresholdTools::MinValue(image, mask);
			max = mean + stddev*3.0;
			min = mean - stddev*3.0;
			if(genMin > min) min = genMin;
			if(genMax < max) max = genMax;
		}
		break;
		case Specified:
			min = _min;
			max = _max;
		break;
	}
	if(min == max)
	{
		min -= 1.0;
		max += 1.0;
	}
	if(_scaleOption == LogScale && min<=0.0)
	{
		if(max <= 0.0)
		{
			max = 1.0;
		}
		min = max / 10000.0;
	}
	if(_scaleOption == ZeroSymmetricScale)
	{
		if(fabs(max) > fabs(min))
		{
			max = fabs(max);
			min = -max;
		} else {
			min = -fabs(min);
			max = -min;
		}
	}
	_max = max;
	_min = min;
}

void HeatMapPlot::downsampleImageBuffer(unsigned newWidth, unsigned newHeight)
{
	_imageSurface->flush();
	const unsigned
		oldWidth = _imageSurface->get_width(),
		oldHeight = _imageSurface->get_height();
	
	Cairo::RefPtr<Cairo::ImageSurface> newImageSurface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, newWidth, newHeight);

	unsigned char* newData = newImageSurface->get_data();
	size_t rowStrideOfNew = newImageSurface->get_stride();

	unsigned char *oldData = _imageSurface->get_data();
	size_t rowStrideOfOld = _imageSurface->get_stride();

	for(unsigned int y=0;y<newHeight;++y) {
		guint8* rowpointerToNew = newData + rowStrideOfNew * y;
		
		for(unsigned int x=0;x<newWidth;++x) {
			unsigned int r=0, g=0, b=0, a=0;
			
			const unsigned
				xOldStart = x * oldWidth / newWidth,
				xOldEnd = (x+1) * oldWidth / newWidth,
				yOldStart = y * oldHeight / newHeight,
				yOldEnd = (y+1) * oldHeight / newHeight;
			
			for(unsigned int yOld=yOldStart;yOld<yOldEnd;++yOld)
			{
				unsigned char *rowpointerToOld = oldData + rowStrideOfOld * yOld + xOldStart*4;
				for(unsigned int xOld=xOldStart;xOld<xOldEnd;++xOld)
				{
					r += (*rowpointerToOld); ++rowpointerToOld;
					g += (*rowpointerToOld); ++rowpointerToOld;
					b += (*rowpointerToOld); ++rowpointerToOld;
					a += (*rowpointerToOld); ++rowpointerToOld;
				}
			}
			
			const unsigned count = (xOldEnd - xOldStart) * (yOldEnd - yOldStart);
			(*rowpointerToNew) = (unsigned char) (r/count);
			++rowpointerToNew;
			(*rowpointerToNew) = (unsigned char) (g/count);
			++rowpointerToNew;
			(*rowpointerToNew) = (unsigned char) (b/count);
			++rowpointerToNew;
			(*rowpointerToNew) = (unsigned char) (a/count);
			++rowpointerToNew;
		}
	}

	_imageSurface = newImageSurface;
	_imageSurface->mark_dirty();
}

Mask2DCPtr HeatMapPlot::GetActiveMask() const
{
	if(!HasImage())
		throw std::runtime_error("GetActiveMask() called without image");
	const bool
		originalActive = _showOriginalMask && _originalMask != 0,
		altActive = _showAlternativeMask && _alternativeMask != 0;
	if(originalActive)
	{
		if(altActive)
		{
			Mask2DPtr mask = Mask2D::MakePtr(*_originalMask); 
			mask->Join(*_alternativeMask);
			return mask;
		} else
			return _originalMask;
	} else {
		if(altActive)
			return _alternativeMask;
		else
			return Mask2D::CreateSetMaskPtr<false>(_image->Width(), _image->Height());
	}
}

TimeFrequencyMetaDataCPtr HeatMapPlot::GetSelectedMetaData() const
{
	TimeFrequencyMetaDataCPtr metaData = _metaData;

	if(_startVertical != 0 && metaData != 0)
	{
		size_t startChannel = round(StartVertical() * _image->Height());
		TimeFrequencyMetaData *newData = new TimeFrequencyMetaData(*metaData);
		metaData = TimeFrequencyMetaDataCPtr(newData);
		BandInfo band = newData->Band();
		band.channels.erase(band.channels.begin(), band.channels.begin()+startChannel );
		newData->SetBand(band);
	}
	if(_startHorizontal != 0 && metaData != 0)
	{
		size_t startTime = round(StartHorizontal() * _image->Width());
		TimeFrequencyMetaData *newData = new TimeFrequencyMetaData(*metaData);
		metaData = TimeFrequencyMetaDataCPtr(newData);
		std::vector<double> obsTimes = newData->ObservationTimes();
		obsTimes.erase(obsTimes.begin(), obsTimes.begin()+startTime );
		newData->SetObservationTimes(obsTimes);
	}
	
	return metaData;
}

bool HeatMapPlot::ConvertToUnits(double mouseX, double mouseY, int &posX, int &posY) const
{
	unsigned int
		startX = (unsigned int) round(_startHorizontal * _image->Width()),
		startY = (unsigned int) round(_startVertical * _image->Height()),
		endX = (unsigned int) round(_endHorizontal * _image->Width()),
		endY = (unsigned int) round(_endVertical * _image->Height());
	if(endX <= startX)
		endX = startX+1;
	if(endY <= startY)
		endY = startY+1;
	const unsigned
		width = endX - startX,
		height = endY - startY;
	posX = (int) round((mouseX - _leftBorderSize) * width / (_initializedWidth - _rightBorderSize - _leftBorderSize) - 0.5);
	posY = (int) round((mouseY - _topBorderSize) * height / (_initializedHeight - _bottomBorderSize - _topBorderSize) - 0.5);
	bool inDomain = posX >= 0 && posY >= 0 && posX < (int) width && posY < (int) height;
	posX += startX;
	posY = endY - posY - 1;
	return inDomain;
}

bool HeatMapPlot::ConvertToScreen(int posX, int posY, double& mouseX, double& mouseY) const
{
	unsigned int
		startX = (unsigned int) round(_startHorizontal * _image->Width()),
		startY = (unsigned int) round(_startVertical * _image->Height()),
		endX = (unsigned int) round(_endHorizontal * _image->Width()),
		endY = (unsigned int) round(_endVertical * _image->Height());
	if(endX <= startX)
		endX = startX+1;
	if(endY <= startY)
		endY = startY+1;
	const unsigned
		width = endX - startX,
		height = endY - startY;
	posX -= startX;
	posY = endY - posY - 1;
	mouseX = (posX + 0.5) * (_initializedWidth - _rightBorderSize - _leftBorderSize) / width + _leftBorderSize;
	mouseY = (posY + 0.5) *(_initializedHeight - _bottomBorderSize - _topBorderSize) / height + _topBorderSize;
	return true;
}
