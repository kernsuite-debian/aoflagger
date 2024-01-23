#include "heatmap.h"

#include "colormap.h"
#include "colorscale.h"
#include "horizontalplotscale.h"
#include "title.h"
#include "vectorimage.h"
#include "verticalplotscale.h"

#include "ticksets/tickset.h"

#include <cmath>

namespace {
// We need this less than operator, because the normal operator
// does not enforce a strictly ordered set, because a<b != !(b<a) in the case
// of nans/infs.
bool numLessThanOperator(const float& a, const float& b) {
  if (std::isfinite(a)) {
    if (std::isfinite(b))
      return a < b;
    else
      return true;
  }
  return false;
}

void WinsorizedMeanAndStdDev(const ImageInterface& image, float& mean,
                             float& stddev) {
  const size_t size = image.Width() * image.Height();
  std::unique_ptr<float[]> data(new float[size]);
  const float* imageData = image.Data();
  for (size_t y = 0; y != image.Height(); ++y) {
    const float* row = &imageData[y * image.Stride()];
    std::copy_n(row, image.Width(), &data[y * image.Width()]);
  }
  std::sort(data.get(), data.get() + size, numLessThanOperator);
  const size_t lowIndex = (size_t)std::floor(0.1 * size);
  const size_t highIndex = (size_t)std::ceil(0.9 * size) - 1;
  const float lowValue = data[lowIndex];
  const float highValue = data[highIndex];
  data.reset();

  // Calculate mean
  mean = 0.0;
  size_t count = 0;
  for (size_t y = 0; y != image.Height(); ++y) {
    const float* row = &imageData[y * image.Stride()];
    for (size_t x = 0; x != image.Width(); ++x) {
      if (std::isfinite(row[x])) {
        if (row[x] < lowValue)
          mean += lowValue;
        else if (row[x] > highValue)
          mean += highValue;
        else
          mean += row[x];
        count++;
      }
    }
  }
  if (count > 0) mean /= float(count);
  // Calculate variance
  stddev = 0.0;
  count = 0;
  for (size_t y = 0; y != image.Height(); ++y) {
    const float* row = &imageData[y * image.Stride()];
    for (size_t x = 0; x != image.Width(); ++x) {
      if (std::isfinite(row[x])) {
        if (row[x] < lowValue)
          stddev += (lowValue - mean) * (lowValue - mean);
        else if (row[x] > highValue)
          stddev += (highValue - mean) * (highValue - mean);
        else
          stddev += (row[x] - mean) * (row[x] - mean);
        count++;
      }
    }
  }
  if (count > 0)
    stddev = std::sqrt(1.54 * stddev / float(count));
  else
    stddev = 0.0;
}
}  // namespace

#ifndef HAVE_EXP10
#define exp10(x) exp((2.3025850929940456840179914546844) * (x))
#endif

HeatMap::HeatMap()
    : _isInitialized(false),
      _isImageInvalidated(true),
      _initializedWidth(0),
      _initializedHeight(0),
      _colorMap(ColorMap::Viridis),
      _image(),
      _leftBorderSize(0.0),
      _rightBorderSize(0.0),
      _topBorderSize(0.0),
      _bottomBorderSize(0.0),
      _colorScale(),
      _plotTitle(),
      _logXScale(false),
      _logYScale(false),
      _logZScale(false),
      _showXAxis(true),
      _showYAxis(true),
      _showX2Axis(false),
      _showY2Axis(false),
      _showColorScale(true),
      _showXAxisDescription(true),
      _showYAxisDescription(true),
      _showZAxisDescription(true),
      _showTitle(true),
      _xAxisType(AxisType::kNumeric),
      _yAxisType(AxisType::kNumeric),
      _derivedMax(1.0),
      _derivedMin(0.0),
      _xAxisMin(1.0),
      _xAxisMax(10.0),
      _yAxisMin(1.0),
      _yAxisMax(10.0),
      _x2AxisMin(1.0),
      _x2AxisMax(10.0),
      _y2AxisMin(1.0),
      _y2AxisMax(10.0),
      _zRange(WinsorizedRange()),
      _cairoFilter(Cairo::FILTER_NEAREST) {
  OnZoomChanged().connect([&]() { _isImageInvalidated = true; });
}

