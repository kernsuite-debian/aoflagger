#ifndef GUI_QUALITY__BLENGTHPLOTPAGE_H
#define GUI_QUALITY__BLENGTHPLOTPAGE_H

#include "twodimensionalplotpage.h"

#include "../structures/measurementset.h"

#include "../remote/clusteredobservation.h"

#include "controllers/blengthpagecontroller.h"

class BLengthPlotPage : public TwoDimensionalPlotPage {
	public:
    BLengthPlotPage(BLengthPageController* controller) :
			TwoDimensionalPlotPage(controller),
			_controller(controller),
			_includeAutoCorrelationsButton("Auto-correlations")
		{ }
	protected:		
		virtual void addCustomPlotButtons(Gtk::Toolbar &container) override final
		{
			_includeAutoCorrelationsButton.signal_clicked().connect(sigc::mem_fun(*this, &BLengthPlotPage::onAutoCorrelationsClicked));
			container.append(_includeAutoCorrelationsButton);
			_includeAutoCorrelationsButton.show();
		}
	private:
		void onAutoCorrelationsClicked()
		{
			_controller->SetIncludeAutoCorrelations(_includeAutoCorrelationsButton.get_active());
			_controller->UpdatePlot();
		}
		
		BLengthPageController* _controller;
		Gtk::ToggleToolButton _includeAutoCorrelationsButton;
};

#endif
