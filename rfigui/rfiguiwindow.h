#ifndef MSWINDOW_H
#define MSWINDOW_H

#include <set>
#include <memory>
#include <thread>

#include <gtkmm/paned.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/window.h>

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "../imagesets/imagesetindex.h"

#include "../plot/plotwidget.h"
#include "../plot/xyplot.h"

#include "timefrequencywidget.h"
#include "interfaces.h"
#include "plotframe.h"
#include "strategyeditor.h"

#include "../algorithms/enums.h"

namespace Gtk {
class RadioMenuItem;
}

namespace algorithms {
class ThresholdConfig;
}

class HistogramCollection;
struct MSOptions;

class RFIGuiWindow : public Gtk::Window {
 public:
  RFIGuiWindow(class RFIGuiController* controller, const std::string& argv0);
  ~RFIGuiWindow();

  void Update() { _timeFrequencyWidget.Update(); }
  bool HasImage() const { return _timeFrequencyWidget.HasImage(); }
  Mask2DCPtr Mask() const { return GetOriginalData().GetSingleMask(); }
  Mask2DCPtr AltMask() const { return GetActiveData().GetSingleMask(); }

  TimeFrequencyData GetActiveData() const;
  const TimeFrequencyData& GetOriginalData() const;

  PlotWidget& GetHeatMapWidget() {
    return _timeFrequencyWidget.GetHeatMapWidget();
  }
  TimeFrequencyWidget& GetTimeFrequencyWidget() { return _timeFrequencyWidget; }

  algorithms::ThresholdConfig& HighlightConfig();
  void SetHighlighting(bool newValue);
  TimeFrequencyMetaDataCPtr SelectedMetaData();

  void onExecuteStrategyFinished(bool successfull);
  void onExecuteStrategyError(const std::string& error);

  void OpenPaths(const std::vector<std::string>& paths);
  void OpenMS(const std::vector<std::string>& filenames,
              const MSOptions& options);

  void ShowHistogram(HistogramCollection& histogramCollection);

  class RFIGuiController& Controller() {
    return *_controller;
  }

  // void UpdateImageSetIndex();
  void OpenGotoWindow() { onGoToPressed(); }
  void SetBaselineInfo(bool isEmpty, bool hasMultipleBaselines,
                       const std::string& name, const std::string& description);
  void SetImageSetIndex(const imagesets::ImageSetIndex& newImageSetIndex);

 private:
  bool onClose(GdkEventAny* event);
  /// Returns 'false' if the user wants to cancel the operation
  bool askToSaveChanges();
  void onTFWidgetMouseMoved(double x, double y);
  void onTFScroll(double x, double y, int direction);
  void onToggleFlags();
  void onTogglePolarizations();
  void onToggleImage();
  void onTFZoomChanged();
  void onZoomFit();
  void onZoomIn();
  void onZoomOut();
  void onZoomSelect();

  // File menu
  void onOpen();
  void onOpenRecent(size_t index);
  void updateRecentFiles();
  void updateOpenStrategyMenu();
  void onSaveBaselineFlags();
  void onClose();
  void onExportBaseline();
  void onQuit() { hide(); }

  // View menu
  void onViewData();
  void onViewStrategy();

  void onViewTimePlot();

  // Strategy menu
  void onExecuteLuaStrategy();
  void onStrategyNewEmpty();
  void onStrategyNewTemplate();
  void onStrategyNewDefault();
  void onStrategyOpen();
  void onStrategyOpenDefault(const std::string& name);
  void onStrategySave();
  void onStrategySaveAs();

  // Browse menu
  void loadWithProgress();
  void onLoadPrevious();
  void onLoadNext();
  void onSelectImage();

  // Data menu
  void onVisualizedToOriginalPressed();
  void onClearOriginalFlagsPressed();
  void onClearAltFlagsPressed();
  void handleAveraging(bool spectrally);

