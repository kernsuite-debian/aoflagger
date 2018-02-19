#include <limits>
#include <functional>

#include "datawindow.h"
#include "twodimensionalplotpage.h"

#include "../quality/statisticscollection.h"
#include "../quality/statisticsderivator.h"

#include "../plot/plotpropertieswindow.h"

#include <gtkmm/icontheme.h>

TwoDimensionalPlotPage::TwoDimensionalPlotPage(AOQPlotPageController* controller) :
	_controller(controller),
	_countButton("#"),
	_meanButton("μ"),
	_stdDevButton("σ"),
	_varianceButton("σ²"),
	_dCountButton("Δ#"),
	_dMeanButton("Δμ"),
	_dStdDevButton("Δσ"),
	_rfiPercentageButton("%"),
	_polPPButton("pp"),
	_polPQButton("pq"),
	_polQPButton("qp"),
	_polQQButton("qq"),
	_polIButton("I"),
	_amplitudeButton("A"),
	_phaseButton("ϕ"),
	_realButton("r"),
	_imaginaryButton("i"),
	_logarithmicButton("Log"),
	_zeroAxisButton("0"),
	_plotPropertiesButton("P"),
	_dataExportButton("D"),
	_plotPropertiesWindow(nullptr),
	_dataWindow(new DataWindow()),
	_customButtonsCreated(false)
{
	_plotWidget.SetPlot(_controller->Plot());
	pack_start(_plotWidget, Gtk::PACK_EXPAND_WIDGET);
	
	show_all_children();
	
	_controller->Attach(this);
}

TwoDimensionalPlotPage::~TwoDimensionalPlotPage()
{ }

void TwoDimensionalPlotPage::updatePlotConfig()
{
	_controller->Plot().SetIncludeZeroYAxis(_zeroAxisButton.get_active());
	_controller->Plot().SetLogarithmicYAxis(_logarithmicButton.get_active());
	_plotWidget.Update();
}

std::set<QualityTablesFormatter::StatisticKind> TwoDimensionalPlotPage::GetSelectedKinds() const
{
	std::set<QualityTablesFormatter::StatisticKind> kinds;
	if(_countButton.get_active())
		kinds.insert(QualityTablesFormatter::CountStatistic);
	if(_meanButton.get_active())
		kinds.insert(QualityTablesFormatter::MeanStatistic);
	if(_stdDevButton.get_active())
		kinds.insert(QualityTablesFormatter::StandardDeviationStatistic);
	if(_varianceButton.get_active())
		kinds.insert(QualityTablesFormatter::VarianceStatistic);
	if(_dCountButton.get_active())
		kinds.insert(QualityTablesFormatter::DCountStatistic);
	if(_dMeanButton.get_active())
		kinds.insert(QualityTablesFormatter::DMeanStatistic);
	if(_dStdDevButton.get_active())
		kinds.insert(QualityTablesFormatter::DStandardDeviationStatistic);
	if(_rfiPercentageButton.get_active())
		kinds.insert(QualityTablesFormatter::RFIPercentageStatistic);
	return kinds;
}

std::set<AOQPlotPageController::SelectedPol> TwoDimensionalPlotPage::GetSelectedPolarizations() const
{
	std::set<AOQPlotPageController::SelectedPol> pols;
	if(_polPPButton.get_active())
		pols.insert(AOQPlotPageController::PolPP);
	if(_polPQButton.get_active())
		pols.insert(AOQPlotPageController::PolPQ);
	if(_polQPButton.get_active())
		pols.insert(AOQPlotPageController::PolQP);
	if(_polQQButton.get_active())
		pols.insert(AOQPlotPageController::PolQQ);
	if(_polIButton.get_active())
		pols.insert(AOQPlotPageController::PolI);
	return pols;
}

std::set<AOQPlotPageController::PhaseType> TwoDimensionalPlotPage::GetSelectedPhases() const
{
	std::set<AOQPlotPageController::PhaseType> phases;
	if(_amplitudeButton.get_active())
		phases.insert(AOQPlotPageController::AmplitudePhaseType);
	if(_phaseButton.get_active())
		phases.insert(AOQPlotPageController::PhasePhaseType);
	if(_realButton.get_active())
		phases.insert(AOQPlotPageController::RealPhaseType);
	if(_imaginaryButton.get_active())
		phases.insert(AOQPlotPageController::ImaginaryPhaseType);
	return phases;
}

void TwoDimensionalPlotPage::InitializeToolbar(Gtk::Toolbar& toolbar)
{
	if(Gtk::IconTheme::get_default()->has_icon("aoflagger"))
	{
		toolbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);
		toolbar.set_icon_size(Gtk::ICON_SIZE_LARGE_TOOLBAR);
	}
	else {
		toolbar.set_toolbar_style(Gtk::TOOLBAR_TEXT);
		toolbar.set_icon_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
	}
	initStatisticKindButtons(toolbar);
	initPolarizationButtons(toolbar);
	initPhaseButtons(toolbar);
	initPlotButtons(toolbar);
	
	if(!_customButtonsCreated)
	{
		addCustomPlotButtons(toolbar);
		_customButtonsCreated = true;
	}
}

