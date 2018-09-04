#include "heatmapwidget.h"

HeatMapWidget::HeatMapWidget(HeatMapPlot* plot) :
	_invalidated(true),
	_mouseIsIn(false),
	_mouseX(0), _mouseY(0),
	_isButtonPressed(false),
	_isZooming(false), _isPanning(false),
	_bpressStartX(0), _bpressStartY(0),
	_plot(plot)
{
	add_events(
		Gdk::POINTER_MOTION_MASK |
		Gdk::BUTTON_RELEASE_MASK |
		Gdk::BUTTON_PRESS_MASK |
		Gdk::LEAVE_NOTIFY_MASK |
		Gdk::SCROLL_MASK);
	signal_motion_notify_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onMotion));
	signal_leave_notify_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onLeave));
	signal_button_press_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onButtonPress));
	signal_button_release_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onButtonRelease));
	signal_scroll_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onScroll));
	signal_draw().connect(sigc::mem_fun(*this, &HeatMapWidget::onDraw) );
}

bool HeatMapWidget::onDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Glib::RefPtr<Gdk::Window> window = get_window();
	if(window && get_width() > 0 && get_height() > 0)
	{
		_plot->Draw(cr, get_width(), get_height(), _invalidated);
		if(_isZooming)
		{
			double x1, y1, x2, y2;
			_plot->ConvertToScreen(_bpressStartX, _bpressStartY, x1, y1);
			_plot->ConvertToScreen(_mouseX, _mouseY, x2, y2);
			cr->set_line_width(1.0);
			cr->rectangle(x1, y1, x2-x1, y2-y1);
			cr->set_source_rgba(0.35, 0.35, 1.0, 0.4);
			cr->fill_preserve();
			cr->set_source_rgb(0.0, 0.0, 0.0);
			cr->stroke();
		}
	}
	return true;
}

void HeatMapWidget::update(bool invalidated) {
	Glib::RefPtr<Gdk::Window> window = get_window();
	if(window && get_width() > 0 && get_height() > 0)
	{
		_invalidated = _invalidated || invalidated;
		queue_draw();
	}
}

bool HeatMapWidget::onMotion(GdkEventMotion *event)
{
	if(_plot->HasImage())
	{
		int posX, posY;
		bool isInside = _plot->ConvertToUnits(event->x, event->y, posX, posY);
		if(_isZooming)
		{
			_mouseX = posX;
			_mouseY = posY;
			update(false);
		}
		else if(_isPanning)
		{
			int
				panX = _mouseX-posX,
				panY = _mouseY-posY;
			_plot->Pan(panX, panY);
			_mouseX = posX + panX;
			_mouseY = posY + panY;
			update(true);
		}
		else {
			if(isInside)
			{
				_mouseX = posX;
				_mouseY = posY;
				_mouseIsIn = true;
				_onMouseMoved(posX, posY);
			} else if(_mouseIsIn) {
				_onMouseLeft();
				_mouseIsIn = false;
			}
		}
	}
	return true;
}

bool HeatMapWidget::onLeave(GdkEventCrossing *event)
{
	if(_mouseIsIn)
	{
		_onMouseLeft();
		_mouseIsIn = false;
	}
	return true;
}

bool HeatMapWidget::onButtonPress(GdkEventButton* event)
{
	_isButtonPressed = true;
	if(_plot->HasImage())
	{
		int posX, posY;
		if(_plot->ConvertToUnits(event->x, event->y, posX, posY))
		{
			_mouseX = posX;
			_mouseY = posY;
			_bpressStartX = posX;
			_bpressStartY = posY;
			if(event->button == 1) // left
				_isZooming = true;
			else if(event->button == 3) // right
				_isPanning = true;
		}
	}
	return true;
}

bool HeatMapWidget::onButtonRelease(GdkEventButton *event)
{
	_isButtonPressed = false;
	if(_plot->HasImage())
	{
		int oldMouseX = _mouseX, oldMouseY = _mouseY;
		int posX, posY;
		if(_plot->ConvertToUnits(event->x, event->y, posX, posY))
		{
			_mouseX = posX;
			_mouseY = posY;
			_onButtonReleased(posX, posY);
		}
		if(_isZooming)
		{
			_isZooming = false;
			_plot->ZoomTo(_bpressStartX, _bpressStartY, _mouseX, _mouseY);
		}
		if(_isPanning)
		{
			_isPanning = false;
			_plot->Pan(oldMouseX-posX, oldMouseY-posY);
		}
		
		update(true);
	}
	return true;
}

bool HeatMapWidget::onScroll(GdkEventScroll* event)
{
	if(_plot->HasImage())
	{
		int posX, posY;
		if(_plot->ConvertToUnits(event->x, event->y, posX, posY))
		{
			int direction = 0;
			if(event->direction == GDK_SCROLL_UP)
				direction = -1;
			else if(event->direction == GDK_SCROLL_DOWN)
				direction = 1;
			_onScroll(posX, posY, direction);
		}
	}
	return true;
}
