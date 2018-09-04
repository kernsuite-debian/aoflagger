#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <gtkmm/drawingarea.h>

#include "plotable.h"

class PlotWidget : public Gtk::DrawingArea {
	public:
		PlotWidget() : _plot(nullptr)
		{
			signal_draw().connect(sigc::mem_fun(*this, &PlotWidget::onDraw) );
			set_size_request(400, 300);
		}
		
		Plotable &Plot() const
		{
			return *_plot; 
		}
		void SetPlot(Plotable &plot)
		{
			_plot = &plot;
			queue_draw();
		}
		void Clear()
		{
			_plot = nullptr;
			queue_draw();
		}
		void Update()
		{
			queue_draw();
		}
	private:
		Plotable *_plot;

		bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr)
		{
			if(_plot != nullptr)
				_plot->Render(*this);
			else {
				cr->set_source_rgba(1, 1, 1, 1);
				cr->paint();
				cr->fill();
			}
			return true;
		}
};

#endif
