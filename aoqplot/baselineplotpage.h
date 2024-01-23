#ifndef GUI_QUALITY__BASELINEPLOTPAGE_H
#define GUI_QUALITY__BASELINEPLOTPAGE_H

#include <gtkmm/box.h>
#include <gtkmm/window.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>

#include "../quality/qualitytablesformatter.h"

#include "grayscaleplotpage.h"

class BaselinePlotPage : public GrayScalePlotPage {
 public:
  explicit BaselinePlotPage(class BaselinePageController* controller);
  virtual ~BaselinePlotPage();

 protected:
 private:
  class BaselinePageController* _controller;

  void onMouseMoved(double x, double y);
};

#endif
