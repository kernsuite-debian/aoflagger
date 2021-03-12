#include "heatmapplot.h"

#include "colorscale.h"
#include "horizontalplotscale.h"
#include "verticalplotscale.h"
#include "title.h"

#include <cmath>
#include <iostream>
#include <fstream>

#include "../structures/colormap.h"

#include "../algorithms/thresholdconfig.h"
#include "../algorithms/thresholdtools.h"

#include "../util/logger.h"

#include <boost/algorithm/string.hpp>

#ifndef HAVE_EXP10
#define exp10(x) exp((2.3025850929940456840179914546844) * (x))
#endif

HeatMapPlot::HeatMapPlot()
    : _isInitialized(false),
      _isImageInvalidated(true),
      _initializedWidth(0),
      _initializedHeight(0),
      _showOriginalMask(true),
      _showAlternativeMask(true),
      _colorMap(ColorMap::Grayscale),
      _image(),
      _highlighting(false),
      _leftBorderSize(0.0),
      _rightBorderSize(0.0),
      _topBorderSize(0.0),
      _bottomBorderSize(0.0),
      _startHorizontal(0.0),
      _endHorizontal(1.0),
      _startVertical(0.0),
      _endVertical(1.0),
      _segmentedImage(),
      _colorScale(),
      _plotTitle(),
      _scaleOption(NormalScale),
      _showXYAxes(true),
      _showColorScale(true),
      _showXAxisDescription(true),
      _showYAxisDescription(true),
      _showZAxisDescription(true),
      _showTitle(true),
      _specifiedMax(1.0),
      _specifiedMin(0.0),
      _derivedMax(1.0),
      _derivedMin(0.0),
      _range(Winsorized),
      _cairoFilter(Cairo::FILTER_NEAREST),
      _manualTitle(false),
      _manualXAxisDescription(false),
      _manualYAxisDescription(false),
      _manualZAxisDescription(false),
      _highlightConfig(new ThresholdConfig()) {
  _highlightConfig->InitializeLengthsSingleSample();
}

HeatMapPlot::~HeatMapPlot() { Clear(); }

void HeatMapPlot::Clear() {
  if (HasImage()) {
    _originalMask.reset();
    _alternativeMask.reset();
    _highlightConfig.reset(new ThresholdConfig());
    _highlightConfig->InitializeLengthsSingleSample();
    _segmentedImage.reset();
    _image.reset();
  }
  _colorScale.reset();
  _plotTitle.reset();
  _isInitialized = false;
  _isImageInvalidated = true;
  _onZoomChanged();
}

void HeatMapPlot::redrawWithoutChanges(
    const Cairo::RefPtr<Cairo::Context>& cairo, size_t width, size_t height) {
  cairo->set_source_rgb(1.0, 1.0, 1.0);
  cairo->set_line_width(1.0);
  cairo->rectangle(0, 0, width, height);
  cairo->fill();

  if (_isInitialized) {
    int destWidth = (int)std::ceil(_horizontalScale.PlotWidth()),
        destHeight = height - (int)floor(_topBorderSize + _bottomBorderSize),
        sourceWidth = _imageSurface->get_width(),
        sourceHeight = _imageSurface->get_height();
    double clx1, cly1, clx2, cly2;
    cairo->get_clip_extents(clx1, cly1, clx2, cly2);
    bool outsideBox = clx2 < _leftBorderSize || cly2 < _topBorderSize ||
                      clx1 > std::round(_leftBorderSize) + destWidth ||
                      cly1 > std::round(_topBorderSize) + destHeight;
    if (!outsideBox) {
      cairo->save();
      cairo->translate((int)std::round(_leftBorderSize),
                       (int)std::round(_topBorderSize));
      cairo->scale((double)destWidth / (double)sourceWidth,
                   (double)destHeight / (double)sourceHeight);
      Cairo::RefPtr<Cairo::SurfacePattern> pattern =
          Cairo::SurfacePattern::create(_imageSurface);
      pattern->set_filter(_cairoFilter);
      cairo->set_source(pattern);
      cairo->rectangle(0, 0, sourceWidth, sourceHeight);
      cairo->clip();
      cairo->paint();
      cairo->restore();
      cairo->set_source_rgb(0.0, 0.0, 0.0);
      cairo->rectangle(std::round(_leftBorderSize), std::round(_topBorderSize),
                       destWidth, destHeight);
      cairo->stroke();
    }
    if (_showColorScale) _colorScale->Draw(cairo);
    if (_showXYAxes) {
      _verticalScale.Draw(
          cairo, _leftBorderSize - _verticalScale.GetWidth(cairo), 0.0);
      _horizontalScale.Draw(cairo);
    }
    if (_plotTitle != nullptr) _plotTitle->Draw(cairo);
  }
}

