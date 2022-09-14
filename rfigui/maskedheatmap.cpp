#include "maskedheatmap.h"

#include "../algorithms/thresholdconfig.h"

#include <boost/algorithm/string.hpp>

#include <fstream>

using algorithms::ThresholdConfig;

MaskedHeatMap::MaskedHeatMap()
    : _showOriginalMask(true),
      _showAlternativeMask(true),
      _highlighting(false),
      _highlightConfig(new ThresholdConfig()),
      _manualXAxisDescription(false),
      _manualYAxisDescription(false),
      _manualZAxisDescription(false) {
  _highlightConfig->InitializeLengthsSingleSample();
  SetXAxisType(AxisType::kTime);
  SetCairoFilter(Cairo::FILTER_NEAREST);
  SignalDrawImage().connect(
      [&](const Cairo::RefPtr<Cairo::ImageSurface>& surface) {
        signalDrawImage(surface);
      });
}

MaskedHeatMap::~MaskedHeatMap() {}

void MaskedHeatMap::Clear() {
  _originalMask.reset();
  _alternativeMask.reset();
  _segmentedImage.reset();
  _highlightConfig.reset(new ThresholdConfig());
  _highlightConfig->InitializeLengthsSingleSample();
  _metaData.reset();
  HeatMap::Clear();
}

Mask2DCPtr MaskedHeatMap::GetActiveMask() const {
  if (!HasImage())
    throw std::runtime_error("GetActiveMask() called without image");
  const bool originalActive = _showOriginalMask && _originalMask;
  const bool altActive = _showAlternativeMask && _alternativeMask;
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
      return Mask2D::CreateSetMaskPtr<false>(Image().Width(), Image().Height());
  }
}

TimeFrequencyMetaDataCPtr MaskedHeatMap::GetSelectedMetaData() const {
  TimeFrequencyMetaDataCPtr metaData = _metaData;

  if (metaData) {
    if (XZoomStart() != 0 && metaData->HasObservationTimes()) {
      size_t startTime = std::round(XZoomStart() * Image().Width());
      TimeFrequencyMetaData* newData = new TimeFrequencyMetaData(*metaData);
      metaData = TimeFrequencyMetaDataCPtr(newData);
      std::vector<double> obsTimes = newData->ObservationTimes();
      obsTimes.erase(obsTimes.begin(), obsTimes.begin() + startTime);
      newData->SetObservationTimes(obsTimes);
    }
    if (YZoomStart() != 0 && metaData->HasBand()) {
      size_t startChannel = std::round(YZoomStart() * Image().Height());
      TimeFrequencyMetaData* newData = new TimeFrequencyMetaData(*metaData);
      metaData = TimeFrequencyMetaDataCPtr(newData);
      BandInfo band = newData->Band();
      band.channels.erase(band.channels.begin(),
                          band.channels.begin() + startChannel);
      newData->SetBand(band);
    }
  }
  return metaData;
}

void MaskedHeatMap::SetMetaData(const TimeFrequencyMetaDataCPtr& metaData) {
  _metaData = metaData;

  if (_metaData) {
    if (_metaData->HasObservationTimes()) {
      SetXAxisMin(_metaData->ObservationTimes().front());
      SetXAxisMax(_metaData->ObservationTimes().back());
      SetXAxisDescription("Time (UTC, hh:mm:ss)");
    }

    if (_metaData->HasBand()) {
      SetYAxisMin(_metaData->Band().channels.front().frequencyHz * 1e-6);
      SetYAxisMax(_metaData->Band().channels.back().frequencyHz * 1e-6);
      SetYAxisDescription("Frequency (MHz)");
    }

    if (!_manualZAxisDescription && !_metaData->ValueDescription().empty()) {
      std::string description = _metaData->ValueDescription();
      if (!_metaData->ValueUnits().empty())
        description = description + " (" + _metaData->ValueUnits() + ")";
      SetZAxisDescription(description);
    }
  }
}

