#include "../structures/timefrequencydata.h"

#include <gtkmm/actiongroup.h>
#include <gtkmm/box.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/image.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubar.h>
#include <gtkmm/menutoolbutton.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/radiotoolbutton.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/toolbutton.h>

#include <string>
#include <vector>

class RFIGuiMenu {
 public:
  RFIGuiMenu();

  Gtk::MenuBar& Menu() { return _menuBar; }
  Gtk::Toolbar& Toolbar() { return _toolbar; }

  // File
  sigc::signal<void> OnOpen;
  sigc::signal<void, std::size_t> OnOpenRecent;
  sigc::signal<void> OnSaveBaselineFlags, OnExportBaseline, OnClose, OnQuit,
      OnAbout;

  // View
  sigc::signal<void> OnViewMode, OnViewData, OnViewStrategy, OnViewTimePlot,
      OnImagePropertiesPressed, OnToggleFlags, OnHightlightPressed, OnZoomFit,
      OnZoomIn, OnZoomOut, OnShowStats;

  // Strategy
  sigc::signal<void> OnExecutePythonStrategy, OnExecuteLuaStrategy,
      OnStrategyNewEmpty, OnStrategyNewTemplate, OnStrategyNewDefault,
      OnStrategyOpen;
  sigc::signal<void, std::string> OnStrategyOpenDefault;
  sigc::signal<void> OnStrategySave, OnStrategySaveAs;

  // Plot
  sigc::signal<void> OnPlotDistPressed;
  sigc::signal<void> OnPlotLogLogDistPressed;
  sigc::signal<void> OnPlotComplexPlanePressed;
  sigc::signal<void> OnPlotMeanSpectrumPressed;
  sigc::signal<void> OnPlotSumSpectrumPressed;
  sigc::signal<void> OnPlotPowerSpectrumPressed;
  sigc::signal<void> OnPlotFrequencyScatterPressed;
  sigc::signal<void> OnPlotTimeMeanPressed;
  sigc::signal<void> OnPlotTimeScatterPressed;
  sigc::signal<void> OnPlotSingularValuesPressed;

  // Browse
  sigc::signal<void> OnLoadPrevious;
  sigc::signal<void> OnReloadPressed;
  sigc::signal<void> OnLoadNext;
  sigc::signal<void> OnGoToPressed;
  sigc::signal<void> OnLoadLongestBaselinePressed;
  sigc::signal<void> OnLoadShortestBaselinePressed;
  sigc::signal<void> OnLoadMedianBaselinePressed;

  // Simulate
  sigc::signal<void> OnGaussianTestSets;
  sigc::signal<void> OnRayleighTestSets;
  sigc::signal<void> OnZeroTestSets;

  sigc::signal<void> OnOpenTestSetA;
  sigc::signal<void> OnOpenTestSetB;
  sigc::signal<void> OnOpenTestSetC;
  sigc::signal<void> OnOpenTestSetD;
  sigc::signal<void> OnOpenTestSetE;
  sigc::signal<void> OnOpenTestSetF;
  sigc::signal<void> OnOpenTestSetG;
  sigc::signal<void> OnOpenTestSetH;
  sigc::signal<void> OnOpenTestSetNoise;
  sigc::signal<void> OnOpenTestSet3Model;
  sigc::signal<void> OnOpenTestSet5Model;
  sigc::signal<void> OnOpenTestSetNoise3Model;
  sigc::signal<void> OnOpenTestSetNoise5Model;
  sigc::signal<void> OnOpenTestSetBStrong;
  sigc::signal<void> OnOpenTestSetBWeak;
  sigc::signal<void> OnOpenTestSetBAligned;

  sigc::signal<void> OnOpenTestSetGaussianBroadband;
  sigc::signal<void> OnOpenTestSetSinusoidalBroadband;
  sigc::signal<void> OnOpenTestSetSlewedGaussianBroadband;
  sigc::signal<void> OnOpenTestSetBurstBroadband;
  sigc::signal<void> OnOpenTestSetRFIDistributionLow;
  sigc::signal<void> OnOpenTestSetRFIDistributionMid;
  sigc::signal<void> OnOpenTestSetRFIDistributionHigh;
  sigc::signal<void> OnOpenTestSetSpike;