void HeatMap::Clear() {
  _image.reset();
  _isInitialized = false;
  _isImageInvalidated = true;
  OnZoomChanged()();
}

void HeatMap::redrawWithoutChanges(const Cairo::RefPtr<Cairo::Context>& cairo,
                                   size_t width, size_t height) {
  cairo->set_source_rgb(1.0, 1.0, 1.0);
  cairo->set_line_width(1.0);
  cairo->rectangle(0, 0, width, height);
  cairo->fill();

  const int destWidth = int(std::ceil(_horizontalScale.PlotWidth()));
  const int destHeight =
      height - int(std::floor(_topBorderSize + _bottomBorderSize));
  if (_isInitialized && destWidth > 0 && destHeight > 0 && _imageSurface) {
    const int sourceWidth = _imageSurface->get_width();
    const int sourceHeight = _imageSurface->get_height();
    double clx1, cly1, clx2, cly2;
    cairo->get_clip_extents(clx1, cly1, clx2, cly2);
    const bool outsideBox = clx2 < _leftBorderSize || cly2 < _topBorderSize ||
                            clx1 > std::round(_leftBorderSize) + destWidth ||
                            cly1 > std::round(_topBorderSize) + destHeight;
    if (!outsideBox) {
      cairo->save();
      cairo->translate((int)std::round(_leftBorderSize),
                       (int)std::round(_topBorderSize));
      cairo->scale((double)destWidth / (double)sourceWidth,
                   (double)destHeight / (double)sourceHeight);
      const Cairo::RefPtr<Cairo::SurfacePattern> pattern =
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
    if (_showColorScale) _colorScale.Draw(cairo);
    if (_showYAxis) {
      _verticalScale.Draw(
          cairo, _leftBorderSize - _verticalScale.GetWidth(cairo), 0.0);
    }
    if (_showY2Axis) {
      _vertScale2.Draw(cairo, width - _rightBorderSize, 0.0);
    }
    if (_showXAxis) {
      _horizontalScale.Draw(cairo);
    }
    if (_showX2Axis) {
      _horiScale2.Draw(cairo);
    }
    if (_showTitle) _plotTitle.Draw(cairo);

    postRender(cairo, width, height);
  }
}

void HeatMap::Draw(const Cairo::RefPtr<Cairo::Context>& cairo) {
  if (HasImage()) {
    drawAll(cairo, Width(), Height());
  } else {
    redrawWithoutChanges(cairo, Width(), Height());
  }
}

void HeatMap::SavePdf(const std::string& filename, size_t width,
                      size_t height) {
  const Cairo::RefPtr<Cairo::PdfSurface> surface =
      Cairo::PdfSurface::create(filename, width, height);
  const Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  if (HasImage()) {
    PlotBase::Draw(cairo, width, height);
  }
  cairo->show_page();
  // We finish the surface. This might be required, because some of the
  // subclasses store the cairo context. In that case, it won't be written.
  surface->finish();
}

void HeatMap::SaveSvg(const std::string& filename, size_t width,
                      size_t height) {
  const Cairo::RefPtr<Cairo::SvgSurface> surface =
      Cairo::SvgSurface::create(filename, width, height);
  const Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  if (HasImage()) {
    PlotBase::Draw(cairo, width, height);
  }
  cairo->show_page();
  surface->finish();
}

void HeatMap::SavePng(const std::string& filename, size_t width,
                      size_t height) {
  const Cairo::RefPtr<Cairo::ImageSurface> surface =
      Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, width, height);
  const Cairo::RefPtr<Cairo::Context> cairo = Cairo::Context::create(surface);
  if (HasImage()) {
    PlotBase::Draw(cairo, width, height);
  }
  surface->write_to_png(filename);
}