void HeatMapPlot::ZoomFit() {
  _startHorizontal = 0.0;
  _endHorizontal = 1.0;
  _startVertical = 0.0;
  _endVertical = 1.0;
  _isImageInvalidated = true;
  _onZoomChanged.emit();
}

void HeatMapPlot::ZoomIn() {
  double distX = (_endHorizontal - _startHorizontal) * 0.25;
  _startHorizontal += distX;
  _endHorizontal -= distX;
  double distY = (_endVertical - _startVertical) * 0.25;
  _startVertical += distY;
  _endVertical -= distY;
  _isImageInvalidated = true;
  _onZoomChanged.emit();
}

void HeatMapPlot::ZoomInOn(size_t x, size_t y) {
  double xr = double(x) / _image->Width(), yr = double(y) / _image->Height();
  double distX = (_endHorizontal - _startHorizontal) * 0.25;
  _startHorizontal = xr - distX;
  _endHorizontal = xr + distX;
  if (_startHorizontal < 0.0) {
    _endHorizontal -= _startHorizontal;
    _startHorizontal = 0.0;
  }
  if (_endHorizontal > 1.0) {
    _startHorizontal -= _endHorizontal - 1.0;
    _endHorizontal = 1.0;
  }
  double distY = (_endVertical - _startVertical) * 0.25;
  _startVertical = yr - distY;
  _endVertical = yr + distY;
  if (_startVertical < 0.0) {
    _endVertical -= _startVertical;
    _startVertical = 0.0;
  }
  if (_endVertical > 1.0) {
    _startVertical -= _endVertical - 1.0;
    _endVertical = 1.0;
  }
  _isImageInvalidated = true;
  _onZoomChanged.emit();
}

void HeatMapPlot::ZoomOut() {
  if (!IsZoomedOut()) {
    double distX = (_endHorizontal - _startHorizontal) * 0.5;
    _startHorizontal -= distX;
    _endHorizontal += distX;
    if (_startHorizontal < 0.0) {
      _endHorizontal -= _startHorizontal;
      _startHorizontal = 0.0;
    }
    if (_endHorizontal > 1.0) {
      _startHorizontal -= _endHorizontal - 1.0;
      _endHorizontal = 1.0;
    }
    if (_startHorizontal < 0.0) _startHorizontal = 0.0;

    double distY = (_endVertical - _startVertical) * 0.5;
    _startVertical -= distY;
    _endVertical += distY;
    if (_startVertical < 0.0) {
      _endVertical -= _startVertical;
      _startVertical = 0.0;
    }
    if (_endVertical > 1.0) {
      _startVertical -= _endVertical - 1.0;
      _endVertical = 1.0;
    }
    if (_startVertical < 0.0) _startVertical = 0.0;
    _isImageInvalidated = true;
    _onZoomChanged.emit();
  }
}

