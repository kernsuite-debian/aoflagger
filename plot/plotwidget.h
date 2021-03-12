#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <gtkmm/drawingarea.h>

#include "plotbase.h"

class PlotWidget : public Gtk::DrawingArea {
 public:
  PlotWidget() : _plot(nullptr) {
    signal_draw().connect(sigc::mem_fun(*this, &PlotWidget::onDraw));
    set_size_request(300, 200);
  }

  ~PlotWidget() { _linkedRedrawConnection.disconnect(); }

  PlotBase &Plot() const { return *_plot; }

  void SetPlot(PlotBase &plot) {
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

  void Update() { queue_draw(); }

 private:
  PlotBase *_plot;
  sigc::connection _linkedRedrawConnection;

  bool onDraw(const Cairo::RefPtr<Cairo::Context> &cr) {
    Glib::RefPtr<Gdk::Window> window = get_window();
    if (window && get_width() > 0 && get_height() > 0) {
      if (_plot != nullptr)
        _plot->Draw(cr, get_width(), get_height());
      else {
        cr->set_source_rgba(1, 1, 1, 1);
        cr->paint();
        cr->fill();
      }
    }
    return true;
  }
};

#endif