  sigc::signal<void> OnAddStaticFringe;
  sigc::signal<void> OnAdd1SigmaFringe;
  sigc::signal<void> OnSetToOne;
  sigc::signal<void> OnSetToI;
  sigc::signal<void> OnSetToOnePlusI;
  sigc::signal<void> OnAddCorrelatorFault;
  sigc::signal<void> OnAddNaNs;
  sigc::signal<void> OnMultiplyData;

  sigc::signal<void> OnSimulateCorrelation;
  sigc::signal<void> OnSimulateSourceSetA;
  sigc::signal<void> OnSimulateSourceSetB;
  sigc::signal<void> OnSimulateSourceSetC;
  sigc::signal<void> OnSimulateSourceSetD;
  sigc::signal<void> OnSimulateOffAxisSource;
  sigc::signal<void> OnSimulateOnAxisSource;

  // Data
  sigc::signal<void> OnVisualizedToOriginalPressed;
  sigc::signal<void> OnKeepRealPressed;
  sigc::signal<void> OnKeepImaginaryPressed;
  sigc::signal<void> OnKeepPhasePressed;
  sigc::signal<void> OnUnrollPhaseButtonPressed;

  sigc::signal<void> OnKeepStokesIPressed;
  sigc::signal<void> OnKeepStokesQPressed;
  sigc::signal<void> OnKeepStokesUPressed;
  sigc::signal<void> OnKeepStokesVPressed;
  sigc::signal<void> OnKeepRRPressed;
  sigc::signal<void> OnKeepRLPressed;
  sigc::signal<void> OnKeepLRPressed;
  sigc::signal<void> OnKeepLLPressed;
  sigc::signal<void> OnKeepXXPressed;
  sigc::signal<void> OnKeepXYPressed;
  sigc::signal<void> OnKeepYXPressed;
  sigc::signal<void> OnKeepYYPressed;

  sigc::signal<void> OnStoreData;
  sigc::signal<void> OnRecallData;
  sigc::signal<void> OnSubtractDataFromMem;
  sigc::signal<void> OnClearOriginalFlagsPressed;
  sigc::signal<void> OnClearAltFlagsPressed;

  // Segmentation
  sigc::signal<void> OnSegment;
  sigc::signal<void> OnCluster;
  sigc::signal<void> OnClassify;
  sigc::signal<void> OnRemoveSmallSegments;

  // Toolbar signals (some are already covered)
  sigc::signal<void> OnTogglePolarizations;
  sigc::signal<void> OnToggleImage;
  sigc::signal<void> OnSelectImage;

  sigc::signal<void, unsigned> openTestSet;

  bool OriginalFlagsActive() const { return _tbOriginalFlags.get_active(); }
  bool AlternativeFlagsActive() const {
    return _tbAlternativeFlags.get_active();
  }

  void SetOriginalFlagsActive(bool originalFlags) {
    _tbOriginalFlags.set_active(originalFlags);
  }
  void SetAlternativeFlagsActive(bool alternativeFlags) {
    _tbAlternativeFlags.set_active(alternativeFlags);
  }

  bool ShowPPActive() const { return _tbDisplayPP.get_active(); }
  bool ShowPQActive() const { return _tbDisplayPQ.get_active(); }
  bool ShowQPActive() const { return _tbDisplayQP.get_active(); }
  bool ShowQQActive() const { return _tbDisplayQQ.get_active(); }

  bool ViewTimePlot() const { return _miViewTimePlot.get_active(); }

  void SetShowPPActive(bool active) { _tbDisplayPP.set_active(active); }
  void SetShowPQActive(bool active) { _tbDisplayPQ.set_active(active); }
  void SetShowQPActive(bool active) { _tbDisplayQP.set_active(active); }
  void SetShowQQActive(bool active) { _tbDisplayQQ.set_active(active); }

  void SetShowPPSensitive(bool sensitive) {
    _tbDisplayPP.set_sensitive(sensitive);
  }
  void SetShowPQSensitive(bool sensitive) {
    _tbDisplayPQ.set_sensitive(sensitive);
  }
  void SetShowQPSensitive(bool sensitive) {
    _tbDisplayQP.set_sensitive(sensitive);
  }
  void SetShowQQSensitive(bool sensitive) {
    _tbDisplayQQ.set_sensitive(sensitive);
  }

