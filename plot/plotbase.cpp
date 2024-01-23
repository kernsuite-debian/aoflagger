#include "plotbase.h"

PlotBase::PlotBase() {
  _horizontalScale.SignalLinkedRedraw().connect(_signalLinkedRedraw);
}

void PlotBase::ZoomFit() {
  _xZoomStart = 0.0;
  _xZoomEnd = 1.0;
  _yZoomStart = 0.0;
  _yZoomEnd = 1.0;
  _onZoomChanged.emit();
}

void PlotBase::ZoomIn() {
  const double distX = (_xZoomEnd - _xZoomStart) * 0.25;
  _xZoomStart += distX;
  _xZoomEnd -= distX;
  const double distY = (_yZoomEnd - _yZoomStart) * 0.25;
  _yZoomStart += distY;
  _yZoomEnd -= distY;
  _onZoomChanged.emit();
}

void PlotBase::ZoomInOn(double x, double y) {
  const double zoomWidth = _xZoomEnd - _xZoomStart;
  const double zoomHeight = _yZoomEnd - _yZoomStart;
  const double xr = _horizontalScale.UnitToAxis(x) * zoomWidth + _xZoomStart;
  const double yr = _verticalScale.UnitToAxis(y) * zoomHeight + _yZoomStart;
  const double distX = zoomWidth * 0.25;
  _xZoomStart = xr - distX;
  _xZoomEnd = xr + distX;
  if (_xZoomStart < 0.0) {
    _xZoomEnd -= _xZoomStart;
    _xZoomStart = 0.0;
  }
  if (_xZoomEnd > 1.0) {
    _xZoomStart -= _xZoomEnd - 1.0;
    _xZoomEnd = 1.0;
  }
  const double distY = zoomHeight * 0.25;
  _yZoomStart = yr - distY;
  _yZoomEnd = yr + distY;
  if (_yZoomStart < 0.0) {
    _yZoomEnd -= _yZoomStart;
    _yZoomStart = 0.0;
  }
  if (_yZoomEnd > 1.0) {
    _yZoomStart -= _yZoomEnd - 1.0;
    _yZoomEnd = 1.0;
  }
  _onZoomChanged.emit();
}

void PlotBase::ZoomOut() {
  if (!IsZoomedOut()) {
    const double distX = std::max(0.01, (_xZoomEnd - _xZoomStart) * 0.5);
    _xZoomStart -= distX;
    _xZoomEnd += distX;
    if (_xZoomStart < 0.0) {
      _xZoomEnd -= _xZoomStart;
      _xZoomStart = 0.0;
    }
    if (_xZoomEnd > 1.0) {
      _xZoomStart -= _xZoomEnd - 1.0;
      _xZoomEnd = 1.0;
    }
    if (_xZoomStart < 0.0) _xZoomStart = 0.0;

    const double distY = std::max(0.01, (_yZoomEnd - _yZoomStart) * 0.5);
    _yZoomStart -= distY;
    _yZoomEnd += distY;
    if (_yZoomStart < 0.0) {
      _yZoomEnd -= _yZoomStart;
      _yZoomStart = 0.0;
    }
    if (_yZoomEnd > 1.0) {
      _yZoomStart -= _yZoomEnd - 1.0;
      _yZoomEnd = 1.0;
    }
    if (_yZoomStart < 0.0) _yZoomStart = 0.0;
    _onZoomChanged.emit();
  }
}

void PlotBase::ZoomTo(double x1, double y1, double x2, double y2) {
  if (x1 > x2) std::swap(x1, x2);
  if (y1 > y2) std::swap(y1, y2);
  const double width = _xZoomEnd - _xZoomStart;
  const double height = _yZoomEnd - _yZoomStart;
  const double newX1 = width * _horizontalScale.UnitToAxis(x1) + _xZoomStart;
  const double newX2 = width * _horizontalScale.UnitToAxis(x2) + _xZoomStart;
  const double newY1 = height * _verticalScale.UnitToAxis(y1) + _yZoomStart;
  const double newY2 = height * _verticalScale.UnitToAxis(y2) + _yZoomStart;
  _xZoomStart = std::max(0.0, std::min(1.0, newX1));
  _xZoomEnd = std::max(0.0, std::min(1.0, newX2));
  _yZoomStart = std::max(0.0, std::min(1.0, newY1));
  _yZoomEnd = std::max(0.0, std::min(1.0, newY2));
  _onZoomChanged.emit();
}

void PlotBase::Pan(double xDisplacement, double yDisplacement) {
  const Rectangle plotArea = getPlotArea(_width, _height);
  const double xFraction = xDisplacement / plotArea.width;
  const double yFraction = yDisplacement / plotArea.height;
  double dx = -xFraction * (_xZoomEnd - _xZoomStart);
  double dy = yFraction * (_yZoomEnd - _yZoomStart);
  if (_xZoomStart + dx < 0.0) dx = -_xZoomStart;
  if (_yZoomStart + dy < 0.0) dy = -_yZoomStart;
  if (_xZoomEnd + dx > 1.0) dx = 1.0 - _xZoomEnd;
  if (_yZoomEnd + dy > 1.0) dy = 1.0 - _yZoomEnd;
  _xZoomStart += dx;
  _xZoomEnd += dx;
  _yZoomStart += dy;
  _yZoomEnd += dy;
  _onZoomChanged.emit();
}
