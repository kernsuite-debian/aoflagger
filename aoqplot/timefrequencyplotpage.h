#ifndef GUI_QUALITY__TIMEFREQUENCYPLOTPAGE_H
#define GUI_QUALITY__TIMEFREQUENCYPLOTPAGE_H

#include "grayscaleplotpage.h"

class TimeFrequencyPlotPage : public GrayScalePlotPage {
 public:
  explicit TimeFrequencyPlotPage(class TFPageController* controller);

 private:
  void onMouseMoved(double x, double y);
};

#endif
