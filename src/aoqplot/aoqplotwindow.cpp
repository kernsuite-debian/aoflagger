#include "aoqplotwindow.h"

#include "controllers/antennapagecontroller.h"
#include "controllers/aoqplotcontroller.h"
#include "controllers/baselinepagecontroller.h"
#include "controllers/blengthpagecontroller.h"
#include "controllers/frequencypagecontroller.h"
#include "controllers/histogrampagecontroller.h"
#include "controllers/tfpagecontroller.h"
#include "controllers/timepagecontroller.h"

#include <limits>

#include <gtkmm/main.h>
#include <gtkmm/messagedialog.h>

#include "../structures/msmetadata.h"

#include "../quality/histogramtablesformatter.h"
#include "../quality/histogramcollection.h"
#include "../quality/statisticscollection.h"

#include "../remote/clusteredobservation.h"
#include "../remote/processcommander.h"

#include "baselineplotpage.h"
#include "blengthplotpage.h"
#include "frequencyplotpage.h"
#include "histogrampage.h"
#include "summarypage.h"
#include "timefrequencyplotpage.h"

AOQPlotWindow::AOQPlotWindow(AOQPlotController* controller) :
	_controller(controller),
	_activeSheetIndex(-1),
	_pageMenuButton("Sheet"),
	_baselineMI(_pageGroup, "Baselines"),
	_antennaeMI(_pageGroup, "Antennae"),
	_bLengthMI(_pageGroup, "Baseline lengths"),
	_timeMI(_pageGroup, "Time"),
	_frequencyMI(_pageGroup, "Frequency"),
	_timeFrequencyMI(_pageGroup, "Time-frequency"),
	_summaryMI(_pageGroup, "Summary"),
	_histogramMI(_pageGroup, "Histograms")
{
	set_default_icon_name("aoqplot");
	
	_toolbar.append(_pageMenuButton);
	_pageMenuButton.set_menu(_pageMenu);
	_pageMenu.append(_baselineMI);
	_baselineMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_antennaeMI);
	_antennaeMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_bLengthMI);
	_bLengthMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_timeMI);
	_timeMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_frequencyMI);
	_frequencyMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_timeFrequencyMI);
	_timeFrequencyMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_summaryMI);
	_summaryMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.append(_histogramMI);
	_histogramMI.signal_toggled().connect(sigc::mem_fun(*this, &AOQPlotWindow::onChangeSheet));
	_pageMenu.show_all_children();
	
	_vBox.pack_start(_toolbar, Gtk::PACK_SHRINK);
	
	_vBox.pack_end(_statusBar, Gtk::PACK_SHRINK);
	_statusBar.push("Quality plot util is ready. Author: André Offringa (offringa@gmail.com)");
	
	add(_vBox);
	
	_openOptionsWindow.SignalOpen().connect(sigc::mem_fun(*this, &AOQPlotWindow::onOpenOptionsSelected));
	signal_hide().connect(sigc::mem_fun(*this, &AOQPlotWindow::onHide));
	
	_controller->Attach(this);
}

void AOQPlotWindow::Open(const std::vector<std::string> &files)
{
	show_all();
	_openOptionsWindow.ShowForFile(files);
}

void AOQPlotWindow::onOpenOptionsSelected(const std::vector<std::string>& files, bool downsampleTime, bool downsampleFreq, size_t timeCount, size_t freqCount, bool correctHistograms)
{
	_controller->ReadStatistics(files, downsampleTime, downsampleFreq, timeCount, freqCount, correctHistograms);
	_activeSheetIndex = -1;
	onChangeSheet();
	show();
}

void AOQPlotWindow::onStatusChange(const std::string &newStatus)
{
	_statusBar.pop();
	_statusBar.push(newStatus);
}

void AOQPlotWindow::onChangeSheet()
{
	int selectedSheet = -1;
	if(_baselineMI.get_active())
		selectedSheet = 0;
	else if(_antennaeMI.get_active())
		selectedSheet = 1;
	else if(_bLengthMI.get_active())
		selectedSheet = 2;
	else if(_timeMI.get_active())
		selectedSheet = 3;
	else if(_frequencyMI.get_active())
		selectedSheet = 4;
	else if(_timeFrequencyMI.get_active())
		selectedSheet = 5;
	else if(_summaryMI.get_active())
		selectedSheet = 6;
	else if(_histogramMI.get_active())
		selectedSheet = 7;
	
	if(selectedSheet != _activeSheetIndex)
	{
		switch(selectedSheet)
		{
		case 0:
			_pageController.reset(new BaselinePageController());
			_activeSheet.reset(new BaselinePlotPage( static_cast<BaselinePageController*>(_pageController.get())));
			SetStatus("Baseline statistics");
			break;
		case 1:
			_pageController.reset(new AntennaePageController());
			_activeSheet.reset(new TwoDimensionalPlotPage( static_cast<AntennaePageController*>(_pageController.get())));
			SetStatus("Antennae statistics");
			break;
		case 2:
			_pageController.reset(new BLengthPageController());
			_activeSheet.reset(new BLengthPlotPage( static_cast<BLengthPageController*>(_pageController.get())));
			SetStatus("Baseline length statistics");
			break;
		case 3:
			_pageController.reset(new TimePageController());
			_activeSheet.reset(new TwoDimensionalPlotPage( static_cast<TimePageController*>(_pageController.get())));
			SetStatus("Time statistics");
			break;
		case 4:
			_pageController.reset(new FrequencyPageController());
			_activeSheet.reset(new FrequencyPlotPage(static_cast<FrequencyPageController*>(_pageController.get())));
			SetStatus("Frequency statistics");
			break;
		case 5:
			_pageController.reset(new TFPageController());
			_activeSheet.reset(new TimeFrequencyPlotPage( static_cast<TFPageController*>(_pageController.get())));
			SetStatus("Time-frequency statistics");
			break;
		case 6:
			_pageController.reset(new SummaryPageController());
			_activeSheet.reset(new SummaryPage( static_cast<SummaryPageController*>(_pageController.get())));
			SetStatus("Summary");
			break;
		case 7:
			_pageController.reset(new HistogramPageController());
			_activeSheet.reset(new HistogramPage( static_cast<HistogramPageController*>(_pageController.get())));
			SetStatus("Histograms");
			break;
		}
		
		_activeSheetIndex = selectedSheet;
		_controller->Initialize(_pageController.get(), selectedSheet != 5);
		_activeSheet->SignalStatusChange().connect(sigc::mem_fun(*this, &AOQPlotWindow::onStatusChange));
		_activeSheet->InitializeToolbar(_toolbar);
		_toolbar.show_all();
		_vBox.pack_start(*_activeSheet);
		_activeSheet->show_all();
	}
}

void AOQPlotWindow::ShowError(const std::string& message)
{
	Gtk::MessageDialog dialog(*this, message, false, Gtk::MESSAGE_ERROR);
	dialog.run();
}