  bool SimulateNCPActive() const { return _miSimNCP.get_active(); }
  bool SimulateB1834Active() const { return _miSimB1834.get_active(); }

  bool Simulate16ChActive() const { return _miSim16channels.get_active(); }
  bool Simulate64ChActive() const { return _miSim64channels.get_active(); }
  bool SimFixBandwidthActive() const { return _miSimFixBandwidth.get_active(); }

  void SetZoomToFitSensitive(bool sensitive) {
    _tbZoomFit.set_sensitive(sensitive);
  }
  void SetZoomOutSensitive(bool sensitive) {
    _tbZoomOut.set_sensitive(sensitive);
  }
  void SetZoomInSensitive(bool sensitive) {
    _tbZoomIn.set_sensitive(sensitive);
  }

  void SetPreviousSensitive(bool sensitive) {
    _tbPrevious.set_sensitive(sensitive);
  }
  void SetNextSensitive(bool sensitive) { _tbNext.set_sensitive(sensitive); }
  void SetReloadSensitive(bool sensitive) {
    _tbReload.set_sensitive(sensitive);
  }

  void SetSelectVisualizationSensitive(bool sensitive) {
    _tbSelectVisualization.set_sensitive(sensitive);
  }

  void SetRecentFiles(const std::vector<std::string>& recentFiles);

  void SetStrategyDefaults(const std::vector<std::string>& recentFiles);

  Gtk::Menu& VisualizationMenu() { return _tfVisualizationMenu; }

  void BlockVisualizationSignals() { _blockVisualizationSignals = true; }
  void UnblockVisualizationSignals() { _blockVisualizationSignals = false; }

  void EnableRunButtons(bool sensitive);

  void ActivateDataMode() { _miViewData.activate(); }
  void ActivateStrategyMode() { _miViewStrategy.activate(); }

 private:
  struct ImgMenuItem {
    Gtk::ImageMenuItem item;
    Gtk::HBox box;
    Gtk::AccelLabel label;
    Gtk::Image image;
  };

  void topMenu(Gtk::Menu& menu, Gtk::MenuItem& item, const char* label) {
    item.set_submenu(menu);
    item.set_label(label);
    item.set_use_underline(true);
    _menuBar.append(item);
  }

  void addItem(Gtk::Menu& menu, Gtk::SeparatorMenuItem& sep) {
    menu.append(sep);
  }

  void addItem(Gtk::Menu& menu, Gtk::MenuItem& item, const char* label) {
    item.set_label(label);
    item.set_use_underline(true);
    menu.append(item);
  }

  void addItem(Gtk::Menu& menu, ImgMenuItem& item, const char* label,
               const char* icon) {
    item.image.set_from_icon_name(icon, Gtk::BuiltinIconSize::ICON_SIZE_MENU);
    item.item.set_label(label);
    item.item.set_use_underline(true);
    item.item.set_image(item.image);
    menu.append(item.item);
  }

  template <typename SigType>
  void addItem(Gtk::Menu& menu, Gtk::MenuItem& item, const SigType& sig,
               const char* label) {
    item.set_label(label);
    item.set_use_underline(true);
    item.signal_activate().connect(sig);
    menu.append(item);
  }

  template <typename SigType>
  void addItem(Gtk::Menu& menu, ImgMenuItem& item, const SigType& sig,
               const char* label, const char* icon) {
    // item.box.set_halign(Gtk::ALIGN_START);
    item.image.set_from_icon_name(icon, Gtk::BuiltinIconSize::ICON_SIZE_MENU);
    // item.box.pack_start(item.image);
    // item.label.set_label(label);
    // item.box.pack_start(item.label);
    item.item.set_label(label);
    item.item.set_use_underline(true);
    item.item.set_image(item.image);
    item.item.signal_activate().connect(sig);
    // item.item.add(item.box);
    menu.append(item.item);
  }

  template <typename SigType>
  void addTool(Gtk::ToolButton& tool, const SigType& sig, const char* label,
               const char* tooltip, const char* icon) {
    tool.set_label(label);
    tool.set_tooltip_text(tooltip);
    tool.set_icon_name(icon);
    tool.signal_clicked().connect(sig);
    _toolbar.append(tool);
  }

