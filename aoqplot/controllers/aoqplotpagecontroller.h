#ifndef AOQ_PLOT_PAGE_CONTROLLER
#define AOQ_PLOT_PAGE_CONTROLLER

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../../quality/qualitytablesformatter.h"

#include "../../plot/xyplot.h"

#include "aoqpagecontroller.h"

class AOQPlotPageController : public AOQPageController {
 public:
  enum SelectedPol { PolPP, PolPQ, PolQP, PolQQ, PolI };
  AOQPlotPageController();

  void Attach(class TwoDimensionalPlotPage* page) { _page = page; }

  void SetStatistics(
      const StatisticsCollection* statCollection,
      const std::vector<class AntennaInfo>& antennas) override final;

  void CloseStatistics() override final { _statCollection = nullptr; }

  bool HasStatistics() const { return _statCollection != nullptr; }

  void SavePdf(const std::string& filename,
               QualityTablesFormatter::StatisticKind kind);

  XYPlot& Plot() { return _plot; }

  void UpdatePlot();

  enum PhaseType {
    AmplitudePhaseType,
    PhasePhaseType,
    RealPhaseType,
    ImaginaryPhaseType
  };

 protected:
  virtual void processStatistics(const StatisticsCollection*,
                                 const std::vector<AntennaInfo>&) {}

  virtual const std::map<double, class DefaultStatistics>& getStatistics()
      const = 0;

  virtual void startLine(XYPlot& plot, const std::string& name, int lineIndex,
                         const std::string& yAxisDesc, bool second_axis) = 0;

  virtual void processPlot(XYPlot& plot) {}

  const StatisticsCollection* getStatCollection() const {
    return _statCollection;
  }

  class TwoDimensionalPlotPage* page() {
    return _page;
  }

 private:
  class TwoDimensionalPlotPage* _page;

  void updatePlotForSettings(
      const std::vector<QualityTablesFormatter::StatisticKind>& kinds,
      const std::set<SelectedPol>& pols, const std::set<PhaseType>& phases);

  double getValue(enum PhaseType Phase, const std::complex<long double>& val);

  void plotStatistic(QualityTablesFormatter::StatisticKind kind,
                     SelectedPol pol, PhaseType phase, int lineIndex,
                     const std::string& yDesc, bool second_axis);
  const StatisticsCollection* _statCollection;
  XYPlot _plot;
};

#endif