void HeatMapPlot::ZoomTo(size_t x1, size_t y1, size_t x2, size_t y2) {
  if (x1 > x2) std::swap(x1, x2);
  if (y1 > y2) std::swap(y1, y2);
  _startHorizontal = std::max(0.0, std::min(1.0, double(x1) / _image->Width()));
  _endHorizontal =
      std::max(0.0, std::min(1.0, double(x2 + 1) / _image->Width()));
  _startVertical = std::max(0.0, std::min(1.0, double(y1) / _image->Height()));
  _endVertical =
      std::max(0.0, std::min(1.0, double(y2 + 1) / _image->Height()));
  _isImageInvalidated = true;
  _onZoomChanged.emit();
}

void HeatMapPlot::Pan(int xDisplacement, int yDisplacement) {
  double dx = double(xDisplacement) / _image->Width(),
         dy = double(yDisplacement) / _image->Height();
  if (_startHorizontal + dx < 0.0) dx = -_startHorizontal;
  if (_startVertical + dy < 0.0) dy = -_startVertical;
  if (_endHorizontal + dx > 1.0) dx = 1.0 - _endHorizontal;
  if (_endVertical + dy > 1.0) dy = 1.0 - _endVertical;
  _startHorizontal += dx;
  _endHorizontal += dx;
  _startVertical += dy;
  _endVertical += dy;
  _isImageInvalidated = true;
  _onZoomChanged.emit();
}

void HeatMapPlot::Draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
                       size_t height) {
  if (HasImage()) {
    update(cairo, width, height);
  } else {
    redrawWithoutChanges(cairo, width, height);
  }
}

void HeatMapPlot::SaveByExtension(const std::string& filename, size_t width,
                                  size_t height) {
  const char* eMsg =
      "Saving image to file failed: could not determine file type from "
      "filename extension -- maybe the type is not supported. Supported types "
      "are .png, .svg or .pdf.";
  if (filename.size() < 4) throw std::runtime_error(eMsg);
  std::string ext = filename.substr(filename.size() - 4);
  boost::to_lower<std::string>(ext);
  if (ext == ".png")
    SavePng(filename, width, height);
  else if (ext == ".svg")
    SaveSvg(filename, width, height);
  else if (ext == ".pdf")
    SavePdf(filename, width, height);
  else
    throw std::runtime_error(eMsg);
}

void HeatMapPlot::SavePdf(const std::string& filename, size_t width,
                          size_t height) {
  Cairo::RefPtr<Cairo::PdfSurface> surface =
      Cairo::PdfSurface::create(filename, width, height);
  Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  if (HasImage()) {
    Logger::Debug << "Saving PDF of " << width << " x " << height << "\n";
    update(cairo, width, height);
  }
  cairo->show_page();
  // We finish the surface. This might be required, because some of the
  // subclasses store the cairo context. In that case, it won't be written.
  surface->finish();
}

void HeatMapPlot::SaveSvg(const std::string& filename, size_t width,
                          size_t height) {
  Cairo::RefPtr<Cairo::SvgSurface> surface =
      Cairo::SvgSurface::create(filename, width, height);
  Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  if (HasImage()) {
    Logger::Debug << "Saving SVG of " << width << " x " << height << "\n";
    update(cairo, width, height);
  }
  cairo->show_page();
  surface->finish();
}

void HeatMapPlot::SavePng(const std::string& filename, size_t width,
                          size_t height) {
  Cairo::RefPtr<Cairo::ImageSurface> surface =
      Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  if (HasImage()) {
    Logger::Debug << "Saving PNG of " << width << " x " << height << "\n";
    update(cairo, width, height);
  }
  surface->write_to_png(filename);
}

void HeatMapPlot::SaveText(const std::string& filename) {
  if (HasImage()) {
    size_t startX = (size_t)std::round(_startHorizontal * _image->Width()),
           startY = (size_t)std::round(_startVertical * _image->Height()),
           endX = (size_t)std::round(_endHorizontal * _image->Width()),
           endY = (size_t)std::round(_endVertical * _image->Height());
    size_t imageWidth = endX - startX, imageHeight = endY - startY;
    Logger::Debug << "Saving text file for " << imageWidth << " x "
                  << imageHeight << " values.\n";
    std::ofstream file(filename.c_str());
    file << imageWidth << '\n' << imageHeight << '\n';
    for (size_t y = startY; y != endY; ++y) {
      for (size_t x = startX; x != endX; ++x) {
        file << _image->Value(x, y) << '\n';
      }
    }
  }
}

