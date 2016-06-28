#ifndef GUI_QUALITY__TIMEFREQUENCYPLOTPAGE_H
#define GUI_QUALITY__TIMEFREQUENCYPLOTPAGE_H

#include <gtkmm/box.h>
#include <gtkmm/window.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>

#include "../imagewidget.h"

#include "../../quality/qualitytablesformatter.h"

#include "grayscaleplotpage.h"

class TimeFrequencyPlotPage : public GrayScalePlotPage {
	public:
		TimeFrequencyPlotPage();
    virtual ~TimeFrequencyPlotPage();
		
		virtual void SetStatistics(const StatisticsCollection *statCollection, const std::vector<class AntennaInfo>&) override final
		{
			_statCollection = statCollection;
			updateImage();
		}
		virtual void CloseStatistics() override final
		{
			_statCollection = 0;
		}
		bool HasStatistics() const
		{
			return _statCollection != 0;
		}
		
	protected:
		virtual std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> constructImage(QualityTablesFormatter::StatisticKind kind);
	private:
		void onMouseMoved(size_t x, size_t y);
		
		const StatisticsCollection *_statCollection;
};

#endif
