#ifndef GUI_QUALITY__2DPLOTPAGE_H
#define GUI_QUALITY__2DPLOTPAGE_H

#include "controllers/aoqplotpagecontroller.h"

#include "../quality/qualitytablesformatter.h"

#include "../plot/plot2d.h"
#include "../plot/plotwidget.h"

#include "plotsheet.h"

#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/separatortoolitem.h>

#include <set>

class TwoDimensionalPlotPage : public PlotSheet {
	public:
		TwoDimensionalPlotPage(AOQPlotPageController* _controller);
    virtual ~TwoDimensionalPlotPage();

		virtual void InitializeToolbar(Gtk::Toolbar& toolbar) override final;
		
		std::set<QualityTablesFormatter::StatisticKind> GetSelectedKinds() const;
		std::set<enum AOQPlotPageController::SelectedPol> GetSelectedPolarizations() const;
		std::set<enum AOQPlotPageController::PhaseType> GetSelectedPhases() const;
		
		void Redraw();
		
	protected:
		virtual void addCustomPlotButtons(Gtk::Toolbar& container)
		{ }
		
	private:		
		void updatePlotConfig();
		void updateDataWindow();
		
		void initStatisticKindButtons(Gtk::Toolbar& toolbar);
		void initPolarizationButtons(Gtk::Toolbar& toolbar);
		void initPhaseButtons(Gtk::Toolbar& toolbar);
		void initPlotButtons(Gtk::Toolbar& toolbar);
		
		void onLogarithmicClicked()
		{
			_zeroAxisButton.set_sensitive(!_logarithmicButton.get_active());
			updatePlotConfig();
		}
		void onPlotPropertiesClicked();
		void onDataExportClicked();
		
		AOQPlotPageController* _controller;
		
		Gtk::SeparatorToolItem _separator1, _separator2, _separator3, _separator4;
		
		Gtk::ToggleToolButton _countButton, _meanButton, _stdDevButton, _varianceButton, _dCountButton, _dMeanButton, _dStdDevButton,  _rfiPercentageButton;
		
		Gtk::ToggleToolButton _polPPButton, _polPQButton, _polQPButton, _polQQButton, _polIButton;
		
		Gtk::ToggleToolButton _amplitudeButton, _phaseButton, _realButton, _imaginaryButton;
		
		Gtk::ToggleToolButton _logarithmicButton, _zeroAxisButton;
		Gtk::ToolButton _plotPropertiesButton, _dataExportButton;
		
		PlotWidget _plotWidget;
		
		std::unique_ptr<class PlotPropertiesWindow> _plotPropertiesWindow;
		std::unique_ptr<class DataWindow> _dataWindow;
		
		bool _customButtonsCreated;
};

#endif