void MaskedHeatMap::signalDrawImage(
    const Cairo::RefPtr<Cairo::ImageSurface>& surface) const {
  Mask2DCPtr mask = GetActiveMask();
  Mask2DCPtr originalMask = _originalMask;
  Mask2DCPtr alternativeMask = _alternativeMask;
  Mask2DPtr highlightMask;
  if (_highlighting) {
    highlightMask =
        Mask2D::CreateSetMaskPtr<false>(Image().Width(), Image().Height());
    _highlightConfig->Execute(GetImage2D().get(), highlightMask.get(), true,
                              10.0, 10.0);
  }

  const size_t xFactor = ImageToSurfaceXFactor();
  if (xFactor > 1) {
    mask = Mask2D::MakePtr(mask->ShrinkHorizontally(xFactor));
    if (originalMask) {
      originalMask = Mask2D::MakePtr(originalMask->ShrinkHorizontally(xFactor));
    }
    if (alternativeMask) {
      alternativeMask =
          Mask2D::MakePtr(alternativeMask->ShrinkHorizontally(xFactor));
    }
    if (highlightMask) {
      highlightMask =
          Mask2D::MakePtr(highlightMask->ShrinkHorizontally(xFactor));
    }
  }

  const size_t yFactor = ImageToSurfaceYFactor();
  if (yFactor > 1) {
    mask = Mask2D::MakePtr(mask->ShrinkVertically(yFactor));
    if (originalMask) {
      originalMask = Mask2D::MakePtr(originalMask->ShrinkVertically(yFactor));
    }
    if (alternativeMask) {
      alternativeMask =
          Mask2D::MakePtr(alternativeMask->ShrinkVertically(yFactor));
    }
    if (highlightMask) {
      highlightMask = Mask2D::MakePtr(highlightMask->ShrinkVertically(yFactor));
    }
  }

  const bool originalActive = _showOriginalMask && originalMask;
  const bool altActive = _showAlternativeMask && alternativeMask;
  int orMaskR, orMaskG, orMaskB;
  int altMaskR, altMaskG, altMaskB;
  if (GetColorMap() == ColorMap::Viridis) {
    orMaskR = 0;
    orMaskG = 0;
    orMaskB = 0;
    altMaskR = 255;
    altMaskG = 255;
    altMaskB = 255;
  } else {
    orMaskR = 255;
    orMaskG = 0;
    orMaskB = 255;
    altMaskR = 255;
    altMaskG = 255;
    altMaskB = 0;
  }

  unsigned char* data = surface->get_data();
  const size_t rowStride = surface->get_stride();

  const std::array<size_t, 2> imageXRange = ImageXRange();
  const std::array<size_t, 2> imageYRange = ImageYRange();
  const std::array<size_t, 2> surfaceXRange = {imageXRange[0] / xFactor,
                                               imageXRange[1] / xFactor};
  const std::array<size_t, 2> surfaceYRange = {imageYRange[0] / yFactor,
                                               imageYRange[1] / yFactor};

  if (_highlighting || originalActive || altActive) {
    for (size_t y = surfaceYRange[0]; y != surfaceYRange[1]; ++y) {
      guint8* rowpointer = data + rowStride * (surfaceYRange[1] - y - 1);
      for (size_t x = surfaceXRange[0]; x != surfaceXRange[1]; ++x) {
        const size_t xa = (x - surfaceXRange[0]) * 4;
        if (_highlighting && highlightMask->Value(x, y)) {
          rowpointer[xa + 0] = 0;
          rowpointer[xa + 1] = 0;
          rowpointer[xa + 2] = 255;
          rowpointer[xa + 3] = 255;
        } else if (originalActive && originalMask->Value(x, y)) {
          rowpointer[xa + 0] = orMaskB;
          rowpointer[xa + 1] = orMaskG;
          rowpointer[xa + 2] = orMaskR;
          rowpointer[xa + 3] = 255;
        } else if (altActive && alternativeMask->Value(x, y)) {
          rowpointer[xa + 0] = altMaskB;
          rowpointer[xa + 1] = altMaskG;
          rowpointer[xa + 2] = altMaskR;
          rowpointer[xa + 3] = 255;
        }
      }
    }
  }

  if (xFactor == 1 && yFactor == 1 && _segmentedImage != nullptr) {
    for (size_t y = imageYRange[0]; y < imageXRange[1]; ++y) {
      guint8* rowpointer = data + rowStride * (imageXRange[1] - y - 1);
      for (size_t x = imageXRange[0]; x < imageXRange[1]; ++x) {
        if (_segmentedImage->Value(x, y) != 0) {
          int xa = (x - imageXRange[0]) * 4;
          rowpointer[xa] = IntMap::R(_segmentedImage->Value(x, y));
          rowpointer[xa + 1] = IntMap::G(_segmentedImage->Value(x, y));
          rowpointer[xa + 2] = IntMap::B(_segmentedImage->Value(x, y));
          rowpointer[xa + 3] = IntMap::A(_segmentedImage->Value(x, y));
        }
      }
    }
  }
}

void MaskedHeatMap::SaveByExtension(const std::string& filename, size_t width,
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

void MaskedHeatMap::SaveText(const std::string& filename) const {
  if (HasImage()) {
    const Image2DCPtr image = GetImage2D();
    const size_t startX = (size_t)std::round(XZoomStart() * image->Width());
    const size_t startY = (size_t)std::round(YZoomStart() * image->Height());
    const size_t endX = (size_t)std::round(XZoomEnd() * image->Width());
    const size_t endY = (size_t)std::round(YZoomEnd() * image->Height());
    const size_t imageWidth = endX - startX;
    const size_t imageHeight = endY - startY;
    std::ofstream file(filename.c_str());
    file << imageWidth << '\n' << imageHeight << '\n';
    for (size_t y = startY; y != endY; ++y) {
      for (size_t x = startX; x != endX; ++x) {
        file << image->Value(x, y) << '\n';
      }
    }
  }
}
