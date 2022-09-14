#ifndef PLOT_WIDGET_H
#define PLOT_WIDGET_H

#include <gtkmm/drawingarea.h>

#include "plotbase.h"

class PlotWidget : public Gtk::DrawingArea {
 public:
  PlotWidget();
  ~PlotWidget();

  sigc::signal<void()>& OnMouseLeaveEvent() { return _onMouseLeft; }
  sigc::signal<void(double, double)>& OnButtonReleasedEvent() {
    return _onButtonReleased;
  }
  sigc::signal<void(double, double, int)>& OnScrollEvent() { return _onScroll; }
  sigc::signal<void()>& BeforeDrawSignal() { return _beforeDrawSignal; }

  void Update();

  PlotBase& Plot() { return *_plot; }
  const PlotBase& Plot() const { return *_plot; }

  void SetPlot(PlotBase& plot) {
    _linkedRedrawConnection.disconnect();
    _plot = &plot;
    _linkedRedrawConnection = _plot->SignalLinkedRedraw().connect(
        sigc::mem_fun(*this, &PlotWidget::Update));
    queue_draw();
  }

  void Clear() {
    _linkedRedrawConnection.disconnect();
    _plot = nullptr;
    queue_draw();
  }

  sigc::signal<void(double, double)>& OnMouseMovedEvent() {
    return _onMouseMoved;
  }
  bool IsMouseInImage() const { return _mouseIsIn; }
  size_t MouseX() { return _mouseX; }
  size_t MouseY() { return _mouseY; }
  void SavePdf(const std::string& filename) {
    _plot->SavePdf(filename, get_width(), get_height());
  }
  void SaveSvg(const std::string& filename) {
    _plot->SaveSvg(filename, get_width(), get_height());
  }
  void SavePng(const std::string& filename) {
    _plot->SavePng(filename, get_width(), get_height());
  }

 private:
  bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr);
  bool onMotion(GdkEventMotion* event);
  bool onLeave(GdkEventCrossing* event);
  bool onButtonPress(GdkEventButton* event);
  bool onButtonRelease(GdkEventButton* event);
  bool onScroll(GdkEventScroll* event);

  PlotBase* _plot;
  bool _mouseIsIn;
  double _mouseX, _mouseY;
  bool _isButtonPressed, _isZooming, _isPanning;
  double _bpressStartX, _bpressStartY;

  sigc::signal<void(double, double)> _onMouseMoved;
  sigc::signal<void()> _onMouseLeft;
  sigc::signal<void(double, double)> _onButtonReleased;
  sigc::signal<void(double, double, int)> _onScroll;
  sigc::signal<void()> _beforeDrawSignal;
  sigc::connection _linkedRedrawConnection;
};

#endif