void HeatMapPlot::update(const Cairo::RefPtr<Cairo::Context>& cairo,
                         size_t width, size_t height) {
  _isImageInvalidated = _isImageInvalidated || (width != _initializedWidth) ||
                        (height != _initializedHeight);

  size_t startX = (size_t)std::round(_startHorizontal * _image->Width()),
         startY = (size_t)std::round(_startVertical * _image->Height()),
         endX = (size_t)std::round(_endHorizontal * _image->Width()),
         endY = (size_t)std::round(_endVertical * _image->Height());
  if (startX >= endX) {
    endX = startX + 1;
    if (endX >= _image->Width()) {
      startX -= endX - _image->Width();
      endX = _image->Width();
    }
  }
  if (startY >= endY) {
    endY = startY + 1;
    if (endY >= _image->Height()) {
      startY -= endY - _image->Height();
      endY = _image->Height();
    }
  }
  size_t startTimestep = startX, endTimestep = endX;
  size_t imageWidth = endX - startX, imageHeight = endY - startY;

  Mask2DCPtr originalMask, alternativeMask;
  Image2DCPtr image;
  if (_isImageInvalidated) {
    Mask2DCPtr mask = GetActiveMask();
    originalMask = _originalMask, alternativeMask = _alternativeMask;
    image = _image;
    if (imageWidth > 30000) {
      int shrinkFactor = (imageWidth + 29999) / 30000;
      image = Image2D::MakePtr(image->ShrinkHorizontally(shrinkFactor));
      mask = Mask2D::MakePtr(mask->ShrinkHorizontally(shrinkFactor));
      if (originalMask)
        originalMask =
            Mask2D::MakePtr(originalMask->ShrinkHorizontally(shrinkFactor));
      if (alternativeMask)
        alternativeMask =
            Mask2D::MakePtr(alternativeMask->ShrinkHorizontally(shrinkFactor));
      startX /= shrinkFactor;
      endX /= shrinkFactor;
      imageWidth = endX - startX;
    }

    findMinMax(image.get(), mask.get(), _derivedMin, _derivedMax);
  }

  _colorScale.reset();
  _plotTitle.reset();

  if (_showXYAxes) {
    _verticalScale.SetDrawWithDescription(_showYAxisDescription);
    _horizontalScale.SetDrawWithDescription(_showXAxisDescription);
  }
  if (_showColorScale) {
    _colorScale.reset(new ColorScale());
    _colorScale->SetDrawWithDescription(_showZAxisDescription);
  } else {
    _colorScale.reset();
  }
  if (_showXYAxes) {
    if (_metaData != nullptr && _metaData->HasBand()) {
      _verticalScale.InitializeNumericTicks(
          _metaData->Band().channels[startY].frequencyHz / 1e6,
          _metaData->Band().channels[endY - 1].frequencyHz / 1e6);
      _verticalScale.SetUnitsCaption("Frequency (MHz)");
    } else {
      _verticalScale.InitializeNumericTicks(-0.5 + startY, 0.5 + endY - 1.0);
    }
    if (_metaData != nullptr && _metaData->HasObservationTimes()) {
      _horizontalScale.InitializeTimeTicks(
          _metaData->ObservationTimes()[startTimestep],
          _metaData->ObservationTimes()[endTimestep - 1]);
      _horizontalScale.SetUnitsCaption("Time (UTC, hh:mm:ss)");
    } else {
      _horizontalScale.InitializeNumericTicks(-0.5 + startTimestep,
                                              0.5 + endTimestep - 1.0);
    }
    if (_manualXAxisDescription)
      _horizontalScale.SetUnitsCaption(_xAxisDescription);
    if (_manualYAxisDescription)
      _verticalScale.SetUnitsCaption(_yAxisDescription);
  }
  if (_metaData != nullptr) {
    if (_showColorScale && _metaData->ValueDescription() != "") {
      if (_metaData->ValueUnits() != "")
        _colorScale->SetUnitsCaption(_metaData->ValueDescription() + " (" +
                                     _metaData->ValueUnits() + ")");
      else
        _colorScale->SetUnitsCaption(_metaData->ValueDescription());
    }
  }
  if (_showColorScale) {
    if (_scaleOption == LogScale)
      _colorScale->InitializeLogarithmicTicks(_derivedMin, _derivedMax);
    else
      _colorScale->InitializeNumericTicks(_derivedMin, _derivedMax);
    if (_manualZAxisDescription)
      _colorScale->SetUnitsCaption(_zAxisDescription);
  }

  if (_showTitle && !actualTitleText().empty()) {
    _plotTitle.reset(new Title());
    _plotTitle->SetText(actualTitleText());
    _plotTitle->SetPlotDimensions(width, height, 0.0);
    _topBorderSize = _plotTitle->GetHeight(cairo);
  } else {
    _plotTitle.reset();
    if (_horizontalScale.MemberCount() != 1)
      _topBorderSize = 0.0;
    else
      _topBorderSize = 10.0;
  }
  // The scale dimensions are depending on each other. However, since the height
  // of the horizontal scale is practically not dependent on other dimensions,
  // we give the horizontal scale temporary width/height, so that we can
  // calculate its height:
  if (_showXYAxes) {
    if (_isImageInvalidated) {
      _horizontalScale.SetPlotDimensions(width, height, 0.0, 0.0, false);
      _horizontalScale.CalculateScales(cairo);
    }
    _bottomBorderSize = _horizontalScale.Height();
    _rightBorderSize = _horizontalScale.RightMargin();

    _verticalScale.SetPlotDimensions(
        width - _rightBorderSize + 5.0,
        height - _topBorderSize - _bottomBorderSize, _topBorderSize, false);
    _leftBorderSize = _verticalScale.GetWidth(cairo);
  } else {
    _bottomBorderSize = 0.0;
    _rightBorderSize = 0.0;
    _leftBorderSize = 0.0;
  }
  double colorScaleWidth = 0.0;
  if (_showColorScale) {
    _colorScale->SetPlotDimensions(width, height - _topBorderSize,
                                   _topBorderSize, false);
    colorScaleWidth = _colorScale->GetWidth(cairo);
  }
  if (_showXYAxes) {
    if (_isImageInvalidated) {
      _horizontalScale.SetPlotDimensions(width - colorScaleWidth, height,
                                         _leftBorderSize, _topBorderSize,
                                         false);
      _horizontalScale.CalculateScales(cairo);
    }
    _leftBorderSize = _horizontalScale.FromLeft();
    _rightBorderSize = width - _horizontalScale.PlotWidth() - _leftBorderSize;
  }

  std::unique_ptr<ColorMap> colorMap(ColorMap::CreateColorMap(_colorMap));

  const double minLog10 = _derivedMin > 0.0 ? log10(_derivedMin) : 0.0,
               maxLog10 = _derivedMax > 0.0 ? log10(_derivedMax) : 0.0;
  if (_showColorScale) {
    for (size_t x = 0; x < 256; ++x) {
      num_t colorVal = (2.0 / 256.0) * x - 1.0;
      num_t imageVal;
      if (_scaleOption == LogScale)
        imageVal =
            exp10((x / 256.0) * (log10(_derivedMax) - minLog10) + minLog10);
      else
        imageVal = (_derivedMax - _derivedMin) * x / 256.0 + _derivedMin;
      double r = colorMap->ValueToColorR(colorVal),
             g = colorMap->ValueToColorG(colorVal),
             b = colorMap->ValueToColorB(colorVal);
      _colorScale->SetColorValue(imageVal, r / 255.0, g / 255.0, b / 255.0);
    }
  }

  if (_isImageInvalidated) {
    _isImageInvalidated = false;
    _imageSurface.clear();
    _imageSurface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                                imageWidth, imageHeight);

    _imageSurface->flush();
    unsigned char* data = _imageSurface->get_data();
    size_t rowStride = _imageSurface->get_stride();

    Mask2DPtr highlightMask;
    if (_highlighting) {
      highlightMask =
          Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
      _highlightConfig->Execute(image.get(), highlightMask.get(), true, 10.0,
                                10.0);
    }
    const bool originalActive = _showOriginalMask && originalMask != nullptr,
               altActive = _showAlternativeMask && alternativeMask != nullptr;
    int orMaskR = 255, orMaskG = 0, orMaskB = 255;
    int altMaskR = 255, altMaskG = 255, altMaskB = 0;
    if (_colorMap == ColorMap::Viridis) {
      orMaskR = 0;
      orMaskG = 0;
      orMaskB = 0;
      altMaskR = 255;
      altMaskG = 255;
      altMaskB = 255;
    }
    for (size_t y = startY; y < endY; ++y) {
      guint8* rowpointer = data + rowStride * (endY - y - 1);
      for (size_t x = startX; x < endX; ++x) {
        int xa = (x - startX) * 4;
        unsigned char r, g, b, a;
        if (_highlighting && highlightMask->Value(x, y) != 0) {
          r = 255;
          g = 0;
          b = 0;
          a = 255;
        } else if (originalActive && originalMask->Value(x, y)) {
          r = orMaskR;
          g = orMaskG;
          b = orMaskB;
          a = 255;
        } else if (altActive && alternativeMask->Value(x, y)) {
          r = altMaskR;
          g = altMaskG;
          b = altMaskB;
          a = 255;
        } else {
          num_t val = image->Value(x, y);
          if (val > _derivedMax)
            val = _derivedMax;
          else if (val < _derivedMin)
            val = _derivedMin;

          if (_scaleOption == LogScale) {
            if (image->Value(x, y) <= 0.0)
              val = -1.0;
            else
              val = (log10(image->Value(x, y)) - minLog10) * 2.0 /
                        (maxLog10 - minLog10) -
                    1.0;
          } else
            val = (image->Value(x, y) - _derivedMin) * 2.0 /
                      (_derivedMax - _derivedMin) -
                  1.0;
          if (val < -1.0)
            val = -1.0;
          else if (val > 1.0)
            val = 1.0;
          r = colorMap->ValueToColorR(val);
          g = colorMap->ValueToColorG(val);
          b = colorMap->ValueToColorB(val);
          a = colorMap->ValueToColorA(val);
        }
        rowpointer[xa] = b;
        rowpointer[xa + 1] = g;
        rowpointer[xa + 2] = r;
        rowpointer[xa + 3] = a;
      }
    }

    if (_segmentedImage != nullptr) {
      for (size_t y = startY; y < endY; ++y) {
        guint8* rowpointer = data + rowStride * (endY - y - 1);
        for (size_t x = startX; x < endX; ++x) {
          if (_segmentedImage->Value(x, y) != 0) {
            int xa = (x - startX) * 4;
            rowpointer[xa] = IntMap::R(_segmentedImage->Value(x, y));
            rowpointer[xa + 1] = IntMap::G(_segmentedImage->Value(x, y));
            rowpointer[xa + 2] = IntMap::B(_segmentedImage->Value(x, y));
            rowpointer[xa + 3] = IntMap::A(_segmentedImage->Value(x, y));
          }
        }
      }
    }
    _imageSurface->mark_dirty();

    while (_imageSurface->get_width() > (int)width ||
           _imageSurface->get_height() > (int)height) {
      size_t newWidth = _imageSurface->get_width(),
             newHeight = _imageSurface->get_height();
      if (newWidth > width) newWidth = width;
      if (newHeight > height) newHeight = height;
      downsampleImageBuffer(newWidth, newHeight);
    }
  }

  _isInitialized = true;
  _initializedWidth = width;
  _initializedHeight = height;
  _isImageInvalidated = false;
  redrawWithoutChanges(cairo, width, height);
}

