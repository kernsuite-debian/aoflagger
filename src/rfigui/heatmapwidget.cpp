#include "heatmapwidget.h"

HeatMapWidget::HeatMapWidget(HeatMapPlot* plot) :
	_mouseIsIn(false),
	_plot(plot)
{
	add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_RELEASE_MASK |
		   Gdk::BUTTON_PRESS_MASK | Gdk::LEAVE_NOTIFY_MASK);
	signal_motion_notify_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onMotion));
	signal_leave_notify_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onLeave));
	signal_button_release_event().connect(sigc::mem_fun(*this, &HeatMapWidget::onButtonReleased));
	signal_draw().connect(sigc::mem_fun(*this, &HeatMapWidget::onDraw) );
}

bool HeatMapWidget::onDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Glib::RefPtr<Gdk::Window> window = get_window();
	if(window && get_width() > 0 && get_height() > 0)
	{
		_plot->Draw(cr, get_width(), get_height(), false);
	}
	window->invalidate(false);
	return true;
}

void HeatMapWidget::Update() {
	Glib::RefPtr<Gdk::Window> window = get_window();
	if(window && get_width() > 0 && get_height() > 0)
	{
		_plot->Draw(get_window()->create_cairo_context(), get_width(), get_height(), true);
	}
}

bool HeatMapWidget::onMotion(GdkEventMotion *event)
{
	if(_plot->HasImage())
	{
		int posX, posY;
		if(_plot->ConvertToUnits(event->x, event->y, posX, posY))
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

bool HeatMapWidget::onButtonReleased(GdkEventButton *event)
{
	if(_plot->HasImage())
	{
		int posX, posY;
		if(_plot->ConvertToUnits(event->x, event->y, posX, posY))
			_onButtonReleased(posX, posY);
	}
	return true;
}
