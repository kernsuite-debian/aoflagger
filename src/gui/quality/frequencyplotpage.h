#ifndef GUI_QUALITY__FREQUENCYPLOTPAGE_H
#define GUI_QUALITY__FREQUENCYPLOTPAGE_H

#include "twodimensionalplotpage.h"

#include "../../quality/statisticscollection.h"
#include "../../quality/statisticsderivator.h"

class FrequencyPlotPage : public TwoDimensionalPlotPage {
	public:
    FrequencyPlotPage() : _ftButton("FT")
		{
		}
		
		virtual void processStatistics(const StatisticsCollection *statCollection, const std::vector<AntennaInfo> &antennas) override final
		{
			_statistics.clear();
			
			const std::map<double, class DefaultStatistics> &map = statCollection->FrequencyStatistics();
			
			for(std::map<double, class DefaultStatistics>::const_iterator i=map.begin();i!=map.end();++i)
			{
				_statistics.insert(std::pair<double, DefaultStatistics>(i->first/1000000.0, i->second));
			}
		}
		
		virtual const std::map<double, class DefaultStatistics> &getStatistics() const override final
		{
			return _statistics;
		}
		
		virtual void startLine(Plot2D &plot, const std::string &name, const std::string &yAxisDesc) override final
		{
			if(_ftButton.get_active())
				plot.StartLine(name, "Time (μs)", yAxisDesc, false);
			else
				plot.StartLine(name, "Frequency (MHz)", yAxisDesc, false);
		}
		
		virtual void processPlot(Plot2D &plot) override final
		{
			if(_ftButton.get_active())
			{
				performFt(plot);
			}
		}
		
		virtual void addCustomPlotButtons(Gtk::Toolbar &container) override final
		{
			_ftButton.signal_clicked().connect(sigc::mem_fun(*this, &FrequencyPlotPage::onFTButtonClicked));
			container.append(_ftButton);
			_ftButton.show();
		}
	private:
		void onFTButtonClicked()
		{
			updatePlot();
		}
		
		void performFt(Plot2D &plot)
		{
			size_t count = plot.PointSetCount();
			for(size_t line=0;line<count;++line)
			{
				Plot2DPointSet &pointSet = plot.GetPointSet(line);
				std::vector<std::pair<double, std::complex<double> > > output;
				const double min = pointSet.MinX();
				const double width = pointSet.MaxX() - min;
				const double fStart = -2.0 * M_PI * (double) pointSet.Size() / width;
				const double fEnd = 2.0 * M_PI * (double) pointSet.Size() / width;
				const double fStep = (fEnd - fStart) / (double) pointSet.Size();
				for(double f = fStart; f < fEnd ; f += fStep)
				{
					std::pair<double, std::complex<double> > newElement(f/(2.0*M_PI), std::complex<double>(0.0, 0.0));
					std::complex<double> &nextStat = newElement.second;
					for(size_t i=0; i != pointSet.Size(); ++i)
					{
						const double t_f = pointSet.GetX(i) * f;
						const double val = pointSet.GetY(i);
						if(std::isfinite(val))
							nextStat += std::complex<double>(val * cos(t_f), val * sin(t_f));
					}
					output.push_back(newElement);
				}
				
				pointSet.Clear();
				for(std::vector<std::pair<double, std::complex<double> > >::const_iterator i=output.begin();i!=output.end();++i)
				{
					double real = i->second.real(), imag=i->second.imag();
					pointSet.PushDataPoint(i->first, sqrt(real*real + imag*imag));
				}
			}
		}
		
		std::map<double, class DefaultStatistics> _statistics;
		Gtk::ToggleToolButton _ftButton;
};

#endif
