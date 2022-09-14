#ifndef PLOT_PLOTBASE_H
#define PLOT_PLOTBASE_H

#include "horizontalplotscale.h"
#include "verticalplotscale.h"

class PlotBase {
 public:
  PlotBase();

  virtual ~PlotBase() { UnlinkHorizontally(); }

  void Draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
            size_t height) {
    _width = width;
    _height = height;
    draw(cairo, width, height);
  }

  void LinkHorizontally(PlotBase& other) {
    _horizontalScale.Link(other._horizontalScale);
    _horiScale2.Link(other._horiScale2);
  }

  void UnlinkHorizontally() { _horizontalScale.Unlink(); }

  void LinkVertically(PlotBase& other) {
    _verticalScale.Link(other._verticalScale);
    _vertScale2.Link(other._vertScale2);
  }

  sigc::signal<void>& SignalLinkedRedraw() { return _signalLinkedRedraw; }

  virtual bool ConvertToPlot(double screenX, double screenY, double& posX,
                             double& posY) const = 0;
  virtual bool ConvertToScreen(double posX, double posY, double& screenX,
                               double& screenY) const = 0;

  void ZoomFit();
  void ZoomIn();
  /**
   * Zoom in to a given position, as done when zooming with the scroll wheel.
   * The position is in plot units.
   */
  void ZoomInOn(double x, double y);
  void ZoomOut();
  /**
   * Zoom in to a given rectangle. The positions are in plot units.
   */
  void ZoomTo(double x1, double y1, double x2, double y2);
  /**
   * Move the view by the given displacement. The displacement are in
   * display units.
   */
  void Pan(double xDisplacement, double yDisplacement);
  bool IsZoomedOut() const {
    return _xZoomStart == 0.0 && _xZoomEnd == 1.0 && _yZoomStart == 0.0 &&
           _yZoomEnd == 1.0;
  }
  sigc::signal<void>& OnZoomChanged() { return _onZoomChanged; }

  double XZoomStart() const { return _xZoomStart; }
  double XZoomEnd() const { return _xZoomEnd; }
  double YZoomStart() const { return _yZoomStart; }
  double YZoomEnd() const { return _yZoomEnd; }

  virtual void SavePdf(const std::string& filename, size_t width,
                       size_t height) = 0;
  virtual void SaveSvg(const std::string& filename, size_t width,
                       size_t height) = 0;
  virtual void SavePng(const std::string& filename, size_t width,
                       size_t height) = 0;

 protected:
  struct Rectangle {
    double x, y;
    double width, height;
  };

  virtual Rectangle getPlotArea(size_t width, size_t height) const = 0;

  HorizontalPlotScale _horizontalScale;
  VerticalPlotScale _verticalScale;
  HorizontalPlotScale _horiScale2;
  VerticalPlotScale _vertScale2;

 protected:
  virtual void draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
                    size_t height) = 0;

 private:
  std::shared_ptr<std::vector<PlotBase*>> _verticallyLinked;
  sigc::signal<void> _signalLinkedRedraw;
  sigc::signal<void> _onZoomChanged;

  /**
   * Values between 0.0 and 1.0 indication the zoom.
   * @{
   */
  double _xZoomStart, _xZoomEnd;
  double _yZoomStart, _yZoomEnd;
  size_t _width, _height;
  /** @} */
};

#endif  // PLOT_PLOTBASE_H