  void tooltip(ImgMenuItem& item, const char* tooltipStr) {}

  void onTBModeData();
  void onTBModeStrategy();
  void onMIViewData();
  void onMIViewStrategy();

  void makeFileMenu();
  void makeViewMenu();
  void makeStrategyMenu();
  void makePlotMenu();
  void makeBrowseMenu();
  void makeSimulateMenu();
  void makeDataMenu();
  void makeSegmentationMenu();
  void makeToolbarActions();

  Gtk::MenuBar _menuBar;
  Gtk::Toolbar _toolbar;

  Gtk::Menu _menuFile, _menuView, _menuStrategy, _menuPlot, _menuBrowse,
      _menuSimulate, _menuData, _menuSelectComplex, _menuSelectStokes,
      _menuSelectCircular, _menuSelectLinear, _menuSegmentation,
      _menuRecentFiles;
  Gtk::MenuItem _miFile, _miView, _miStrategy, _miPlot, _miBrowse, _miSimulate,
      _miData;
  ImgMenuItem _miRecentFiles;

  // File menu
  Gtk::SeparatorMenuItem _miFileSep1, _miFileSep2, _miFileSep3;
  ImgMenuItem _miFileOpen;
  ImgMenuItem _miSaveBaselineFlags;
  Gtk::MenuItem _miFileExportBaseline;
  ImgMenuItem _miFileClose, _miHelpAbout, _miFileQuit;
  std::vector<Gtk::MenuItem> _miRecentFileChoices;

  // View menu
  Gtk::RadioButtonGroup _viewModeGroup;
  Gtk::Menu _menuViewMode;
  Gtk::MenuItem _miViewMode;
  Gtk::RadioMenuItem _miViewData, _miViewStrategy;
  Gtk::MenuItem _miViewProperties;
  Gtk::CheckMenuItem _miViewTimePlot;
  ImgMenuItem _miViewOriginalFlags, _miViewAlternativeFlags;
  Gtk::SeparatorMenuItem _miViewSep1, _miViewSep2, _miViewSep3;
  ImgMenuItem _miViewZoomFit, _miViewZoomIn, _miViewZoomOut;
  Gtk::MenuItem _miViewStats;

  // Strategy menu
  Gtk::SeparatorMenuItem _miStrategySep1, _miStrategySep2, _miStrategySep3,
      _miStrategySep4;
  ImgMenuItem _miStrategyNew, _miStrategyOpen, _miStrategyOpenDefault,
      _miStrategySave, _miStrategySaveAs;
  Gtk::Menu _menuStrategyNew, _menuStrategyOpenDefault;
  std::vector<Gtk::MenuItem> _miStrategyDefaults;
  Gtk::MenuItem _miStrategyNewEmpty, _miStrategyNewTemplate,
      _miStrategyNewDefault;
  ImgMenuItem _miExecuteLuaStrategy;
  Gtk::MenuItem _miExecutePythonStrategy;

  // Plot menu
  Gtk::MenuItem _miPlotTime, _miPlotFrequency;
  Gtk::Menu _menuPlotTime, _menuPlotFrequency;
  Gtk::MenuItem _miPlotDistribution, _miPlotLogLogDistribution,
      _miPlotComplexPlane, _miPlotMeanSpectrum;
  Gtk::MenuItem _miPlotSumSpectrum, _miPlotPowerSpectrum,
      _miPlotFrequencyScatter, _miPlotTimeMean;
  Gtk::MenuItem _miPlotTimeScatter, _miPlotSingularValues;

  // Browse menu
  Gtk::SeparatorMenuItem _miBrowseSep1, _miBrowseSep2;
  ImgMenuItem _miBrowsePrevious, _miBrowseReload, _miBrowseNext;
  Gtk::MenuItem _miBrowseGoto, _miBrowseLongestBaseline,
      _miBrowseMedianBaseline, _miBrowseShortestBaseline;

