#ifndef AOQPLOT_WINDOW_H
#define AOQPLOT_WINDOW_H

#include <gtkmm/application.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/menu.h>
#include <gtkmm/menutoolbutton.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/window.h>

#include "../rfigui/heatmapwidget.h"

#include "../quality/qualitytablesformatter.h"

#include "plotsheet.h"
#include "openoptionswindow.h"

#include "../structures/antennainfo.h"

#include "controllers/aoqpagecontroller.h"

class AOQPlotWindow : public Gtk::Window {
	public:
		AOQPlotWindow(class AOQPlotController* controller);
    
		void Open(const std::vector<std::string>& files);
		void Open(const std::string& file)
		{
			std::vector<std::string> files(1);
			files[0] = file;
			Open(files);
		}
		void SetStatus(const std::string &newStatus)
		{
			onStatusChange(newStatus);
		}
		
		void ShowError(const std::string& message);
		
		void SetShowHistograms(bool show)
		{
			_histogramMI.set_sensitive(show);
		}
	private:
		void onOpenOptionsSelected(const std::vector<std::string>& files, bool downsampleTime, bool downsampleFreq, size_t timeSize, size_t freqSize, bool correctHistograms);
		void close();
		void readDistributedObservation(const std::string& filename, bool correctHistograms);
		void readMetaInfoFromMS(const std::string& filename);
		void readAndCombine(const std::string& filename);
		
		void onHide()
		{
			//Gtk::Application::quit();
		}
		void onStatusChange(const std::string &newStatus);
		
		void onChangeSheet();
		
		class AOQPlotController* _controller;
		int _activeSheetIndex;
		Gtk::Toolbar _toolbar;
		Gtk::MenuToolButton _pageMenuButton;
		Gtk::Menu _pageMenu;
		Gtk::RadioButtonGroup _pageGroup, _statisticsGroup;
		Gtk::RadioMenuItem _baselineMI, _antennaeMI, _bLengthMI, _timeMI, _frequencyMI, _timeFrequencyMI, _summaryMI, _histogramMI;
		
		Gtk::VBox _vBox;
		Gtk::Statusbar _statusBar;
		
		std::unique_ptr<AOQPageController> _pageController;
		std::unique_ptr<PlotSheet> _activeSheet;
		
		OpenOptionsWindow _openOptionsWindow;
};

#endif