void HeatMapPlot::findMinMax(const Image2D* image, const Mask2D* mask,
                             num_t& min, num_t& max) {
  switch (_range) {
    case MinMax:
      max = ThresholdTools::MaxValue(image, mask);
      min = ThresholdTools::MinValue(image, mask);
      break;
    case Winsorized: {
      num_t mean, stddev, genMax, genMin;
      ThresholdTools::WinsorizedMeanAndStdDev(image, mask, mean, stddev);
      genMax = ThresholdTools::MaxValue(image, mask);
      genMin = ThresholdTools::MinValue(image, mask);
      max = mean + stddev * 3.0;
      min = mean - stddev * 3.0;
      if (genMin > min) min = genMin;
      if (genMax < max) max = genMax;
    } break;
    case Specified:
      min = _specifiedMin;
      max = _specifiedMax;
      break;
  }
  if (min == max) {
    min -= 1.0;
    max += 1.0;
  }
  if (_scaleOption == LogScale && min <= 0.0) {
    if (max <= 0.0) {
      max = 1.0;
    }
    min = max / 10000.0;
  }
  if (_scaleOption == ZeroSymmetricScale) {
    if (fabs(max) > fabs(min)) {
      max = fabs(max);
      min = -max;
    } else {
      min = -fabs(min);
      max = -min;
    }
  }
  _specifiedMax = max;
  _specifiedMin = min;
}