void HeatMap::drawAll(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
                      size_t height) {
  _isImageInvalidated = _isImageInvalidated || (width != _initializedWidth) ||
                        (height != _initializedHeight);
  _initializedWidth = width;
  _initializedHeight = height;

  const std::array<size_t, 2> imageXRange = ImageXRange();
  const std::array<size_t, 2> imageYRange = ImageYRange();
  std::array<size_t, 2> surfaceXRange = imageXRange;
  std::array<size_t, 2> surfaceYRange = imageYRange;

  std::unique_ptr<ImageInterface> scratchImage;
  if (_isImageInvalidated) {
    ImageInterface* image = _image.get();
    const int xFactor = ImageToSurfaceXFactor();
    if (xFactor > 1) {
      scratchImage = ShrinkHorizontally(*image, xFactor);
      surfaceXRange[0] /= xFactor;
      surfaceXRange[1] /= xFactor;
    }

    const int yFactor = ImageToSurfaceYFactor();
    if (yFactor > 1) {
      scratchImage = ShrinkVertically(*image, yFactor);
      surfaceYRange[0] /= yFactor;
      surfaceYRange[1] /= yFactor;
    }

    std::tie(_derivedMin, _derivedMax) =
        DetermineRange(*image, _zRange, _logZScale);

    const size_t surfaceWidth = surfaceXRange[1] - surfaceXRange[0];
    const size_t surfaceHeight = surfaceYRange[1] - surfaceYRange[0];

    initializeComponents();

    if (_showTitle && !_titleText.empty()) {
      _plotTitle.SetPlotDimensions(width, height, 0.0);
      _topBorderSize = _plotTitle.GetHeight(cairo);
    } else {
      if (_horizontalScale.MemberCount() != 1)
        _topBorderSize = 0.0;
      else
        _topBorderSize = 10.0;
    }
    double temporary_right_border_size = 0.0;
    // The scale dimensions are depending on each other. However, since the
    // height of the horizontal scale is practically not dependent on other
    // dimensions, it is assumed to be constant:
    if (_showXAxis) {
      _topAxisHeight = 0.0;
      _bottomBorderSize = _horizontalScale.CalculateHeight(cairo);
      temporary_right_border_size = _horizontalScale.RightMargin();
    } else {
      _bottomBorderSize = 0.0;
      temporary_right_border_size = 0.0;
    }
    if (_showX2Axis) {
      temporary_right_border_size =
          std::max(temporary_right_border_size, _horiScale2.RightMargin());
      _topAxisHeight += _horiScale2.CalculateHeight(cairo);
    }

    if (_showYAxis) {
      const double plot_height =
          height - _topBorderSize - _bottomBorderSize - _topAxisHeight;
      _verticalScale.SetPlotDimensions(
          width - temporary_right_border_size + 5.0, plot_height,
          _topBorderSize, false);
      _leftBorderSize = _verticalScale.GetWidth(cairo);
    } else {
      _leftBorderSize = 0.0;
    }
    if (_showY2Axis) {
      _vertScale2.SetPlotDimensions(
          width - temporary_right_border_size + 5.0,
          height - _topBorderSize - _bottomBorderSize - _topAxisHeight,
          _topBorderSize, true);
    }
    double colorScaleWidth = 0.0;
    if (_showColorScale) {
      _colorScale.SetPlotDimensions(width, height - _topBorderSize,
                                    _topBorderSize, false);
      colorScaleWidth = _colorScale.GetWidth(cairo);
    }
    const double topAxis2Size =
        _showX2Axis ? _horiScale2.CalculateHeight(cairo) : 0.0;
    double vertScaleWidth = 0.0;
    if (_showYAxis) {
      _verticalScale.SetPlotDimensions(
          width - colorScaleWidth,
          height - _topBorderSize - _bottomBorderSize - _topAxisHeight,
          _topBorderSize + topAxis2Size, false);
      vertScaleWidth = _verticalScale.GetWidth(cairo);
    }
    if (_showY2Axis) {
      _vertScale2.SetPlotDimensions(
          width - colorScaleWidth,
          height - _topBorderSize - _bottomBorderSize - _topAxisHeight,
          _topBorderSize + topAxis2Size, true);
      temporary_right_border_size += _vertScale2.GetWidth(cairo);
    }

    _horizontalScale.SetPlotDimensions(width - colorScaleWidth, height,
                                       _leftBorderSize, _topBorderSize, false);
    _horizontalScale.CalculateScales(cairo);
    if (_showX2Axis) {
      _horiScale2.SetPlotDimensions(width - colorScaleWidth,
                                    height - _topBorderSize - _bottomBorderSize,
                                    vertScaleWidth, _topBorderSize, true);
      _topBorderSize += topAxis2Size;
    }

    _leftBorderSize = _horizontalScale.FromLeft();
    _rightBorderSize = width - _horizontalScale.PlotWidth() - _leftBorderSize;

    if (surfaceWidth > 0 && surfaceHeight > 0) {
      const double minLog10 = _derivedMin > 0.0 ? std::log10(_derivedMin) : 0.0;
      const double maxLog10 = _derivedMax > 0.0 ? std::log10(_derivedMax) : 0.0;
      std::unique_ptr<ColorMap> colorMap(ColorMap::CreateColorMap(_colorMap));
      if (_showColorScale) {
        _colorScale.Clear();
        for (size_t x = 0; x < 256; ++x) {
          const double colorVal = (2.0 / 256.0) * x - 1.0;
          double imageVal;
          if (_logZScale)
            imageVal = exp10(
                (x / 256.0) * (std::log10(_derivedMax) - minLog10) + minLog10);
          else
            imageVal = (_derivedMax - _derivedMin) * x / 256.0 + _derivedMin;
          const double r = colorMap->ValueToColorR(colorVal);
          const double g = colorMap->ValueToColorG(colorVal);
          const double b = colorMap->ValueToColorB(colorVal);
          _colorScale.SetColorValue(imageVal, r / 255.0, g / 255.0, b / 255.0);
        }
      }

      _imageSurface.clear();
      _imageSurface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                                  surfaceWidth, surfaceHeight);

      _imageSurface->flush();
      unsigned char* data = _imageSurface->get_data();
      const size_t rowStride = _imageSurface->get_stride();
      const float* imageData = image->Data();
      for (size_t y = surfaceYRange[0]; y != surfaceYRange[1]; ++y) {
        guint8* rowpointer = data + rowStride * (surfaceYRange[1] - y - 1);
        const float* imageRow = &imageData[y * image->Stride()];
        for (size_t x = surfaceXRange[0]; x != surfaceXRange[1]; ++x) {
          const int xa = (x - surfaceXRange[0]) * 4;
          float val = imageRow[x];
          if (val > _derivedMax)
            val = _derivedMax;
          else if (val < _derivedMin)
            val = _derivedMin;

          if (_logZScale) {
            if (val <= 0.0)
              val = -1.0;
            else
              val = (std::log10(val) - minLog10) * 2.0 / (maxLog10 - minLog10) -
                    1.0;
          } else {
            val = (val - _derivedMin) * 2.0 / (_derivedMax - _derivedMin) - 1.0;
          }
          if (val < -1.0)
            val = -1.0;
          else if (val > 1.0)
            val = 1.0;
          rowpointer[xa + 0] = colorMap->ValueToColorB(val);
          rowpointer[xa + 1] = colorMap->ValueToColorG(val);
          rowpointer[xa + 2] = colorMap->ValueToColorR(val);
          rowpointer[xa + 3] = colorMap->ValueToColorA(val);
        }
      }
      _signalDrawImage(_imageSurface);
      _imageSurface->mark_dirty();

      while (_imageSurface->get_width() > (int)width ||
             _imageSurface->get_height() > (int)height) {
        const size_t newWidth =
            std::min<size_t>(width, _imageSurface->get_width());
        const size_t newHeight =
            std::min<size_t>(height, _imageSurface->get_height());
        downsampleImageBuffer(newWidth, newHeight);
      }
    }
  }

  _isInitialized = true;
  _isImageInvalidated = false;
  redrawWithoutChanges(cairo, width, height);
}

