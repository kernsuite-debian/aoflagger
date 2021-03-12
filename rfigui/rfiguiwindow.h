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
#include "../plot/plot2d.h"

#include "timefrequencywidget.h"
#include "interfaces.h"
#include "plotframe.h"
#include "strategyeditor.h"

#include "../imaging/defaultmodels.h"

namespace Gtk {
class RadioMenuItem;
}

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
  // const TimeFrequencyData &GetContaminatedData() const;

  HeatMapWidget& GetHeatMap() { return _timeFrequencyWidget.HeatMap(); }
  TimeFrequencyWidget& GetTimeFrequencyWidget() { return _timeFrequencyWidget; }

  class ThresholdConfig& HighlightConfig();
  void SetHighlighting(bool newValue);
  TimeFrequencyMetaDataCPtr SelectedMetaData();

  void onExecuteStrategyFinished(bool successfull);
  void onExecuteStrategyError(const std::string& error);

  void OpenPaths(const std::vector<std::string>& paths);
  void OpenMS(const std::vector<std::string>& filenames,
              const class MSOptions& options);

  void ShowHistogram(class HistogramCollection& histogramCollection);

  class RFIGuiController& Controller() {
    return *_controller;
  }

  // void UpdateImageSetIndex();
  void OpenGotoWindow() { onGoToPressed(); }
  void SetBaselineInfo(bool isEmpty, bool hasMultipleBaselines,
                       const std::string& name, const std::string& description);
  void SetImageSetIndex(const rfiStrategy::ImageSetIndex& newImageSetIndex);

 private:
  void onTFWidgetMouseMoved(size_t x, size_t y);
  void onTFScroll(size_t x, size_t y, int direction);
  void onToggleFlags();
  void onTogglePolarizations();
  void onToggleImage();
  void onTFZoomChanged();
  void onZoomFit();
  void onZoomIn();
  void onZoomOut();

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
  void onEditStrategyPressed();
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
  void onOpenTestSetNoise() { openTestSet(2); }
  void onOpenTestSetA() { openTestSet(3); }
  void onOpenTestSetB() { openTestSet(4); }
  void onOpenTestSetC() { openTestSet(5); }
  void onOpenTestSetD() { openTestSet(18); }
  void onOpenTestSetE() { openTestSet(14); }
  void onOpenTestSetF() { openTestSet(16); }
  void onOpenTestSetG() { openTestSet(17); }
  void onOpenTestSetH() { openTestSet(7); }
  void onOpenTestSetNoise3Model() { openTestSet(19); }
  void onOpenTestSetNoise5Model() { openTestSet(20); }
  void onOpenTestSet3Model() { openTestSet(21); }
  void onOpenTestSet5Model() { openTestSet(22); }
  void onOpenTestSetBStrong() { openTestSet(24); }
  void onOpenTestSetBWeak() { openTestSet(23); }
  void onOpenTestSetBAligned() { openTestSet(25); }
  void onOpenTestSetGaussianBroadband() { openTestSet(26); }
  void onOpenTestSetSinusoidalBroadband() { openTestSet(27); }
  void onOpenTestSetSlewedGaussianBroadband() { openTestSet(28); }
  void onOpenTestSetBurstBroadband() { openTestSet(29); }
  void onOpenTestSetRFIDistributionLow() { openTestSet(32); }
  void onOpenTestSetRFIDistributionMid() { openTestSet(31); }
  void onOpenTestSetRFIDistributionHigh() { openTestSet(30); }
  void onOpenTestSetSpike() { openTestSet(1000); }
  void onGaussianTestSets() { _gaussianTestSets = 1; }
  void onRayleighTestSets() { _gaussianTestSets = 0; }
  void onZeroTestSets() { _gaussianTestSets = 2; }
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
  void onPlotComplexPlanePressed();
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

  DefaultModels::SetLocation getSetLocation(bool empty = false);
  void loadDefaultModel(DefaultModels::Distortion distortion, bool withNoise,
                        bool empty = false);
  void onSimulateCorrelation() {
    loadDefaultModel(DefaultModels::ConstantDistortion, false);
  }
  void onSimulateSourceSetA() {
    loadDefaultModel(DefaultModels::ConstantDistortion, true);
  }
  void onSimulateSourceSetB() {
    loadDefaultModel(DefaultModels::VariableDistortion, true);
  }
  void onSimulateSourceSetC() {
    loadDefaultModel(DefaultModels::FaintDistortion, true);
  }
  void onSimulateSourceSetD() {
    loadDefaultModel(DefaultModels::MislocatedDistortion, true);
  }
  void onSimulateOffAxisSource() {
    loadDefaultModel(DefaultModels::ConstantDistortion, false, true);
  }
  void onSimulateOnAxisSource() {
    loadDefaultModel(DefaultModels::OnAxisSource, false, true);
  }

  void onHelpAbout();

  void openTestSet(unsigned index);

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
  std::unique_ptr<Gtk::Window> _gotoWindow, _plotComplexPlaneWindow,
      _imagePropertiesWindow;
  std::unique_ptr<class MSOptionWindow> _msOptionWindow;
  std::unique_ptr<class ProgressWindow> _progressWindow;
  std::unique_ptr<class RFIGuiMenu> _menu;

  int _gaussianTestSets;
  SegmentedImagePtr _segmentedImage;
  TimeFrequencyData _storedData;
  TimeFrequencyMetaDataCPtr _storedMetaData;
  std::vector<std::unique_ptr<Gtk::RadioMenuItem>> _tfVisualizationMenuItems;
  std::thread _taskThread;
};

#endif
