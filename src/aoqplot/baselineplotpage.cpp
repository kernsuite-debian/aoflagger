#include <limits>

#include "baselineplotpage.h"

#include "controllers/baselinepagecontroller.h"

BaselinePlotPage::BaselinePlotPage(BaselinePageController* controller) :
	GrayScalePlotPage(controller),
	_controller(controller)
{
	grayScaleWidget().OnMouseMovedEvent().connect(sigc::mem_fun(*this, &BaselinePlotPage::onMouseMoved));
	grayScaleWidget().Plot().SetXAxisDescription("Antenna 1 index");
	grayScaleWidget().Plot().SetManualXAxisDescription(true);
	grayScaleWidget().Plot().SetYAxisDescription("Antenna 2 index");
	grayScaleWidget().Plot().SetManualYAxisDescription(true);
}

BaselinePlotPage::~BaselinePlotPage()
{ }

void BaselinePlotPage::onMouseMoved(size_t x, size_t y)
{
	std::string
		antenna1Name = 	_controller->AntennaName(x),
		antenna2Name = _controller->AntennaName(y);
	const QualityTablesFormatter::StatisticKind kind = getSelectedStatisticKind();
	const std::string &kindName = QualityTablesFormatter::KindToName(kind);
	
	std::stringstream text;
	text << "Correlation " << antenna1Name << " (" << x << ") x " << antenna2Name << " (" << y << "), " << kindName << " = " << grayScaleWidget().Plot().Image()->Value(x, y);
	_signalStatusChange(text.str());
}
