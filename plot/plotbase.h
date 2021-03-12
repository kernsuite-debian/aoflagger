#ifndef PLOT_PLOTBASE_H
#define PLOT_PLOTBASE_H

#include "horizontalplotscale.h"
#include "verticalplotscale.h"

class PlotBase {
 public:
  PlotBase() {
    _horizontalScale.SignalLinkedRedraw().connect(_signalLinkedRedraw);
  }

  virtual ~PlotBase() { UnlinkHorizontally(); }

  virtual void Draw(const Cairo::RefPtr<Cairo::Context>& cairo, size_t width,
                    size_t height) = 0;

  void LinkHorizontally(PlotBase& other) {
    _horizontalScale.Link(other._horizontalScale);
  }

  void UnlinkHorizontally() { _horizontalScale.Unlink(); }

  void LinkVertically(PlotBase& other) {
    _verticalScale.Link(other._verticalScale);
  }

  sigc::signal<void>& SignalLinkedRedraw() { return _signalLinkedRedraw; }

 protected:
  HorizontalPlotScale _horizontalScale;
  VerticalPlotScale _verticalScale;

 private:
  std::shared_ptr<std::vector<PlotBase*>> _verticallyLinked;
  sigc::signal<void> _signalLinkedRedraw;
};

#endif  // PLOT_PLOTBASE_H
