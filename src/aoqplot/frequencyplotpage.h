#ifndef GUI_QUALITY__FREQUENCYPLOTPAGE_H
#define GUI_QUALITY__FREQUENCYPLOTPAGE_H

#include "twodimensionalplotpage.h"

#include "controllers/frequencypagecontroller.h"

#include "../quality/statisticscollection.h"
#include "../quality/statisticsderivator.h"

class FrequencyPlotPage : public TwoDimensionalPlotPage {
	public:
    FrequencyPlotPage(FrequencyPageController* controller) :
			TwoDimensionalPlotPage(controller),
			_controller(controller),
			_ftButton("FT")
		{ }
		
		virtual void addCustomPlotButtons(Gtk::Toolbar &container) override final
		{
			_ftButton.signal_clicked().connect(sigc::mem_fun(*this, &FrequencyPlotPage::onFTButtonClicked));
			container.append(_ftButton);
			_ftButton.show();
		}
	private:
		void onFTButtonClicked()
		{
			_controller->SetPerformFT(_ftButton.get_active());
			_controller->UpdatePlot();
		}
		
		FrequencyPageController* _controller;
		Gtk::ToggleToolButton _ftButton;
};

#endif