void HeatMap::initializeComponents() {
  if (_showColorScale) {
    _colorScale.SetDrawWithDescription(_showZAxisDescription);
    _colorScale.SetUnitsCaption(_zAxisDescription);
    _colorScale.SetTickRange(_derivedMin, _derivedMax);
    _colorScale.SetLogarithmic(_logZScale);
    _colorScale.InitializeTicks();
  }
  if (_showXAxis) {
    _horizontalScale.SetDrawWithDescription(_showXAxisDescription);
    _horizontalScale.SetUnitsCaption(_xAxisDescription);
    _horizontalScale.SetAxisType(_xAxisType);
    const bool useLogX = _logXScale && _xAxisMin > 0;
    _horizontalScale.SetLogarithmic(useLogX);
    _horizontalScale.InitializeTicks();
    _horiScale2.SetAxisType(AxisType::kNumeric);
    _horiScale2.SetTickRange(_x2AxisMin, _x2AxisMax);
    _horiScale2.SetLogarithmic(useLogX);
    _horiScale2.InitializeTicks();
    const double xAxisRange = _xAxisMax - _xAxisMin;
    const std::array<size_t, 2> pixelXRange = ImageXRange();
    if (_logXScale) {
      // TODO fix zoom pixel offsets
      _horizontalScale.SetTickRange(_xAxisMin, _xAxisMax);
    } else if (_image->Width() == 0) {
      _horizontalScale.SetTickRange(_xAxisMin, _xAxisMax);
    } else {
      const double left = _xAxisMin + double(-0.5 + pixelXRange[0]) /
                                          _image->Width() * xAxisRange;
      const double right = _xAxisMin + double(0.5 + pixelXRange[1] - 1.0) /
                                           _image->Width() * xAxisRange;
      _horizontalScale.SetTickRange(left, right);
    }
    _horizontalScale.InitializeTicks();
  }
  if (_showYAxis) {
    _verticalScale.SetDrawWithDescription(_showYAxisDescription);
    _verticalScale.SetLogarithmic(_logYScale && _yAxisMin > 0);
    const double yAxisRange = _yAxisMax - _yAxisMin;
    const std::array<size_t, 2> pixelYRange = ImageYRange();
    if (_logYScale) {
      // TODO fix pixel offsets
      _verticalScale.SetTickRange(_yAxisMin, _yAxisMax);
    } else if (_image->Height() == 0) {
      _verticalScale.SetTickRange(_yAxisMin, _yAxisMax);
    } else {
      const double bottom = _yAxisMin + double(-0.5 + pixelYRange[0]) /
                                            _image->Height() * yAxisRange;
      const double top = _yAxisMin + double(0.5 + pixelYRange[1] - 1.0) /
                                         _image->Height() * yAxisRange;
      _verticalScale.SetTickRange(bottom, top);
    }
    _verticalScale.InitializeTicks();
    _verticalScale.SetUnitsCaption(_yAxisDescription);
  }
  if (_showX2Axis) {
    _horiScale2.SetDrawWithDescription(_showXAxisDescription);
    _horiScale2.SetUnitsCaption(_x2AxisDescription);
  }
  if (_showY2Axis) {
    _vertScale2.SetDrawWithDescription(_showYAxisDescription);
    _vertScale2.SetAxisType(_yAxisType);
    _vertScale2.SetTickRange(_y2AxisMin, _y2AxisMax);
    _vertScale2.SetLogarithmic(_verticalScale.IsLogarithmic());
    _vertScale2.InitializeTicks();
  }

  if (_showTitle) {
    _plotTitle.SetText(_titleText);
  }
}

