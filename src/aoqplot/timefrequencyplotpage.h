#ifndef GUI_QUALITY__TIMEFREQUENCYPLOTPAGE_H
#define GUI_QUALITY__TIMEFREQUENCYPLOTPAGE_H

#include "grayscaleplotpage.h"

class TimeFrequencyPlotPage : public GrayScalePlotPage {
public:
	TimeFrequencyPlotPage(class TFPageController* controller);

private:
	void onMouseMoved(size_t x, size_t y);
};

#endif
