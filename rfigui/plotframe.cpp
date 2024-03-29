#include <limits>

#include "../plot/xyplot.h"

#include "plotframe.h"

using aocommon::Polarization;

PlotFrame::PlotFrame()
    : _plotData(nullptr),
      _selectedXStart(0),
      _selectedYStart(0),
      _selectedXEnd(0),
      _selectedYEnd(0) {
  pack_start(_plot);
  _plot.show();
}

PlotFrame::~PlotFrame() { delete _plotData; }

void PlotFrame::plot() {
  _plot.Clear();
  if (_plotData != nullptr) delete _plotData;
  _plotData = new XYPlot();

  bool drawn = false;
  if (_data.HasXX()) {
    plotTimeGraph(_data, "XX", Polarization::XX);
    drawn = true;
  }
  if (_data.HasXY()) {
    plotTimeGraph(_data, "XY", Polarization::XY);
    drawn = true;
  }
  if (_data.HasYX()) {
    plotTimeGraph(_data, "YX", Polarization::YX);
    drawn = true;
  }
  if (_data.HasYY()) {
    plotTimeGraph(_data, "YY", Polarization::YY);
    drawn = true;
  }

  if (_data.PolarizationCount() > 0 &&
      _data.GetPolarization(0) == Polarization::StokesI) {
    plotTimeGraph(_data, "Stokes I");
  } else if (!drawn) {
    plotTimeGraph(_data, "Data");
  }

  _plot.SetPlot(*_plotData);
}

void PlotFrame::plotTimeGraph(const TimeFrequencyData& data,
                              const std::string& label,
                              aocommon::PolarizationEnum polarisation) {
  plotTimeGraph(data.Make(polarisation), label);
}

void PlotFrame::plotTimeGraph(const TimeFrequencyData& data,
                              const std::string& label) {
  _plotData->StartLine(label);
  const Image2DCPtr image = data.GetSingleImage();
  const Mask2DCPtr mask = data.GetSingleMask();

  for (size_t x = 0; x < image->Width(); ++x) {
    size_t count = 0;
    num_t value = 0.0;

    for (size_t y = _selectedYStart; y < _selectedYEnd; ++y) {
      if (!mask->Value(x, y)) {
        ++count;
        value += image->Value(x, y);
      }
    }
    if (count > 0)
      _plotData->PushDataPoint(x, value / (num_t)count);
    else
      _plotData->PushDataPoint(x, std::numeric_limits<num_t>::quiet_NaN());
  }
}