void HeatMap::postRender(const Cairo::RefPtr<Cairo::Context>& cairo,
                         size_t width, size_t height) {
  const Rectangle pArea = getPlotArea(width, height);
  const double xAxisSize = _xAxisMax - _xAxisMin;
  const double yAxisSize = _yAxisMax - _yAxisMin;
  if (xAxisSize != 0.0 && yAxisSize != 0.0) {
    cairo->save();
    cairo->transform(Cairo::Matrix(
        pArea.width / xAxisSize, 0.0, 0.0, -pArea.height / yAxisSize,
        pArea.x - _xAxisMin * pArea.width / xAxisSize,
        height - (height - pArea.height - pArea.y) +
            _yAxisMin * pArea.height / yAxisSize));
    // cairo->rectangle(0.0, 0.0, xAxisSize, yAxisSize);
    // cairo->clip();

    _signalDraw(cairo);
    cairo->restore();
  }
}

std::pair<float, float> HeatMap::DetermineRange(
    const ImageInterface& image, const RangeConfiguration& range_configuration,
    bool log_scale) {
  float winsorized_mean = 0.0;
  float winsorized_stddev = 0.0;
  if (range_configuration.minimum == RangeLimit::Winsorized ||
      range_configuration.maximum == RangeLimit::Winsorized) {
    WinsorizedMeanAndStdDev(image, winsorized_mean, winsorized_stddev);
  }

  std::pair<float, float> result;

  switch (range_configuration.minimum) {
    case RangeLimit::Extreme:
      result.first = image.Minimum();
      break;
    case RangeLimit::Winsorized: {
      const double image_min = image.Minimum();
      result.first =
          std::max(image_min, winsorized_mean - winsorized_stddev * 3.0);
    } break;
    case RangeLimit::Specified:
      result.first = range_configuration.specified_min;
      break;
  }

  switch (range_configuration.maximum) {
    case RangeLimit::Extreme:
      result.second = image.Maximum();
      break;
    case RangeLimit::Winsorized: {
      const double image_max = image.Maximum();
      result.second =
          std::min(image_max, winsorized_mean + winsorized_stddev * 3.0);
    } break;
    case RangeLimit::Specified:
      result.second = range_configuration.specified_max;
      break;
  }

  if (result.first == result.second) {
    result.first -= 1.0;
    result.second += 1.0;
  }
  if (log_scale) {
    if (result.second <= 0.0) {
      result.second = 1.0;
    }
    if (result.first <= 0.0) {
      result.first = result.second / 10000.0;
    }
  }
  return result;
}