void HeatMapPlot::downsampleImageBuffer(size_t newWidth, size_t newHeight) {
  _imageSurface->flush();
  const size_t oldWidth = _imageSurface->get_width(),
               oldHeight = _imageSurface->get_height();

  Cairo::RefPtr<Cairo::ImageSurface> newImageSurface =
      Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, newWidth, newHeight);

  unsigned char* newData = newImageSurface->get_data();
  size_t rowStrideOfNew = newImageSurface->get_stride();

  unsigned char* oldData = _imageSurface->get_data();
  size_t rowStrideOfOld = _imageSurface->get_stride();

  for (size_t y = 0; y < newHeight; ++y) {
    guint8* rowpointerToNew = newData + rowStrideOfNew * y;

    for (size_t x = 0; x < newWidth; ++x) {
      unsigned int r = 0, g = 0, b = 0, a = 0;

      const size_t xOldStart = x * oldWidth / newWidth,
                   xOldEnd = (x + 1) * oldWidth / newWidth,
                   yOldStart = y * oldHeight / newHeight,
                   yOldEnd = (y + 1) * oldHeight / newHeight;

      for (size_t yOld = yOldStart; yOld < yOldEnd; ++yOld) {
        unsigned char* rowpointerToOld =
            oldData + rowStrideOfOld * yOld + xOldStart * 4;
        for (size_t xOld = xOldStart; xOld < xOldEnd; ++xOld) {
          r += (*rowpointerToOld);
          ++rowpointerToOld;
          g += (*rowpointerToOld);
          ++rowpointerToOld;
          b += (*rowpointerToOld);
          ++rowpointerToOld;
          a += (*rowpointerToOld);
          ++rowpointerToOld;
        }
      }

      const size_t count = (xOldEnd - xOldStart) * (yOldEnd - yOldStart);
      (*rowpointerToNew) = (unsigned char)(r / count);
      ++rowpointerToNew;
      (*rowpointerToNew) = (unsigned char)(g / count);
      ++rowpointerToNew;
      (*rowpointerToNew) = (unsigned char)(b / count);
      ++rowpointerToNew;
      (*rowpointerToNew) = (unsigned char)(a / count);
      ++rowpointerToNew;
    }
  }

  _imageSurface = newImageSurface;
  _imageSurface->mark_dirty();
}

