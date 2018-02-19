#ifndef GUI___HISTOGRAMWINDOW_H
#define GUI___HISTOGRAMWINDOW_H

#include "../aoqplot/histogrampage.h"

#include "../aoqplot/controllers/histogrampagecontroller.h"

#include <gtkmm/window.h>

#include "../quality/histogramcollection.h"

class HistogramWindow : public Gtk::Window
{
	public:
		explicit HistogramWindow(const HistogramCollection &histograms) :
			_histogramPage(&_controller)
		{
			_controller.SetHistograms(&histograms);
			add(_histogramPage);
			_histogramPage.show();
		}
		void SetStatistics(const HistogramCollection &histograms)
		{
			_controller.SetHistograms(&histograms);
		}
	private:
		HistogramPageController _controller;
		HistogramPage _histogramPage;
};

#endif