HeatMap::Rectangle HeatMap::getPlotArea(size_t width, size_t height) const {
  Rectangle r;
  r.width = width - _rightBorderSize - _leftBorderSize;
  r.height = height - _bottomBorderSize - _topBorderSize;
  r.x = _leftBorderSize;
  r.y = _topBorderSize;
  return r;
}

void HeatMap::downsampleImageBuffer(size_t newWidth, size_t newHeight) {
  _imageSurface->flush();
  const size_t oldWidth = _imageSurface->get_width();
  const size_t oldHeight = _imageSurface->get_height();

  const Cairo::RefPtr<Cairo::ImageSurface> newImageSurface =
      Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, newWidth, newHeight);

  unsigned char* newData = newImageSurface->get_data();
  const size_t rowStrideOfNew = newImageSurface->get_stride();

  unsigned char* oldData = _imageSurface->get_data();
  const size_t rowStrideOfOld = _imageSurface->get_stride();

  for (size_t y = 0; y < newHeight; ++y) {
    guint8* rowpointerToNew = newData + rowStrideOfNew * y;

    for (size_t x = 0; x < newWidth; ++x) {
      unsigned int r = 0, g = 0, b = 0, a = 0;

      const size_t xOldStart = x * oldWidth / newWidth;
      const size_t xOldEnd = (x + 1) * oldWidth / newWidth;
      const size_t yOldStart = y * oldHeight / newHeight;
      const size_t yOldEnd = (y + 1) * oldHeight / newHeight;

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

std::array<size_t, 2> HeatMap::ImageXRange() const {
  size_t startX = size_t(std::round(XZoomStart() * _image->Width()));
  size_t endX = size_t(std::round(XZoomEnd() * _image->Width()));
  if (startX >= endX && _image->Width() > 0) {
    endX = startX + 1;
    if (endX >= _image->Width()) {
      startX -= endX - _image->Width();
      endX = _image->Width();
    }
  }
  return {startX, endX};
}

std::array<size_t, 2> HeatMap::ImageYRange() const {
  size_t startY = size_t(std::round(YZoomStart() * _image->Height()));
  size_t endY = size_t(std::round(YZoomEnd() * _image->Height()));
  if (startY >= endY && _image->Height() > 0) {
    endY = startY + 1;
    if (endY >= _image->Height()) {
      startY -= endY - _image->Height();
      endY = _image->Height();
    }
  }
  return {startY, endY};
}

size_t HeatMap::ImageToSurfaceXFactor() const {
  const std::array<size_t, 2> xRange = ImageXRange();
  const size_t imageWidth = xRange[1] - xRange[0];
  return (imageWidth + 29999) / 30000;
}

size_t HeatMap::ImageToSurfaceYFactor() const {
  const std::array<size_t, 2> yRange = ImageYRange();
  const size_t imageHeight = yRange[1] - yRange[0];
  return (imageHeight + 29999) / 30000;
}

bool HeatMap::ConvertToPlot(double screenX, double screenY, double& posX,
                            double& posY) const {
  if (HasImage()) {
    const size_t startX = (size_t)std::round(XZoomStart() * _image->Width());
    const size_t startY = (size_t)std::round(YZoomStart() * _image->Height());
    const size_t endX =
        std::max(startX + 1, (size_t)std::round(XZoomEnd() * _image->Width()));
    const size_t endY =
        std::max(startY + 1, (size_t)std::round(YZoomEnd() * _image->Height()));
    const size_t width = endX - startX;
    const size_t height = endY - startY;
    int pixelX = (int)std::round(
        (screenX - _leftBorderSize) * width /
            (_initializedWidth - _rightBorderSize - _leftBorderSize) -
        0.5);
    int pixelY = (int)std::round(
        (screenY - _topBorderSize) * height /
            (_initializedHeight - _bottomBorderSize - _topBorderSize) -
        0.5);
    const bool inDomain = pixelX >= 0 && pixelY >= 0 && pixelX < (int)width &&
                          pixelY < (int)height;
    // clamp inside domain
    pixelX = std::max(0, std::min(int(width) - 1, pixelX));
    pixelY = height - 1 - std::max(0, std::min(int(height) - 1, pixelY));
    posX = _horizontalScale.AxisToUnit(double(pixelX) / width);
    posY = _verticalScale.AxisToUnit(double(pixelY) / height);
    return inDomain;
  } else {
    posX = 0.0;
    posY = 0.0;
    return false;
  }
}

bool HeatMap::ConvertToScreen(double posX, double posY, double& screenX,
                              double& screenY) const {
  if (HasImage()) {
    const size_t startX = (size_t)std::round(XZoomStart() * _image->Width());
    const size_t startY = (size_t)std::round(YZoomStart() * _image->Height());
    const size_t endX =
        std::max(startX + 1, (size_t)std::round(XZoomEnd() * _image->Width()));
    const size_t endY =
        std::max(startY + 1, (size_t)std::round(YZoomEnd() * _image->Height()));
    const size_t width = endX - startX;
    const size_t height = endY - startY;
    posX = _horizontalScale.UnitToAxis(posX) * width;
    posY = height - 1 - _verticalScale.UnitToAxis(posY) * height;
    screenX = (posX + 0.5) *
                  (_initializedWidth - _rightBorderSize - _leftBorderSize) /
                  width +
              _leftBorderSize;
    screenY = (posY + 0.5) *
                  (_initializedHeight - _bottomBorderSize - _topBorderSize) /
                  height +
              _topBorderSize;
    return true;
  } else {
    return false;
  }
}

bool HeatMap::UnitToImage(double posX, double posY, size_t& x,
                          size_t& y) const {
  if (HasImage()) {
    const size_t startX = (size_t)std::round(XZoomStart() * _image->Width());
    const size_t startY = (size_t)std::round(YZoomStart() * _image->Height());
    const size_t endX =
        std::max(startX + 1, (size_t)std::round(XZoomEnd() * _image->Width()));
    const size_t endY =
        std::max(startY + 1, (size_t)std::round(YZoomEnd() * _image->Height()));
    const size_t width = endX - startX;
    const size_t height = endY - startY;
    const double xf = _horizontalScale.UnitToAxis(posX) * width + startX;
    const double yf = _verticalScale.UnitToAxis(posY) * height + startY;
    if (xf >= 0.0 && xf < _image->Width() && yf >= 0.0 &&
        yf < _image->Height()) {
      x = std::round(xf);
      y = std::round(yf);
      return true;
    }
  }
  return false;
}

void HeatMap::ImageToUnit(size_t x, size_t y, double& posX,
                          double& posY) const {
  if (HasImage()) {
    const size_t startX = (size_t)std::round(XZoomStart() * _image->Width());
    const size_t startY = (size_t)std::round(YZoomStart() * _image->Height());
    const size_t endX =
        std::max(startX + 1, (size_t)std::round(XZoomEnd() * _image->Width()));
    const size_t endY =
        std::max(startY + 1, (size_t)std::round(YZoomEnd() * _image->Height()));
    const size_t width = endX - startX;
    const size_t height = endY - startY;
    posX = _horizontalScale.AxisToUnit((double(x) - startX) / width);
    posY = _verticalScale.AxisToUnit((double(y) - startY) / height);
  }
}
