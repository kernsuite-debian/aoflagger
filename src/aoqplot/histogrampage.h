#ifndef GUI_QUALITY__HISTOGRAMPAGE_H
#define GUI_QUALITY__HISTOGRAMPAGE_H

#include <string>
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/expander.h>
#include <gtkmm/frame.h>
#include <gtkmm/textview.h>
#include <gtkmm/radiobutton.h>

#include "../plot/plotwidget.h"

#include "plotsheet.h"

class HistogramPage : public PlotSheet {
	public:
		HistogramPage(class HistogramPageController* controller);
    ~HistogramPage();

		void updatePlot();
		
		void Redraw() { _plotWidget.Update(); }
		
		void SetSlopeFrame(const std::string& str);
		void SetFitText(const std::string& str)
		{
			_fitTextView.get_buffer()->set_text(str);
		}
	private:
		void onPlotPropertiesClicked();
		void onDataExportClicked();
		void updateSlopeFrame(const class LogHistogram &histogram);
		std::string SlopeText(std::stringstream &str, const LogHistogram &histogram, bool updateRange);
		void updateDataWindow();
		
		void onAutoRangeClicked()
		{
			bool autoRange = _fitAutoRangeButton.get_active();
			_fitStartEntry.set_sensitive(!autoRange);
			_fitEndEntry.set_sensitive(!autoRange);
			if(autoRange)
				updatePlot();
		}
		
		void onSlopeAutoRangeClicked()
		{
			bool autoRange = _slopeAutoRangeButton.get_active();
			_slopeStartEntry.set_sensitive(!autoRange);
			_slopeEndEntry.set_sensitive(!autoRange);
			if(autoRange)
				updatePlot();
		}
		
		class HistogramPageController* _controller;
		
		Gtk::Expander _expander;
		Gtk::VBox _sideBox;
		
		Gtk::Frame _histogramTypeFrame;
		Gtk::VBox _histogramTypeBox;
		Gtk::CheckButton _totalHistogramButton, _rfiHistogramButton, _notRFIHistogramButton;
		
		Gtk::Frame _polarizationFrame;
		Gtk::VBox _polarizationBox;
		Gtk::CheckButton _xxPolarizationButton, _xyPolarizationButton, _yxPolarizationButton, _yyPolarizationButton, _sumPolarizationButton;
		
		Gtk::Frame _fitFrame;
		Gtk::VBox _fitBox;
		Gtk::CheckButton _fitButton, _subtractFitButton, _fitLogarithmicButton, _fitAutoRangeButton;
		Gtk::Entry _fitStartEntry, _fitEndEntry;
		Gtk::TextView _fitTextView;
		
		Gtk::Frame _functionFrame;
		Gtk::VBox _functionBox;
		Gtk::RadioButton _nsButton, _dndsButton;
		Gtk::Entry _deltaSEntry;
		Gtk::CheckButton _staircaseFunctionButton, _normalizeButton;
		
		Gtk::Button _plotPropertiesButton, _dataExportButton;
		
		Gtk::Frame _slopeFrame;
		Gtk::VBox _slopeBox;
		Gtk::TextView _slopeTextView;
		Gtk::CheckButton _drawSlopeButton, _drawSlope2Button;
		Gtk::CheckButton _slopeAutoRangeButton;
		Gtk::Entry _slopeStartEntry, _slopeEndEntry, _slopeRFIRatio;
		
		PlotWidget _plotWidget;
		class PlotPropertiesWindow *_plotPropertiesWindow;
		class DataWindow *_dataWindow;
};

#endif
