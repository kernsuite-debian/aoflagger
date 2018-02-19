#ifndef GUI_QUALITY__BASELINEPLOTPAGE_H
#define GUI_QUALITY__BASELINEPLOTPAGE_H

#include <gtkmm/box.h>
#include <gtkmm/window.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>

#include "../rfigui/heatmapwidget.h"

#include "../quality/qualitytablesformatter.h"

#include "grayscaleplotpage.h"

class BaselinePlotPage : public GrayScalePlotPage {
	public:
		BaselinePlotPage(class BaselinePageController* controller);
    virtual ~BaselinePlotPage();
		
	protected:
	private:
		class BaselinePageController* _controller;
		
		void onMouseMoved(size_t x, size_t y);
};

#endif
