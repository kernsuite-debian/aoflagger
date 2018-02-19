#include "histogrampage.h"

#include <boost/bind.hpp>

#include "controllers/histogrampagecontroller.h"

#include "../plot/plotpropertieswindow.h"

#include "datawindow.h"

HistogramPage::HistogramPage(HistogramPageController* controller) :
 	_controller(controller),
	_expander("Side bar"),
	_histogramTypeFrame("Histogram"),
	_totalHistogramButton("Total"),
	_rfiHistogramButton("RFI"),
	_notRFIHistogramButton("Not RFI"),
	_xxPolarizationButton("XX"),
	_xyPolarizationButton("XY"),
	_yxPolarizationButton("YX"),
	_yyPolarizationButton("YY"),
	_sumPolarizationButton("Sum"),
	_fitFrame("Fitting"),
	_fitButton("Fit"),
	_subtractFitButton("Subtract"),
	_fitLogarithmicButton("Log fit"),
	_fitAutoRangeButton("Auto range"),
	_functionFrame("Function"),
	_nsButton("N(S)"),
	_dndsButton("dN(S)/dS"),
	_deltaSEntry(),
	_staircaseFunctionButton("Staircase"),
	_normalizeButton("Normalize"),
	_plotPropertiesButton("Properties"),
	_dataExportButton("Data"),
	_slopeFrame("Slope"),
	_drawSlopeButton("Draw"),
	_drawSlope2Button("Draw2"),
	_slopeAutoRangeButton("Auto range"),
	_plotPropertiesWindow(0)
{
	_histogramTypeBox.pack_start(_totalHistogramButton, Gtk::PACK_SHRINK);
	_totalHistogramButton.set_active(true);
	_totalHistogramButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_histogramTypeBox.pack_start(_rfiHistogramButton, Gtk::PACK_SHRINK);
	_rfiHistogramButton.set_active(false);
	_rfiHistogramButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_histogramTypeBox.pack_start(_notRFIHistogramButton, Gtk::PACK_SHRINK);
	_notRFIHistogramButton.set_active(false);
	_notRFIHistogramButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	
	_histogramTypeFrame.add(_histogramTypeBox);
	
	_sideBox.pack_start(_histogramTypeFrame, Gtk::PACK_SHRINK);
	
	_polarizationBox.pack_start(_xxPolarizationButton, Gtk::PACK_SHRINK);
	_xxPolarizationButton.set_active(false);
	_xxPolarizationButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_polarizationBox.pack_start(_xyPolarizationButton, Gtk::PACK_SHRINK);
	_xyPolarizationButton.set_active(false);
	_xyPolarizationButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_polarizationBox.pack_start(_yxPolarizationButton, Gtk::PACK_SHRINK);
	_yxPolarizationButton.set_active(false);
	_yxPolarizationButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_polarizationBox.pack_start(_yyPolarizationButton, Gtk::PACK_SHRINK);
	_yyPolarizationButton.set_active(false);
	_yyPolarizationButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_polarizationBox.pack_start(_sumPolarizationButton, Gtk::PACK_SHRINK);
	_sumPolarizationButton.set_active(true);
	_sumPolarizationButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));

	_polarizationFrame.add(_polarizationBox);
	
	_sideBox.pack_start(_polarizationFrame, Gtk::PACK_SHRINK);
	
	_fitBox.pack_start(_fitButton, Gtk::PACK_SHRINK);
	_fitButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_fitBox.pack_start(_subtractFitButton, Gtk::PACK_SHRINK);
	_subtractFitButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_fitBox.pack_start(_fitLogarithmicButton, Gtk::PACK_SHRINK);
	_fitLogarithmicButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_fitBox.pack_start(_fitAutoRangeButton, Gtk::PACK_SHRINK);
	_fitAutoRangeButton.set_active(true);
	_fitAutoRangeButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::onAutoRangeClicked));
	
	_fitBox.pack_start(_fitStartEntry, Gtk::PACK_SHRINK);
	_fitStartEntry.set_sensitive(false);
	_fitStartEntry.signal_activate().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_fitBox.pack_start(_fitEndEntry, Gtk::PACK_SHRINK);
	_fitEndEntry.set_sensitive(false);
	_fitEndEntry.signal_activate().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_fitBox.pack_start(_fitTextView, Gtk::PACK_SHRINK);
	
	_fitFrame.add(_fitBox);
	
	_sideBox.pack_start(_fitFrame, Gtk::PACK_SHRINK);
	
	Gtk::RadioButtonGroup group;
	_functionBox.pack_start(_nsButton, Gtk::PACK_SHRINK);
	_nsButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_nsButton.set_group(group);
	_functionBox.pack_start(_dndsButton, Gtk::PACK_SHRINK);
	_dndsButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_dndsButton.set_group(group);
	_nsButton.set_active(true);
	_functionBox.pack_start(_deltaSEntry, Gtk::PACK_SHRINK);
	_deltaSEntry.set_text("2");
	_deltaSEntry.signal_activate().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_functionBox.pack_start(_staircaseFunctionButton, Gtk::PACK_SHRINK);
	_staircaseFunctionButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_functionBox.pack_start(_normalizeButton, Gtk::PACK_SHRINK);
	_normalizeButton.set_active(true);
	_normalizeButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	
	_functionFrame.add(_functionBox);
	_sideBox.pack_start(_functionFrame, Gtk::PACK_SHRINK);
	
	_plotPropertiesButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::onPlotPropertiesClicked));
	_sideBox.pack_start(_plotPropertiesButton, Gtk::PACK_SHRINK);
	
	_dataExportButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::onDataExportClicked));
	_sideBox.pack_start(_dataExportButton, Gtk::PACK_SHRINK);
	
	_slopeBox.pack_start(_slopeTextView, Gtk::PACK_SHRINK);
	_drawSlopeButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_slopeBox.pack_start(_drawSlopeButton, Gtk::PACK_SHRINK);
	_drawSlope2Button.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_slopeBox.pack_start(_drawSlope2Button, Gtk::PACK_SHRINK);

	_slopeBox.pack_start(_slopeAutoRangeButton, Gtk::PACK_SHRINK);
	_slopeAutoRangeButton.set_active(true);
	_slopeAutoRangeButton.signal_clicked().connect(sigc::mem_fun(*this, &HistogramPage::onSlopeAutoRangeClicked));
	
	_slopeBox.pack_start(_slopeStartEntry, Gtk::PACK_SHRINK);
	_slopeStartEntry.set_sensitive(false);
	_slopeStartEntry.signal_activate().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_slopeBox.pack_start(_slopeEndEntry, Gtk::PACK_SHRINK);
	_slopeEndEntry.set_sensitive(false);
	_slopeEndEntry.signal_activate().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	_slopeBox.pack_start(_slopeRFIRatio, Gtk::PACK_SHRINK);
	_slopeRFIRatio.set_text("1.0");
	_slopeRFIRatio.signal_activate().connect(sigc::mem_fun(*this, &HistogramPage::updatePlot));
	
	_slopeFrame.add(_slopeBox);
	_sideBox.pack_start(_slopeFrame, Gtk::PACK_SHRINK);
	
	_expander.add(_sideBox);
	
	pack_start(_expander, Gtk::PACK_SHRINK);
	
	_plotWidget.SetPlot(_controller->Plot());
	pack_start(_plotWidget, Gtk::PACK_EXPAND_WIDGET);
	
	show_all_children();
	
	_dataWindow = new DataWindow();
	
	_controller->Attach(this);
}