Mask2DCPtr HeatMapPlot::GetActiveMask() const {
  if (!HasImage())
    throw std::runtime_error("GetActiveMask() called without image");
  const bool originalActive = _showOriginalMask && _originalMask != nullptr,
             altActive = _showAlternativeMask && _alternativeMask != nullptr;
  if (originalActive) {
    if (altActive) {
      Mask2DPtr mask = Mask2D::MakePtr(*_originalMask);
      mask->Join(*_alternativeMask);
      return mask;
    } else
      return _originalMask;
  } else {
    if (altActive)
      return _alternativeMask;
    else
      return Mask2D::CreateSetMaskPtr<false>(_image->Width(), _image->Height());
  }
}

TimeFrequencyMetaDataCPtr HeatMapPlot::GetSelectedMetaData() const {
  TimeFrequencyMetaDataCPtr metaData = _metaData;

  if (_startVertical != 0 && metaData != nullptr && metaData->HasBand()) {
    size_t startChannel = std::round(StartVertical() * _image->Height());
    TimeFrequencyMetaData* newData = new TimeFrequencyMetaData(*metaData);
    metaData = TimeFrequencyMetaDataCPtr(newData);
    BandInfo band = newData->Band();
    band.channels.erase(band.channels.begin(),
                        band.channels.begin() + startChannel);
    newData->SetBand(band);
  }
  if (_startHorizontal != 0 && metaData != nullptr &&
      metaData->HasObservationTimes()) {
    size_t startTime = std::round(StartHorizontal() * _image->Width());
    TimeFrequencyMetaData* newData = new TimeFrequencyMetaData(*metaData);
    metaData = TimeFrequencyMetaDataCPtr(newData);
    std::vector<double> obsTimes = newData->ObservationTimes();
    obsTimes.erase(obsTimes.begin(), obsTimes.begin() + startTime);
    newData->SetObservationTimes(obsTimes);
  }

  return metaData;
}