  void keepPhasePart(
      enum TimeFrequencyData::ComplexRepresentation phaseRepresentation);
  void onKeepRealPressed() { keepPhasePart(TimeFrequencyData::RealPart); }
  void onKeepImaginaryPressed() {
    keepPhasePart(TimeFrequencyData::ImaginaryPart);
  }
  void onKeepPhasePressed() { keepPhasePart(TimeFrequencyData::PhasePart); }
  void keepPolarisation(aocommon::PolarizationEnum polarisation);
  void onKeepStokesIPressed() {
    keepPolarisation(aocommon::Polarization::StokesI);
  }
  void onKeepStokesQPressed() {
    keepPolarisation(aocommon::Polarization::StokesQ);
  }
  void onKeepStokesUPressed() {
    keepPolarisation(aocommon::Polarization::StokesU);
  }
  void onKeepStokesVPressed() {
    keepPolarisation(aocommon::Polarization::StokesV);
  }
  void onKeepRRPressed() { keepPolarisation(aocommon::Polarization::RR); }
  void onKeepRLPressed() { keepPolarisation(aocommon::Polarization::RL); }
  void onKeepLRPressed() { keepPolarisation(aocommon::Polarization::LR); }
  void onKeepLLPressed() { keepPolarisation(aocommon::Polarization::LL); }
  void onKeepXXPressed() { keepPolarisation(aocommon::Polarization::XX); }
  void onKeepXYPressed() { keepPolarisation(aocommon::Polarization::XY); }
  void onKeepYXPressed() { keepPolarisation(aocommon::Polarization::YX); }
  void onKeepYYPressed() { keepPolarisation(aocommon::Polarization::YY); }
  void onImagePropertiesPressed();
  void onOpenTestSetNoise() { openTestSet(algorithms::RFITestSet::Empty); }
  void onOpenTestSetA() { openTestSet(algorithms::RFITestSet::FullBandBursts); }
  void onOpenTestSetB() { openTestSet(algorithms::RFITestSet::HalfBandBursts); }
  void onOpenTestSetC() { openTestSet(algorithms::RFITestSet::VaryingBursts); }
  void onOpenTestSetD() {
    openTestSet(algorithms::RFITestSet::VaryingBursts,
                algorithms::BackgroundTestSet::ThreeSources);
  }
  void onOpenTestSetE() {
    openTestSet(algorithms::RFITestSet::FullBandBursts,
                algorithms::BackgroundTestSet::FiveSources);
  }
  void onOpenTestSetF() {
    openTestSet(algorithms::RFITestSet::VaryingBursts,
                algorithms::BackgroundTestSet::FiveSources);
  }
  void onOpenTestSetG() {
    openTestSet(algorithms::RFITestSet::VaryingBursts,
                algorithms::BackgroundTestSet::FiveFilteredSources);
  }
  void onOpenTestSetH() {
    openTestSet(algorithms::RFITestSet::VaryingBursts,
                algorithms::BackgroundTestSet::HighFrequency);
  }
  void onAddStaticFringe();
  void onAdd1SigmaFringe();
  void onSetToOne();
  void onSetToI();
  void onSetToOnePlusI();
  void onAddCorrelatorFault();
  void onAddNaNs();
  void onShowStats();
  void onPlotDistPressed();
  void onPlotLogLogDistPressed();
  void onPlotMeanSpectrumPressed();
  void onPlotSumSpectrumPressed();
  void onPlotPowerSpectrumPressed();
  void onPlotFrequencyScatterPressed();
  void onPlotTimeMeanPressed();
  void onPlotTimeScatterPressed();
  void onPlotSingularValuesPressed();
  void onReloadPressed();
  void onGoToPressed();
  void onLoadExtremeBaseline(bool longest);
  void onLoadMedianBaseline();
  void onMultiplyData();
  void onSegment();
  void onCluster();
  void onClassify();
  void onRemoveSmallSegments();
  void onUnrollPhaseButtonPressed();
  void onStoreData();
  void onRecallData();
  void onSubtractDataFromMem();

  void showError(const std::string& description);
  void setSetNameInStatusBar();

  void onSimulate();
  void onHelpAbout();

  void openTestSet(algorithms::RFITestSet rfiSet,
                   algorithms::BackgroundTestSet backgroundSet =
                       algorithms::BackgroundTestSet::Empty);

  void onControllerStateChange();

  void onExecutePythonStrategy();

  void updatePolarizations();

  void updateTFVisualizationMenu();
  size_t getActiveTFVisualization();

  class RFIGuiController* _controller;

  Gtk::Box _mainVBox;

  TimeFrequencyWidget _timeFrequencyWidget;

  StrategyEditor _strategyEditor;
  Gtk::Statusbar _statusbar;
  std::string _imageSetName, _imageSetIndexDescription;

  std::unique_ptr<class HistogramWindow> _histogramWindow;
  std::unique_ptr<class PlotWindow> _plotWindow;
  std::unique_ptr<Gtk::Window> _gotoWindow, _imagePropertiesWindow;
  std::unique_ptr<class ProgressWindow> _progressWindow;
  std::unique_ptr<class RFIGuiMenu> _menu;

  TimeFrequencyData _storedData;
  TimeFrequencyMetaDataCPtr _storedMetaData;
  std::vector<std::unique_ptr<Gtk::RadioMenuItem>> _tfVisualizationMenuItems;
  std::thread _taskThread;
};

#endif
