#include "aoqplotpagecontroller.h"

#include "../twodimensionalplotpage.h"

#include "../../quality/statisticsderivator.h"

#include "../../util/logger.h"

AOQPlotPageController::AOQPlotPageController()
    : _page(nullptr), _statCollection(nullptr) {}

void AOQPlotPageController::UpdatePlot() {
  if (_page != nullptr) {
    updatePlotForSettings(_page->GetSelectedKinds(),
                          _page->GetSelectedPolarizations(),
                          _page->GetSelectedPhases());
  }
}

void AOQPlotPageController::updatePlotForSettings(
    const std::vector<QualityTablesFormatter::StatisticKind>& kinds,
    const std::set<SelectedPol>& pols, const std::set<PhaseType>& phases) {
  if (HasStatistics()) {
    _plot.Clear();

    std::map<std::string, int> units;
    for (const QualityTablesFormatter::StatisticKind k : kinds) {
      const std::string unit_str = StatisticsDerivator::GetUnits(k);
      std::map<std::string, int>::const_iterator iterator =
          units.find(unit_str);
      if (iterator == units.end()) {
        units.emplace(unit_str, units.size() % 2);
      }
    }
    _plot.Y2Axis().SetShow(false);
    int index = 0;
    for (const QualityTablesFormatter::StatisticKind k : kinds) {
      const std::string unit_str = StatisticsDerivator::GetUnits(k);
      const int axis_number = units.find(unit_str)->second;
      const bool multiple_units =
          units.size() > 3 || (units.size() > 2 && (axis_number % 2) == 0);
      XYPlotAxis& y_axis = axis_number == 1 ? _plot.Y2Axis() : _plot.YAxis();
      y_axis.SetShow(true);
      for (const SelectedPol p : pols) {
        for (const PhaseType ph : phases) {
          const std::string description =
              multiple_units ? "Value"
                             : StatisticsDerivator::GetDescWithUnits(k);
          plotStatistic(k, p, ph, index, description, axis_number == 1);
          ++index;
        }
      }
    }

    processPlot(_plot);

    if (_page != nullptr) {
      _page->Redraw();
    }
  }
}

double AOQPlotPageController::getValue(enum PhaseType phase,
                                       const std::complex<long double>& val) {
  switch (phase) {
    default:
    case AmplitudePhaseType:
      return sqrt(val.real() * val.real() + val.imag() * val.imag());
    case PhasePhaseType:
      return atan2(val.imag(), val.real());
    case RealPhaseType:
      return val.real();
    case ImaginaryPhaseType:
      return val.imag();
  }
}

void AOQPlotPageController::plotStatistic(
    QualityTablesFormatter::StatisticKind kind, SelectedPol pol,
    PhaseType phase, int lineIndex, const std::string& yDesc,
    bool second_axis) {
  const StatisticsDerivator derivator(*_statCollection);
  const size_t polCount = _statCollection->PolarizationCount();
  const std::map<double, DefaultStatistics>& statistics = getStatistics();
  std::ostringstream s;
  int polIndex = -1;
  s << StatisticsDerivator::GetDescription(kind);
  switch (pol) {
    case PolI:
      s << ", pol I";
      polIndex = 0;
      break;
    case PolPP:
      s << ", pol PP";
      polIndex = 0;
      break;
    case PolPQ:
      s << ", pol PQ";
      if (polCount == 4) polIndex = 1;
      break;
    case PolQP:
      s << ", pol QP";
      if (polCount == 4) polIndex = 2;
      break;
    case PolQQ:
      s << ", pol QQ";
      if (polCount == 4)
        polIndex = 3;
      else if (polCount == 2)
        polIndex = 1;
      break;
  }
  if (phase == RealPhaseType)
    s << " (real)";
  else if (phase == ImaginaryPhaseType)
    s << " (imag)";
  if (polIndex >= 0) {
    startLine(_plot, s.str(), lineIndex, yDesc, second_axis);
    for (std::map<double, DefaultStatistics>::const_iterator i =
             statistics.begin();
         i != statistics.end(); ++i) {
      const double x = i->first;
      std::complex<long double> val;

      if (pol == PolI) {
        const std::complex<long double> valA =
            derivator.GetComplexStatistic(kind, i->second, 0);
        const std::complex<long double> valB =
            derivator.GetComplexStatistic(kind, i->second, polCount - 1);
        val = valA * 0.5l + valB * 0.5l;
      } else {
        val = derivator.GetComplexStatistic(kind, i->second, polIndex);
      }
      _plot.PushDataPoint(x, getValue(phase, val));
    }
  }
}

void AOQPlotPageController::SavePdf(
    const string& filename, QualityTablesFormatter::StatisticKind kind) {
  const std::vector<QualityTablesFormatter::StatisticKind> kinds{kind};
  const std::set<SelectedPol> pols{PolI};
  const std::set<PhaseType> phases{AmplitudePhaseType};

  updatePlotForSettings(kinds, pols, phases);

  _plot.SavePdf(filename, 640, 480);
}

void AOQPlotPageController::SetStatistics(
    const StatisticsCollection* statCollection,
    const std::vector<class AntennaInfo>& antennas) {
  processStatistics(statCollection, antennas);

  _statCollection = statCollection;
  UpdatePlot();
}