  // Simulate menu
  Gtk::Menu _menuTestSets;
  Gtk::MenuItem _miTestSetSubMenu;
  Gtk::SeparatorMenuItem _miTestSep1;
  Gtk::RadioButtonGroup _testSetGroup;
  Gtk::RadioMenuItem _miTestGaussian, _miTestRayleigh, _miTestZero;
  Gtk::MenuItem _miTestA, _miTestB, _miTestC, _miTestD, _miTestE;
  Gtk::MenuItem _miTestF, _miTestG, _miTestH, _miTestNoise, _miTestModel3;
  Gtk::MenuItem _miTestModel5, _miTestNoiseModel3, _miTestNoiseModel5,
      _miTestBStrong;
  Gtk::MenuItem _miTestBWeak, _miTestBAligned, _miTestGaussianBroadband,
      _miTestSenusoidalBroadband;
  Gtk::MenuItem _miTestSlewedGaussianBroadband, _miTestBurstBroadband;
  Gtk::MenuItem _miTestRFIDistLow, _miTestRFIDistMid, _miTestRFIDistHigh;
  Gtk::MenuItem _miTestSpike;

  Gtk::Menu _menuModify;
  Gtk::MenuItem _miSimulateModify;
  Gtk::MenuItem _miModifyStaticFringe, _miModify1SigmaStaticFringe;
  Gtk::MenuItem _miModifyToOne, _miModifyToI, _miModifyToOnePlusI;
  Gtk::MenuItem _miModifyCorrelatorFault, _miModifyAddNaNs, _miModifyMultiply;

  Gtk::SeparatorMenuItem _miSimSep1, _miSimSep2;
  Gtk::RadioButtonGroup _simSetGroup, _simChGroup;
  Gtk::RadioMenuItem _miSimNCP, _miSimB1834, _miSimEmpty;
  Gtk::RadioMenuItem _miSim16channels, _miSim64channels, _miSim256channels;
  Gtk::CheckMenuItem _miSimFixBandwidth;
  Gtk::MenuItem _miSimCorrelation;
  Gtk::MenuItem _miSimSourceSetA, _miSimSourceSetB, _miSimSourceSetC,
      _miSimSourceSetD;
  Gtk::MenuItem _miSimSourceOffAxis, _miSimSourceOnAxis;

  // Data menu
  Gtk::MenuItem _miDataToOriginal;
  Gtk::SeparatorMenuItem _miDataSep1, _miDataSep2, _miDataSep3;
  Gtk::MenuItem _miSelectComplex;
  Gtk::MenuItem _miDataReal, _miDataImaginary, _miDataPhase, _miDataUnrollPhase;
  Gtk::MenuItem _miSelectStokes;
  Gtk::MenuItem _miDataStokesI, _miDataStokesQ, _miDataStokesU, _miDataStokesV;
  Gtk::MenuItem _miSelectCircular;
  Gtk::MenuItem _miDataRR, _miDataRL, _miDataLR, _miDataLL;
  Gtk::MenuItem _miSelectLinear;
  Gtk::MenuItem _miDataXX, _miDataXY, _miDataYX, _miDataYY;
  Gtk::MenuItem _miSegmentationSubMenu;
  Gtk::MenuItem _miDataStore, _miDataRecall, _miDataSubtract;
  Gtk::MenuItem _miDataClearOriginalFlags, _miDataClearAltFlags;

  // Segmentation menu
  Gtk::MenuItem _miSegmSegment, _miSegmCluster, _miSegmClassify,
      _miSegmRemoveSmallSegments;

  // Toolbar
  Gtk::RadioToolButton::Group _tbModeGroup;
  Gtk::SeparatorToolItem _tbSep1, _tbSep2, _tbSep3, _tbSep4, _tbSep5;
  Gtk::ToolButton _tbOpen, _tbExecuteStrategy;
  Gtk::RadioToolButton _tbModeData, _tbModeStrategy;
  Gtk::ToggleToolButton _tbOriginalFlags, _tbAlternativeFlags;
  Gtk::MenuToolButton _tbSelectVisualization;
  Gtk::ToolButton _tbPrevious, _tbReload, _tbNext;
  Gtk::ToolButton _tbZoomFit, _tbZoomIn, _tbZoomOut;
  Gtk::ToggleToolButton _tbDisplayPP, _tbDisplayPQ, _tbDisplayQP, _tbDisplayQQ;

  bool _inStrategyMode;
  bool _blockVisualizationSignals;

  Gtk::Menu _tfVisualizationMenu;
};
