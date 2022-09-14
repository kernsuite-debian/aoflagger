#include "plotwidget.h"

PlotWidget::PlotWidget()
    : _plot(nullptr),
      _mouseIsIn(false),
      _mouseX(0),
      _mouseY(0),
      _isButtonPressed(false),
      _isZooming(false),
      _isPanning(false),
      _bpressStartX(0),
      _bpressStartY(0) {
  add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_RELEASE_MASK |
             Gdk::BUTTON_PRESS_MASK | Gdk::LEAVE_NOTIFY_MASK |
             Gdk::SCROLL_MASK);
  signal_motion_notify_event().connect(
      sigc::mem_fun(*this, &PlotWidget::onMotion));
  signal_leave_notify_event().connect(
      sigc::mem_fun(*this, &PlotWidget::onLeave));
  signal_button_press_event().connect(
      sigc::mem_fun(*this, &PlotWidget::onButtonPress));
  signal_button_release_event().connect(
      sigc::mem_fun(*this, &PlotWidget::onButtonRelease));
  signal_scroll_event().connect(sigc::mem_fun(*this, &PlotWidget::onScroll));
  signal_draw().connect(sigc::mem_fun(*this, &PlotWidget::onDraw));
  set_size_request(300, 200);
}

PlotWidget::~PlotWidget() { _linkedRedrawConnection.disconnect(); }

bool PlotWidget::onDraw(const Cairo::RefPtr<Cairo::Context>& cr) {
  _beforeDrawSignal();
  Glib::RefPtr<Gdk::Window> window = get_window();
  if (window && get_width() > 0 && get_height() > 0) {
    if (_plot != nullptr) {
      _plot->Draw(cr, get_width(), get_height());
      if (_isZooming) {
        cr->set_line_width(1.0);
        cr->rectangle(_bpressStartX, _bpressStartY, _mouseX - _bpressStartX,
                      _mouseY - _bpressStartY);
        cr->set_source_rgba(0.35, 0.35, 1.0, 0.4);
        cr->fill_preserve();
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->stroke();
      }
    } else {
      cr->set_source_rgba(1, 1, 1, 1);
      cr->paint();
      cr->fill();
    }
  }
  return true;
}

void PlotWidget::Update() { queue_draw(); }

bool PlotWidget::onMotion(GdkEventMotion* event) {
  if (_plot) {
    double posX, posY;
    bool isInside = _plot->ConvertToPlot(event->x, event->y, posX, posY);
    if (_isZooming) {
      _mouseX = event->x;
      _mouseY = event->y;
      Update();
    } else if (_isPanning) {
      _plot->Pan(event->x - _mouseX, event->y - _mouseY);
      _mouseX = event->x;
      _mouseY = event->y;
      Update();
    } else {
      if (isInside) {
        _mouseX = event->x;
        _mouseY = event->y;
        _mouseIsIn = true;
        _onMouseMoved(posX, posY);
      } else if (_mouseIsIn) {
        _onMouseLeft();
        _mouseIsIn = false;
      }
    }
  }
  return true;
}

bool PlotWidget::onLeave(GdkEventCrossing* /*event*/) {
  if (_mouseIsIn) {
    _onMouseLeft();
    _mouseIsIn = false;
  }
  return true;
}

bool PlotWidget::onButtonPress(GdkEventButton* event) {
  _isButtonPressed = true;
  if (_plot) {
    double posX, posY;
    if (_plot->ConvertToPlot(event->x, event->y, posX, posY)) {
      _mouseX = event->x;
      _mouseY = event->y;
      _bpressStartX = _mouseX;
      _bpressStartY = _mouseY;
      if (event->button == 1)  // left
        _isZooming = true;
      else if (event->button == 3)  // right
        _isPanning = true;
    }
  }
  return true;
}

bool PlotWidget::onButtonRelease(GdkEventButton* event) {
  _isButtonPressed = false;
  if (_plot) {
    const double oldMouseX = _mouseX;
    const double oldMouseY = _mouseY;
    double posX, posY;
    if (_plot->ConvertToPlot(event->x, event->y, posX, posY)) {
      _mouseX = event->x;
      _mouseY = event->y;
      _onButtonReleased(posX, posY);
    }
    if (_isZooming) {
      _isZooming = false;
      if (_bpressStartX != _mouseX || _bpressStartY != _mouseY) {
        double startX, startY;
        _plot->ConvertToPlot(_bpressStartX, _bpressStartY, startX, startY);
        _plot->ZoomTo(startX, startY, posX, posY);
      }
    }
    if (_isPanning) {
      _isPanning = false;
      _plot->Pan(oldMouseX - event->x, oldMouseY - event->y);
    }

    Update();
  }
  return true;
}

bool PlotWidget::onScroll(GdkEventScroll* event) {
  if (_plot) {
    double posX, posY;
    if (_plot->ConvertToPlot(event->x, event->y, posX, posY)) {
      int direction = 0;
      if (event->direction == GDK_SCROLL_UP)
        direction = -1;
      else if (event->direction == GDK_SCROLL_DOWN)
        direction = 1;
      _onScroll(posX, posY, direction);
    }
  }
  return true;
}
