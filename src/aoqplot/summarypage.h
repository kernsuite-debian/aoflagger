#ifndef GUI_QUALITY__SUMMARYPAGE_H
#define GUI_QUALITY__SUMMARYPAGE_H

#include <gtkmm/box.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/textview.h>

#include "../quality/statisticscollection.h"
#include "../quality/statisticsderivator.h"

#include "plotsheet.h"

#include <cmath>

#ifndef HAVE_EXP10
#define exp10(x) exp( (2.3025850929940456840179914546844) * (x) )
#endif

class SummaryPageController : public AOQPageController
{
public:
	void Attach(class SummaryPage* page) { _page = page; }
	void SetStatistics(const StatisticsCollection* statCollection, const std::vector<class AntennaInfo>&) final override;
	void CloseStatistics() final override;
	bool HasStatistics() const
	{
		return _statCollection != nullptr;
	}
	
private:
	void updateText();
	
	void addText(std::ostringstream& str, DefaultStatistics &statistics)
	{
		unsigned long totalRFICount = 0;
		unsigned long totalCount = 0;
		const unsigned polarizationCount = statistics.PolarizationCount();
		std::vector<double>
			stdDev(polarizationCount), dStdDev(polarizationCount),
			variance(polarizationCount), dVariance(polarizationCount);
		for(unsigned p=0;p<polarizationCount;++p)
		{
			totalRFICount += statistics.rfiCount[p];
			totalCount += statistics.rfiCount[p] + statistics.count[p];
			variance[p] = StatisticsDerivator::VarianceAmplitude(statistics.count[p], statistics.sum[p], statistics.sumP2[p]);
			dVariance[p] = StatisticsDerivator::VarianceAmplitude(statistics.dCount[p], statistics.dSum[p], statistics.dSumP2[p]);
			stdDev[p] = StatisticsDerivator::StandardDeviationAmplitude(statistics.count[p], statistics.sum[p], statistics.sumP2[p]);
			dStdDev[p] = StatisticsDerivator::StandardDeviationAmplitude(statistics.dCount[p], statistics.dSum[p], statistics.dSumP2[p]);
		}
		
		double rfiRatioValue = round(((double) totalRFICount * 10000.0 / (double) totalCount)) * 0.01;
		
		double countExp = floor(log10(totalCount));
		double countMantissa = totalCount / exp10(countExp);
		
		str << "Sample count = " << round(countMantissa*100.0)*0.01 << " x 10^" << countExp << "\n";
		str << "Total RFI ratio = " << rfiRatioValue << "%\n";
		str << "Standard deviation amplitude = ";
		addValues(&stdDev[0], polarizationCount, str);
		str << " Jy\nDifferential stddev amplitude = ";
		addValues(&dStdDev[0], polarizationCount, str);
		str << " Jy\nVariance amplitude = ";
		addValues(&variance[0], polarizationCount, str);
		str << " Jy\nDifferential variance amplitude = ";
		addValues(&dVariance[0], polarizationCount, str);
		str << " Jy\n";
	}
	
	void addBaselineAverages(std::ostringstream &str)
	{
		const BaselineStatisticsMap &map = _statCollection->BaselineStatistics();
		std::vector<std::pair <unsigned, unsigned> > list = map.BaselineList();
		std::vector<double>
			totalStdDev(map.PolarizationCount()),
			totalSNR(map.PolarizationCount());
		std::vector<size_t> count(map.PolarizationCount());
		for(size_t p=0;p<map.PolarizationCount();++p)
		{
			totalStdDev[p] = 0.0;
			totalSNR[p] = 0.0;
			count[p] = 0;
		}
		for(std::vector<std::pair <unsigned, unsigned> >::const_iterator i=list.begin(); i!=list.end(); ++i)
		{
			unsigned a1=i->first, a2=i->second;
			if(a1 != a2)
			{
				const DefaultStatistics &stat = map.GetStatistics(a1, a2);
				for(size_t p=0;p<map.PolarizationCount();++p)
				{
					const double
						thisStdDev = StatisticsDerivator::GetStatisticAmplitude(QualityTablesFormatter::StandardDeviationStatistic, stat, p),
						thisSNR = StatisticsDerivator::GetStatisticAmplitude(QualityTablesFormatter::SignalToNoiseStatistic, stat, p);
					if(std::isfinite(thisStdDev) && std::isfinite(thisSNR))
					{
						totalStdDev[p] += thisStdDev;
						totalSNR[p] += thisSNR;
						++count[p];
					}
				}
			}
		}
		for(size_t p=0;p<map.PolarizationCount();++p)
		{
			totalStdDev[p] /= (double) count[p];
			totalSNR[p] /= (double) count[p];
		}
		str << "Average standard deviation = ";
		addValues(&totalStdDev[0], map.PolarizationCount(), str);
		str << " Jy\nAverage signal to noise ratio = ";
		addValues(&totalSNR[0], map.PolarizationCount(), str);
		str << " Jy\n(calculated with BaselineMean/BaselineDStdDev)\n";
	}

	void addValues(const double *values, unsigned polarizationCount, std::ostringstream &s)
	{
		s << '[' << values[0];
		for(unsigned p=1;p<polarizationCount;++p)
		{
			s << ", " << values[p];
		}
		s << ']';
	}
	
	const StatisticsCollection *_statCollection;
	class SummaryPage* _page;
};

class SummaryPage : public PlotSheet {
public:
	SummaryPage(SummaryPageController* controller) :
		_controller(controller)
	{
		add(_textView);
		_textView.show();
		
		_controller->Attach(this);
	}
	
	void SetText(const std::string& str)
	{
		Glib::RefPtr<Gtk::TextBuffer> buffer = _textView.get_buffer();
		buffer->set_text(str);
	}
	
private:
	Gtk::TextView _textView;
	SummaryPageController* _controller;
};

inline void SummaryPageController::SetStatistics(const StatisticsCollection* statCollection, const std::vector<class AntennaInfo>&)
{
	_statCollection = statCollection;
	updateText();
}

inline void SummaryPageController::CloseStatistics()
{
	_statCollection = nullptr;
	_page->SetText("No open measurement set");
}

inline void SummaryPageController::updateText()
{
	DefaultStatistics statistics(_statCollection->PolarizationCount());
	std::ostringstream str;
	
	_statCollection->GetGlobalCrossBaselineStatistics(statistics);
	str << "Statistics of cross-correlated baselines\n";
	addText(str, statistics);
	
	str << "\nAverages over cross-correlated baselines\n";
	addBaselineAverages(str);

	_statCollection->GetGlobalAutoBaselineStatistics(statistics);
	str << "\nStatistics of auto-correlated baselines\n";
	addText(str, statistics);
	_page->SetText(str.str());
}

#endif
