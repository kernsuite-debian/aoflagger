#include "aoqplotpagecontroller.h"

#include "../twodimensionalplotpage.h"

#include "../../quality/statisticsderivator.h"

#include "../../util/logger.h"

AOQPlotPageController::AOQPlotPageController() :
	_page(nullptr),
	_statCollection(nullptr)
{ }

void AOQPlotPageController::UpdatePlot()
{
	if(_page != nullptr)
	{
		updatePlotForSettings(
			_page->GetSelectedKinds(),
			_page->GetSelectedPolarizations(),
			_page->GetSelectedPhases());
	}
}

void AOQPlotPageController::updatePlotForSettings(
	const std::set<QualityTablesFormatter::StatisticKind>& kinds,
	const std::set<SelectedPol>& pols,
	const std::set<PhaseType>& phases)
{
	if(HasStatistics())
	{
		_plot.Clear();
		
		int index = 0;
		for(QualityTablesFormatter::StatisticKind k : kinds)
		{
			for(SelectedPol p : pols)
			{
				for(PhaseType ph : phases)
				{
					plotStatistic(k, p, ph, index, getYDesc(kinds));
					++index;
				}
			}
		}
		
		processPlot(_plot);
		
		if(_page != nullptr)
		{
			_page->Redraw();
		}
	}
}

double AOQPlotPageController::getValue(enum PhaseType phase, const std::complex<long double>& val)
{
	switch(phase)
	{
		default:
		case AmplitudePhaseType: return sqrt(val.real()*val.real() + val.imag()*val.imag());
		case PhasePhaseType: return atan2(val.imag(), val.real());
		case RealPhaseType: return val.real();
		case ImaginaryPhaseType: return val.imag();
	}
}

void AOQPlotPageController::plotStatistic(QualityTablesFormatter::StatisticKind kind, SelectedPol pol, PhaseType phase, int lineIndex, const std::string& yDesc)
{
	StatisticsDerivator derivator(*_statCollection);
	size_t polCount = _statCollection->PolarizationCount();
	const std::map<double, DefaultStatistics> &statistics = getStatistics();
	std::ostringstream s;
	int polIndex = -1;
	if(pol == PolI) {
		s << "Polarization I";
	}
	else {
		s << "Polarization " << pol;
	}
	switch(pol)
	{
	case PolI:
	case PolPP:
		polIndex = 0;
		break;
	case PolPQ:
		if(polCount == 4)
			polIndex = 1;
		break;
	case PolQP:
		if(polCount == 4)
			polIndex = 2;
		break;
	case PolQQ:
		if(polCount == 4)
			polIndex = 3;
		else if(polCount == 2)
			polIndex = 1;
		break;
	}
	if(polIndex >= 0)
	{
		startLine(_plot, s.str(), lineIndex, yDesc);
		for(std::map<double, DefaultStatistics>::const_iterator i=statistics.begin();i!=statistics.end();++i)
		{
			const double x = i->first;
			std::complex<long double> val;
			
			if(pol == PolI) {
				const std::complex<long double>
					valA = derivator.GetComplexStatistic(kind, i->second, 0),
					valB = derivator.GetComplexStatistic(kind, i->second, polCount-1);
				val = valA*0.5l + valB*0.5l;
			}
			else {
				val = derivator.GetComplexStatistic(kind, i->second, polIndex);
			}
			_plot.PushDataPoint(x, getValue(phase, val));
		}
	}
}

std::string AOQPlotPageController::getYDesc(const std::set<QualityTablesFormatter::StatisticKind>& kinds) const
{
	if(kinds.size() != 1)
		return "Value";
	else
	{
		QualityTablesFormatter::StatisticKind kind = *kinds.begin();
		return StatisticsDerivator::GetDescWithUnits(kind);
	}
}

void AOQPlotPageController::SavePdf(const string& filename, QualityTablesFormatter::StatisticKind kind)
{
	std::set<QualityTablesFormatter::StatisticKind> kinds{kind};
	std::set<SelectedPol> pols{PolI};
	std::set<PhaseType> phases{AmplitudePhaseType};

	updatePlotForSettings(kinds, pols, phases);
	
	_plot.SavePdf(filename);
}

void AOQPlotPageController::SetStatistics(const StatisticsCollection* statCollection, const std::vector<class AntennaInfo>& antennas)
{
	processStatistics(statCollection, antennas);
	
	_statCollection = statCollection;
	UpdatePlot();
}