HistogramPage::~HistogramPage()
{
	if(_plotPropertiesWindow != 0)
		delete _plotPropertiesWindow;
	delete _dataWindow;
}

void HistogramPage::updatePlot()
{
	_controller->SetDrawXX(_xxPolarizationButton.get_active());
	_controller->SetDrawXY(_xyPolarizationButton.get_active());
	_controller->SetDrawYX(_yxPolarizationButton.get_active());
	_controller->SetDrawYY(_yyPolarizationButton.get_active());
	_controller->SetDrawSum(_sumPolarizationButton.get_active());

	_controller->SetAutomaticFitRange(_fitAutoRangeButton.get_active());
	_controller->SetFitStart(atof(_fitStartEntry.get_text().c_str()));
	_controller->SetFitEnd(atof(_fitEndEntry.get_text().c_str()));
	_controller->SetFitLogarithmic(_fitLogarithmicButton.get_active());
	
	_controller->SetAutomaticSlopeRange( _slopeAutoRangeButton.get_active() );
	_controller->SetSlopeStart( atof(_slopeStartEntry.get_text().c_str()));
	_controller->SetSlopeEnd( atof(_slopeEndEntry.get_text().c_str()));
	_controller->SetSlopeRFIRatio( atof(_slopeRFIRatio.get_text().c_str()) );
	double deltaS = atof(_deltaSEntry.get_text().c_str());
	if(deltaS <= 1.0001) deltaS = 1.0001;
	_controller->SetDeltaS( deltaS );
	
	_controller->SetDrawTotal(_totalHistogramButton.get_active());
	_controller->SetDrawFit(_fitButton.get_active());
	_controller->SetDrawSubtractedFit(_subtractFitButton.get_active());
	_controller->SetDrawSlope(_drawSlopeButton.get_active());
	_controller->SetDrawSlope2(_drawSlope2Button.get_active());
	_controller->SetDrawRFI(_rfiHistogramButton.get_active());
	_controller->SetDrawNonRFI(_notRFIHistogramButton.get_active());
	
	_controller->SetDerivative(_dndsButton.get_active());
	_controller->SetStaircase(_staircaseFunctionButton.get_active());
	_controller->SetNormalize(_normalizeButton.get_active());
	_controller->SetDeltaS(atof(_deltaSEntry.get_text().c_str()));
}

void HistogramPage::onPlotPropertiesClicked()
{
	if(_plotPropertiesWindow == 0)
	{
		_plotPropertiesWindow = new PlotPropertiesWindow(_controller->Plot(), "Plot properties");
		_plotPropertiesWindow->OnChangesApplied = boost::bind(&HistogramPage::Redraw, this);
	}
	
	_plotPropertiesWindow->show();
	_plotPropertiesWindow->raise();
}

void HistogramPage::onDataExportClicked()
{
	_dataWindow->show();
	_dataWindow->raise();
	updateDataWindow();
}

void HistogramPage::SetSlopeFrame(const std::string& str)
{
	_slopeTextView.get_buffer()->set_text(str);
	
	if(_slopeAutoRangeButton.get_active())
	{
		double
			minRange = _controller->SlopeStart(),
			maxRange = _controller->SlopeEnd();
		std::stringstream minRangeStr, maxRangeStr;
		minRangeStr << minRange;
		maxRangeStr << maxRange;
		_slopeStartEntry.set_text(minRangeStr.str());
		_slopeEndEntry.set_text(maxRangeStr.str());
	}
	
	if(_fitAutoRangeButton.get_active())
	{
		std::stringstream minRangeStr, maxRangeStr;
		minRangeStr << _controller->FitStart();
		maxRangeStr << _controller->FitEnd();
		_fitStartEntry.set_text(minRangeStr.str());
		_fitEndEntry.set_text(maxRangeStr.str());
	}
}

void HistogramPage::updateDataWindow()
{
	if(_dataWindow->get_visible())
		_dataWindow->SetData(_controller->Plot());
}