void TwoDimensionalPlotPage::initStatisticKindButtons(Gtk::Toolbar& toolbar)
{
	toolbar.append(_separator1);
	
	_countButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_countButton.set_tooltip_text("Visibility count");
	toolbar.append(_countButton);
	
	_meanButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_meanButton.set_tooltip_text("Mean value");
	toolbar.append(_meanButton);
	
	_stdDevButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_stdDevButton.set_active(true);
	_stdDevButton.set_tooltip_text("Standard deviation");
	toolbar.append(_stdDevButton);
	
	_varianceButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	toolbar.append(_varianceButton);
	
	//_dCountButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	//toolbar.append(_dCountButton);
	
	_dMeanButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_dMeanButton.set_tooltip_text("Frequency-differential (difference between channels) mean value");
	toolbar.append(_dMeanButton);
	
	_dStdDevButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_dStdDevButton.set_tooltip_text("Frequency-differential (difference between channels) standard deviation");
	toolbar.append(_dStdDevButton);
	
	_rfiPercentageButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_rfiPercentageButton.set_tooltip_text("Flagged percentage");
	toolbar.append(_rfiPercentageButton);
}

void TwoDimensionalPlotPage::initPolarizationButtons(Gtk::Toolbar& toolbar)
{
	toolbar.append(_separator2);
	
	_polPPButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_polPPButton.set_active(false);
	_polPPButton.set_icon_name("showpp");
	_polPPButton.set_tooltip_text("Display statistics for the PP polarization. Depending on the polarization configuration of the measurement set, this will show XX or RR.");
	toolbar.append(_polPPButton);
	
	_polPQButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_polPQButton.set_active(false);
	_polPQButton.set_icon_name("showpq");
	_polPQButton.set_tooltip_text("Display statistics for the PQ polarization. Depending on the polarization configuration of the measurement set, this will show XY or RL.");
	toolbar.append(_polPQButton);
	
	_polQPButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_polQPButton.set_active(false);
	_polQPButton.set_icon_name("showqp");
	_polPQButton.set_tooltip_text("Display statistics for the QP polarization. Depending on the polarization configuration of the measurement set, this will show YX or LR.");
	toolbar.append(_polQPButton);
	
	_polQQButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_polQQButton.set_active(false);
	_polQQButton.set_icon_name("showqq");
	_polQQButton.set_tooltip_text("Display statistics for the QQ polarization. Depending on the polarization configuration of the measurement set, this will show YY or LL.");
	toolbar.append(_polQQButton);
	
	_polIButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_polIButton.set_active(true);
	_polIButton.set_tooltip_text("Display statistics for QQ + PP.");
	toolbar.append(_polIButton);
}

void TwoDimensionalPlotPage::initPhaseButtons(Gtk::Toolbar& toolbar)
{
	toolbar.append(_separator3);
	
	_amplitudeButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_amplitudeButton.set_active(true);
	_amplitudeButton.set_tooltip_text("Amplitude");
	toolbar.append(_amplitudeButton);
	
	_phaseButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_phaseButton.set_tooltip_text("Phase");
	toolbar.append(_phaseButton);
	
	_realButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_realButton.set_tooltip_text("Real value");
	toolbar.append(_realButton);
	
	_imaginaryButton.signal_clicked().connect(sigc::mem_fun(*_controller, &AOQPlotPageController::UpdatePlot));
	_imaginaryButton.set_tooltip_text("Imaginary value");
	toolbar.append(_imaginaryButton);
}

void TwoDimensionalPlotPage::initPlotButtons(Gtk::Toolbar& toolbar)
{
	toolbar.append(_separator4);
	
	_logarithmicButton.signal_clicked().connect(sigc::mem_fun(*this, &TwoDimensionalPlotPage::onLogarithmicClicked));
	toolbar.append(_logarithmicButton);
	
	_zeroAxisButton.signal_clicked().connect(sigc::mem_fun(*this, &TwoDimensionalPlotPage::updatePlotConfig));
	_zeroAxisButton.set_active(true);
	toolbar.append(_zeroAxisButton);
	_controller->Plot().SetIncludeZeroYAxis(true);
	
	_plotPropertiesButton.signal_clicked().connect(sigc::mem_fun(*this, &TwoDimensionalPlotPage::onPlotPropertiesClicked));
	toolbar.append(_plotPropertiesButton);

	_dataExportButton.signal_clicked().connect(sigc::mem_fun(*this, &TwoDimensionalPlotPage::onDataExportClicked));
	toolbar.append(_dataExportButton);
}

void TwoDimensionalPlotPage::onPlotPropertiesClicked()
{
	if(_plotPropertiesWindow == nullptr)
	{
		_plotPropertiesWindow.reset(new PlotPropertiesWindow(_controller->Plot(), "Plot properties"));
		_plotPropertiesWindow->OnChangesApplied = std::bind(&AOQPlotPageController::UpdatePlot, _controller);
	}
	
	_plotPropertiesWindow->show();
	_plotPropertiesWindow->raise();
}

void TwoDimensionalPlotPage::updateDataWindow()
{
	if(_dataWindow->get_visible())
		_dataWindow->SetData(_controller->Plot());
}

void TwoDimensionalPlotPage::onDataExportClicked()
{
	_dataWindow->show();
	_dataWindow->raise();
	updateDataWindow();
}
void TwoDimensionalPlotPage::Redraw()
{
	_plotWidget.Update();
	
	if(_dataWindow->get_visible())
	{
		updateDataWindow();
	}
}