bool HeatMapPlot::ConvertToUnits(double mouseX, double mouseY, int& posX,
                                 int& posY) const {
  int startX = (int)std::round(_startHorizontal * _image->Width()),
      startY = (int)std::round(_startVertical * _image->Height()),
      endX = (int)std::round(_endHorizontal * _image->Width()),
      endY = (int)std::round(_endVertical * _image->Height());
  if (endX <= startX) endX = startX + 1;
  if (endY <= startY) endY = startY + 1;
  const int width = endX - startX, height = endY - startY;
  posX =
      (int)round((mouseX - _leftBorderSize) * width /
                     (_initializedWidth - _rightBorderSize - _leftBorderSize) -
                 0.5);
  posY =
      (int)round((mouseY - _topBorderSize) * height /
                     (_initializedHeight - _bottomBorderSize - _topBorderSize) -
                 0.5);
  bool inDomain =
      posX >= 0 && posY >= 0 && posX < (int)width && posY < (int)height;
  posX = std::max(0, std::min(width - 1, posX)) + startX;
  posY = endY - 1 - std::max(0, std::min(height - 1, posY));
  return inDomain;
}

bool HeatMapPlot::ConvertToScreen(int posX, int posY, double& mouseX,
                                  double& mouseY) const {
  size_t startX = (size_t)std::round(_startHorizontal * _image->Width()),
         startY = (size_t)std::round(_startVertical * _image->Height()),
         endX = (size_t)std::round(_endHorizontal * _image->Width()),
         endY = (size_t)std::round(_endVertical * _image->Height());
  if (endX <= startX) endX = startX + 1;
  if (endY <= startY) endY = startY + 1;
  const size_t width = endX - startX, height = endY - startY;
  posX -= startX;
  posY = endY - posY - 1;
  mouseX = (posX + 0.5) *
               (_initializedWidth - _rightBorderSize - _leftBorderSize) /
               width +
           _leftBorderSize;
  mouseY = (posY + 0.5) *
               (_initializedHeight - _bottomBorderSize - _topBorderSize) /
               height +
           _topBorderSize;
  return true;
}
