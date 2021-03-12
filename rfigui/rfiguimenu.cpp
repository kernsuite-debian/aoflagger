#include "rfiguimenu.h"

#include <gtkmm/icontheme.h>

RFIGuiMenu::RFIGuiMenu()
    :  // older versions of gtkmm don't have the 'empty' Gtk::RadioMenuItem()
       // constructor, so we have to initialize them here.
      _miViewData(_viewModeGroup),
      _miViewStrategy(_viewModeGroup),
      _miTestGaussian(_testSetGroup),
      _miTestRayleigh(_testSetGroup),
      _miTestZero(_testSetGroup),
      _miSimNCP(_simSetGroup),
      _miSimB1834(_simSetGroup),
      _miSimEmpty(_simSetGroup),
      _miSim16channels(_simChGroup),
      _miSim64channels(_simChGroup),
      _miSim256channels(_simChGroup),
      _tbModeData(_tbModeGroup),
      _tbModeStrategy(_tbModeGroup),
      _inStrategyMode(false),
      _blockVisualizationSignals(false) {
  topMenu(_menuFile, _miFile, "_File");
  topMenu(_menuView, _miView, "_View");
  topMenu(_menuStrategy, _miStrategy, "_Strategy");
  topMenu(_menuPlot, _miPlot, "_Plot");
  topMenu(_menuBrowse, _miBrowse, "_Browse");
  topMenu(_menuSimulate, _miSimulate, "S_imulate");
  topMenu(_menuData, _miData, "_Data");

  makeFileMenu();
  makeViewMenu();
  makeStrategyMenu();
  makePlotMenu();
  makeBrowseMenu();
  makeSimulateMenu();
  makeDataMenu();
  makeToolbarActions();

  if (Gtk::IconTheme::get_default()->has_icon("aoflagger")) {
    _toolbar.set_toolbar_style(Gtk::TOOLBAR_ICONS);
    _toolbar.set_icon_size(Gtk::ICON_SIZE_LARGE_TOOLBAR);
  } else {
    _toolbar.set_toolbar_style(Gtk::TOOLBAR_TEXT);
    _toolbar.set_icon_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
  }
}

void RFIGuiMenu::makeFileMenu() {
  // Gtk::AccelKey("<control>O")
  addItem(_menuFile, _miFileOpen, OnOpen, "_Open...", "document-open");

  _miRecentFiles.item.set_submenu(_menuRecentFiles);
  addItem(_menuFile, _miRecentFiles, "Open _recent", "document-open");

  addItem(_menuFile, _miFileSep1);
  addItem(_menuFile, _miSaveBaselineFlags, OnSaveBaselineFlags,
          "Write baseline flags", "document-save");
  addItem(_menuFile, _miFileExportBaseline, OnExportBaseline,
          "Export baseline...");

  addItem(_menuFile, _miFileSep2);
  addItem(_menuFile, _miFileClose, OnClose, "Close", "window-close");

  // <control>Q
  addItem(_menuFile, _miFileSep3);
  addItem(_menuFile, _miHelpAbout, OnAbout, "_About", "aoflagger");
  addItem(_menuFile, _miFileQuit, OnQuit, "_Quit", "application-exit");
}

void RFIGuiMenu::makeViewMenu() {
  addItem(
      _menuViewMode, _miViewData, [&]() { onMIViewData(); }, "Data");
  // F8
  addItem(
      _menuViewMode, _miViewStrategy, [&]() { onMIViewStrategy(); },
      "Strategy");

  _miViewMode.set_submenu(_menuViewMode);
  addItem(_menuView, _miViewMode, "Mode");
  addItem(_menuView, _miViewTimePlot, OnViewTimePlot, "Time plot");

  // <control>P
  addItem(_menuView, _miViewProperties, OnImagePropertiesPressed,
          "Plot properties...");
  addItem(_menuView, _miViewSep1);

  addItem(
      _menuView, _miViewOriginalFlags,
      [&]() {
        if (!_blockVisualizationSignals) OnToggleFlags();
      },
      "Ori. flags", "showoriginalflags");

  addItem(
      _menuView, _miViewAlternativeFlags,
      [&]() {
        if (!_blockVisualizationSignals) OnToggleFlags();
      },
      "Alt. flags", "showalternativeflags");
  addItem(_menuView, _miViewSep2);

  addItem(
      _menuView, _miViewZoomFit,
      [&]() {
        if (!_blockVisualizationSignals) OnZoomFit();
      },
      "Zoom _fit", "zoom-fit-best");
  addItem(
      _menuView, _miViewZoomIn,
      [&]() {
        if (!_blockVisualizationSignals) OnZoomIn();
      },
      "Zoom in", "zoom-in");
  addItem(
      _menuView, _miViewZoomOut,
      [&]() {
        if (!_blockVisualizationSignals) OnZoomOut();
      },
      "Zoom out", "zoom-out");
  addItem(_menuView, _miViewSep3);

  // F2
  addItem(_menuView, _miViewStats, OnShowStats, "Statistics");
}

void RFIGuiMenu::makeStrategyMenu() {
  addItem(_menuStrategyNew, _miStrategyNewEmpty, OnStrategyNewEmpty, "Empty");
  addItem(_menuStrategyNew, _miStrategyNewTemplate, OnStrategyNewTemplate,
          "Template");
  addItem(_menuStrategyNew, _miStrategyNewDefault, OnStrategyNewDefault,
          "Default");
  _miStrategyNew.item.set_submenu(_menuStrategyNew);

  addItem(_menuStrategy, _miStrategyNew, "New", "document-new");
  addItem(_menuStrategy, _miStrategySep1);

  addItem(_menuStrategy, _miStrategyOpen, OnStrategyOpen, "Open...",
          "document-open");

  _miStrategyOpenDefault.item.set_submenu(_menuStrategyOpenDefault);
  addItem(_menuStrategy, _miStrategyOpenDefault, "Open _default",
          "document-open");
  addItem(_menuStrategy, _miStrategySep2);

  addItem(_menuStrategy, _miStrategySave, OnStrategySave, "Save",
          "document-save");
  addItem(_menuStrategy, _miStrategySaveAs, OnStrategySaveAs, "Save as...",
          "document-save-as");
  addItem(_menuStrategy, _miStrategySep3);

  // F9
  addItem(_menuStrategy, _miExecuteLuaStrategy, OnExecuteLuaStrategy,
          "E_xecute strategy", "system-run");
  addItem(_menuStrategy, _miStrategySep4);

  addItem(_menuStrategy, _miExecutePythonStrategy, OnExecutePythonStrategy,
          "Execute _python script");
  // TODO Functionality is broken and should be removed, but
  // it might be interesting to do a comparison of Lua vs Python strategies
  // first
  _miExecutePythonStrategy.set_sensitive(false);
}

void RFIGuiMenu::makePlotMenu() {
  _miPlotTime.set_submenu(_menuPlotTime);
  addItem(_menuPlot, _miPlotTime, "Time");

  addItem(_menuPlotTime, _miPlotTimeMean, OnPlotTimeMeanPressed,
          "_Mean (low res)");
  addItem(_menuPlotTime, _miPlotTimeScatter, OnPlotTimeScatterPressed,
          "Mean (_scatter)");

  _miPlotFrequency.set_submenu(_menuPlotFrequency);
  addItem(_menuPlot, _miPlotFrequency, "Frequency");

  addItem(_menuPlotFrequency, _miPlotPowerSpectrum, OnPlotPowerSpectrumPressed,
          "_Power (low res)");
  addItem(_menuPlotFrequency, _miPlotFrequencyScatter,
          OnPlotFrequencyScatterPressed, "_Power (scatter)");
  addItem(_menuPlotFrequency, _miPlotMeanSpectrum, OnPlotMeanSpectrumPressed,
          "_Mean");
  addItem(_menuPlotFrequency, _miPlotSumSpectrum, OnPlotSumSpectrumPressed,
          "_S_um");

  addItem(_menuPlot, _miPlotDistribution, OnPlotDistPressed,
          "Plot _distribution");
  addItem(_menuPlot, _miPlotLogLogDistribution, OnPlotLogLogDistPressed,
          "Plot _log-log dist");
  addItem(_menuPlot, _miPlotComplexPlane, OnPlotComplexPlanePressed,
          "Plot _complex plane");
  addItem(_menuPlot, _miPlotSingularValues, OnPlotSingularValuesPressed,
          "Plot _singular values");
}

void RFIGuiMenu::makeBrowseMenu() {
  // F6
  addItem(_menuBrowse, _miBrowsePrevious, OnLoadPrevious, "Previous",
          "go-previous");
  _miBrowsePrevious.item.set_sensitive(false);
  // F5
  addItem(_menuBrowse, _miBrowseReload, OnReloadPressed, "_Reload",
          "view-refresh");
  _miBrowseReload.item.set_sensitive(false);
  // F7
  addItem(_menuBrowse, _miBrowseNext, OnLoadPrevious, "Next", "go-next");
  _miBrowseNext.item.set_sensitive(false);
  addItem(_menuBrowse, _miBrowseSep1);
  // "<control>G"
  addItem(_menuBrowse, _miBrowseGoto, OnGoToPressed, "_Go to...");
  addItem(_menuBrowse, _miBrowseSep2);
  addItem(_menuBrowse, _miBrowseLongestBaseline, OnLoadLongestBaselinePressed,
          "Longest baseline");
  addItem(_menuBrowse, _miBrowseMedianBaseline, OnLoadMedianBaselinePressed,
          "Median baseline");
  addItem(_menuBrowse, _miBrowseShortestBaseline, OnLoadShortestBaselinePressed,
          "Shortest baseline");
}

void RFIGuiMenu::makeSimulateMenu() {
  _miTestSetSubMenu.set_submenu(_menuTestSets);
  addItem(_menuSimulate, _miTestSetSubMenu, "Open _testset");

  // Gtk::RadioMenuItem::Group testSetGroup;
  addItem(_menuTestSets, _miTestGaussian, OnGaussianTestSets, "Gaussian");
  //_miTestGaussian.set_group(testSetGroup);
  _miTestGaussian.set_active(true);
  addItem(_menuTestSets, _miTestRayleigh, OnRayleighTestSets, "Rayleigh");
  //_miTestRayleigh.set_group(testSetGroup);
  addItem(_menuTestSets, _miTestZero, OnZeroTestSets, "Zero");
  //_miTestZero.set_group(testSetGroup);

  addItem(_menuTestSets, _miTestSep1);

  addItem(_menuTestSets, _miTestA, OnOpenTestSetA, "A Full spikes");
  addItem(_menuTestSets, _miTestB, OnOpenTestSetB, "B Half spikes");
  addItem(_menuTestSets, _miTestC, OnOpenTestSetC, "C Varying spikes");
  addItem(_menuTestSets, _miTestD, OnOpenTestSetD, "D 3 srcs + spikes");
  addItem(_menuTestSets, _miTestE, OnOpenTestSetE, "E 5 srcs + spikes");
  addItem(_menuTestSets, _miTestF, OnOpenTestSetF, "F 5 srcs + spikes");
  addItem(_menuTestSets, _miTestG, OnOpenTestSetG, "G Test set G");
  addItem(_menuTestSets, _miTestH, OnOpenTestSetG, "H filtered srcs + spikes");

  addItem(_menuTestSets, _miTestNoise, OnOpenTestSetNoise, "Noise");
  addItem(_menuTestSets, _miTestModel3, OnOpenTestSet3Model, "3-source model");
  addItem(_menuTestSets, _miTestModel5, OnOpenTestSet5Model, "5-source model");
  addItem(_menuTestSets, _miTestNoiseModel3, OnOpenTestSetNoise3Model,
          "3-source model with noise");
  addItem(_menuTestSets, _miTestNoiseModel5, OnOpenTestSetNoise5Model,
          "5-source model with noise");
  addItem(_menuTestSets, _miTestBStrong, OnOpenTestSetBStrong,
          "Test set B (strong RFI)");
  addItem(_menuTestSets, _miTestBWeak, OnOpenTestSetBWeak,
          "Test set B (weak RFI)");
  addItem(_menuTestSets, _miTestBAligned, OnOpenTestSetBAligned,
          "Test set B (aligned)");

  addItem(_menuTestSets, _miTestGaussianBroadband,
          OnOpenTestSetGaussianBroadband, "Gaussian broadband");
  addItem(_menuTestSets, _miTestSenusoidalBroadband,
          OnOpenTestSetSinusoidalBroadband, "Sinusoidal broadband");
  addItem(_menuTestSets, _miTestSlewedGaussianBroadband,
          OnOpenTestSetSlewedGaussianBroadband, "Slewed Gaussian");
  addItem(_menuTestSets, _miTestBurstBroadband, OnOpenTestSetBurstBroadband,
          "Burst");
  addItem(_menuTestSets, _miTestRFIDistLow, OnOpenTestSetRFIDistributionLow,
          "Slope -2 dist low");
  addItem(_menuTestSets, _miTestRFIDistMid, OnOpenTestSetRFIDistributionMid,
          "Slope -2 dist mid");
  addItem(_menuTestSets, _miTestRFIDistHigh, OnOpenTestSetRFIDistributionHigh,
          "Slope -2 dist high");
  addItem(_menuTestSets, _miTestSpike, OnOpenTestSetSpike, "Spike (debug)");

  _miSimulateModify.set_submenu(_menuModify);
  addItem(_menuSimulate, _miSimulateModify, "Modify");

  addItem(_menuModify, _miModifyStaticFringe, OnAddStaticFringe,
          "Static fringe");
  addItem(_menuModify, _miModify1SigmaStaticFringe, OnAdd1SigmaFringe,
          "Static 1 sigma fringe");
  addItem(_menuModify, _miModifyToOne, OnSetToOne, "Set to 1");
  addItem(_menuModify, _miModifyToI, OnSetToI, "Set to i");
  addItem(_menuModify, _miModifyToOnePlusI, OnSetToOnePlusI, "Set to 1+i");
  addItem(_menuModify, _miModifyCorrelatorFault, OnAddCorrelatorFault,
          "Add correlator fault");
  addItem(_menuModify, _miModifyAddNaNs, OnAddNaNs, "Add NaNs");
  addItem(_menuModify, _miModifyMultiply, OnMultiplyData, "Multiply data...");

  // Gtk::RadioMenuItem::Group setGroup;
  //_miSimNCP.set_group(setGroup);
  addItem(_menuSimulate, _miSimNCP, "Use NCP set");
  //_miSimB1834.set_group(setGroup);
  addItem(_menuSimulate, _miSimB1834, "Use B1834 set");
  //_miSimEmpty.set_group(setGroup);
  addItem(_menuSimulate, _miSimEmpty, "Use empty set");
  _miSimNCP.set_active(true);

  addItem(_menuSimulate, _miSimSep1);

  // Gtk::RadioMenuItem::Group chGroup;
  //_miSim16channels.set_group(chGroup);
  addItem(_menuSimulate, _miSim16channels, "16 channels");
  //_miSim64channels.set_group(chGroup);
  addItem(_menuSimulate, _miSim64channels, "64 channels");
  //_miSim256channels.set_group(chGroup);
  addItem(_menuSimulate, _miSim256channels, "256 channels");
  _miSim64channels.set_active(true);

  addItem(_menuSimulate, _miSimSep2);

  addItem(_menuSimulate, _miSimFixBandwidth, "Fix bandwidth");
  _miSimFixBandwidth.set_active(false);

  addItem(_menuSimulate, _miSimCorrelation, OnSimulateCorrelation,
          "Simulate correlation");
  addItem(_menuSimulate, _miSimSourceSetA, OnSimulateSourceSetA,
          "Simulate source set A");
  addItem(_menuSimulate, _miSimSourceSetB, OnSimulateSourceSetB,
          "Simulate source set B");
  addItem(_menuSimulate, _miSimSourceSetC, OnSimulateSourceSetC,
          "Simulate source set C");
  addItem(_menuSimulate, _miSimSourceSetD, OnSimulateSourceSetD,
          "Simulate source set D");
  addItem(_menuSimulate, _miSimSourceOffAxis, OnSimulateOffAxisSource,
          "Simulate off-axis source");
  addItem(_menuSimulate, _miSimSourceOnAxis, OnSimulateOnAxisSource,
          "Simulate on-axis source");
}

void RFIGuiMenu::makeDataMenu() {
  makeSegmentationMenu();

  addItem(_menuData, _miDataToOriginal, OnVisualizedToOriginalPressed,
          "Select current view");
  addItem(_menuData, _miDataSep1);

  _miSelectComplex.set_submenu(_menuSelectComplex);
  addItem(_menuData, _miSelectComplex, "Select _complex");

  addItem(_menuSelectComplex, _miDataReal, OnKeepRealPressed, "_Real part");
  addItem(_menuSelectComplex, _miDataImaginary, OnKeepImaginaryPressed,
          "_Imaginary part");
  addItem(_menuSelectComplex, _miDataPhase, OnKeepPhasePressed, "_Phase part");
  addItem(_menuSelectComplex, _miDataUnrollPhase, OnUnrollPhaseButtonPressed,
          "_Unroll phase");

  _miSelectStokes.set_submenu(_menuSelectStokes);
  addItem(_menuData, _miSelectStokes, "Select _stokes");

  addItem(_menuSelectStokes, _miDataStokesI, OnKeepStokesIPressed, "Stokes _I");
  addItem(_menuSelectStokes, _miDataStokesQ, OnKeepStokesQPressed, "Stokes _Q");
  addItem(_menuSelectStokes, _miDataStokesU, OnKeepStokesUPressed, "Stokes _U");
  addItem(_menuSelectStokes, _miDataStokesV, OnKeepStokesVPressed, "Stokes _V");

  _miSelectCircular.set_submenu(_menuSelectCircular);
  addItem(_menuData, _miSelectCircular, "Select circ_ular");

  addItem(_menuSelectCircular, _miDataRR, OnKeepRRPressed, "_RR");
  addItem(_menuSelectCircular, _miDataRL, OnKeepRLPressed, "RL");
  addItem(_menuSelectCircular, _miDataLR, OnKeepLRPressed, "LR");
  addItem(_menuSelectCircular, _miDataLL, OnKeepLLPressed, "_LL");

  _miSelectLinear.set_submenu(_menuSelectLinear);
  addItem(_menuData, _miSelectLinear, "Select _linear");

  addItem(_menuSelectLinear, _miDataXX, OnKeepXXPressed, "_XX");
  addItem(_menuSelectLinear, _miDataXY, OnKeepXYPressed, "XY");
  addItem(_menuSelectLinear, _miDataYX, OnKeepYXPressed, "YX");
  addItem(_menuSelectLinear, _miDataYY, OnKeepYYPressed, "_YY");

  _miSegmentationSubMenu.set_submenu(_menuSegmentation);
  addItem(_menuData, _miSegmentationSubMenu, "_Segmentation");

  addItem(_menuData, _miDataSep2);

  addItem(_menuData, _miDataStore, OnStoreData, "Store");
  addItem(_menuData, _miDataRecall, OnRecallData, "Recall");
  addItem(_menuData, _miDataSubtract, OnSubtractDataFromMem,
          "Subtract from mem");

  addItem(_menuData, _miDataSep3);

  addItem(_menuData, _miDataClearOriginalFlags, OnClearOriginalFlagsPressed,
          "Clear ori. flags");
  addItem(_menuData, _miDataClearAltFlags, OnClearAltFlagsPressed,
          "Clear alt. flags");
}

void RFIGuiMenu::makeSegmentationMenu() {
  addItem(_menuSegmentation, _miSegmSegment, OnSegment, "Segment");
  addItem(_menuSegmentation, _miSegmCluster, OnCluster, "Cluster");
  addItem(_menuSegmentation, _miSegmClassify, OnClassify, "Classify");
  addItem(_menuSegmentation, _miSegmRemoveSmallSegments, OnRemoveSmallSegments,
          "Remove small segments");
}

void RFIGuiMenu::makeToolbarActions() {
  addTool(_tbOpen, OnOpen, "Open", "Open a file or directory on disk",
          "document-open");

  _toolbar.append(_tbSep1);

  addTool(
      _tbModeData, [&]() { onTBModeData(); }, "Data",
      "Switch to data-viewing mode.", "spectrum");
  addTool(
      _tbModeStrategy, [&]() { onTBModeStrategy(); }, "Strategy",
      "Switch to the strategy editor.", "lua-editor");

  _toolbar.append(_tbSep2);

  // Gtk::AccelKey("F9")
  addTool(_tbExecuteStrategy, OnExecuteLuaStrategy, "E_xecute strategy",
          "Run the Lua strategy script. This will not write to the opened set. "
          "The flagging results are displayed in the plot as yellow "
          "('alternative') flag mask.",
          "system-run");
  // F3
  addTool(_tbOriginalFlags, OnToggleFlags, "Ori flags",
          "Display the first flag mask on top of the visibilities. These flags "
          "are displayed in purple and indicate the flags as they originally "
          "were stored in the measurement set.",
          "showoriginalflags");
  // F4
  addTool(_tbAlternativeFlags, OnToggleFlags, "Alt flags",
          "Display the second flag mask on top of the visibilities. These "
          "flags are displayed in yellow and indicate flags found by running "
          "the strategy.",
          "showalternativeflags");

  addTool(_tbSelectVisualization, OnToggleImage, "Change visualization",
          "Switch visualization", "showoriginalvisibilities");
  _tbSelectVisualization.set_menu(_tfVisualizationMenu);

  _toolbar.append(_tbSep3);

  // F6
  addTool(_tbPrevious, OnLoadPrevious, "Previous",
          "Load and display the previous baseline. Normally, this steps from "
          "the baseline between antennas (i) and (j) to (i) and (j-1).",
          "go-previous");
  _tbPrevious.set_sensitive(false);
  // F5
  addTool(_tbReload, OnReloadPressed, "Reload",
          "Reload the currently displayed baseline. This will reset the purple "
          "flags to the measurement set flags, and clear the yellow flags.",
          "view-refresh");
  _tbReload.set_sensitive(false);
  // F7
  addTool(_tbNext, OnLoadNext, "Next",
          "Load and display the next baseline. Normally, this steps from the "
          "baseline between antennas (i) and (j) to (i) and (j+1).",
          "go-next");
  _tbNext.set_sensitive(false);

  _toolbar.append(_tbSep4);

  // <control>0
  addTool(_tbZoomFit, OnZoomFit, "Fit", "Zoom fit", "zoom-fit-best");
  addTool(_tbZoomIn, OnZoomIn, "+", "Zoom in", "zoom-in");
  addTool(_tbZoomOut, OnZoomOut, "-", "Zoom out", "zoom-out");

  _toolbar.append(_tbSep5);

  auto sig = [&]() {
    if (!_blockVisualizationSignals) OnTogglePolarizations();
  };
  addTool(_tbDisplayPP, sig, "PP",
          "Display the PP polarization. Depending on the polarization "
          "configuration of the measurement set, this will show XX or RR",
          "showpp");
  addTool(_tbDisplayPQ, sig, "PQ",
          "Display the PQ polarization. Depending on the polarization "
          "configuration of the measurement set, this will show XY or RL",
          "showpq");
  addTool(_tbDisplayQP, sig, "QP",
          "Display the QP polarization. Depending on the polarization "
          "configuration of the measurement set, this will show YX or LR",
          "showqp");
  addTool(_tbDisplayQQ, sig, "QQ",
          "Display the QQ polarization. Depending on the polarization "
          "configuration of the measurement set, this will show YY or LL",
          "showqq");
  _tbDisplayPP.set_active(true);
  _tbDisplayQQ.set_active(true);
}

void RFIGuiMenu::EnableRunButtons(bool sensitive) {
  _menuFile.set_sensitive(sensitive);
  _miExecuteLuaStrategy.item.set_sensitive(sensitive);
  _miExecutePythonStrategy.set_sensitive(sensitive);
  _tbOpen.set_sensitive(sensitive);
  _tbExecuteStrategy.set_sensitive(sensitive);
}

void RFIGuiMenu::SetRecentFiles(const std::vector<std::string>& recentFiles) {
  _miRecentFileChoices.clear();
  size_t n = std::min(recentFiles.size(), size_t(10));
  _miRecentFileChoices.resize(n);
  for (size_t i = 0; i != n; ++i) {
    addItem(
        _menuRecentFiles, _miRecentFileChoices[i],
        [&, i]() { OnOpenRecent(i); }, recentFiles[i].c_str());
    _miRecentFileChoices[i].set_use_underline(false);
  }
  _menuRecentFiles.show_all_children();
}

void RFIGuiMenu::SetStrategyDefaults(
    const std::vector<std::string>& strategyDefaults) {
  _miStrategyDefaults.resize(strategyDefaults.size());
  for (size_t i = 0; i != strategyDefaults.size(); ++i) {
    std::string label = strategyDefaults[i];
    addItem(
        _menuStrategyOpenDefault, _miStrategyDefaults[i],
        [&, label]() { OnStrategyOpenDefault(label); }, label.c_str());
    _miStrategyDefaults[i].set_use_underline(false);
  }
  _menuStrategyOpenDefault.show_all_children();
}

void RFIGuiMenu::onTBModeData() {
  if (_inStrategyMode && _tbModeData.get_active()) {
    _inStrategyMode = false;
    _miViewData.set_active(true);
    OnViewData();
  }
}

void RFIGuiMenu::onTBModeStrategy() {
  if (!_inStrategyMode && _tbModeStrategy.get_active()) {
    _inStrategyMode = true;
    _miViewStrategy.set_active(true);
    OnViewStrategy();
  }
}

void RFIGuiMenu::onMIViewData() {
  if (_inStrategyMode && _miViewData.get_active()) {
    _inStrategyMode = false;
    _tbModeData.set_active(true);
    OnViewData();
  }
}

void RFIGuiMenu::onMIViewStrategy() {
  if (!_inStrategyMode && _miViewStrategy.get_active()) {
    _inStrategyMode = true;
    _tbModeStrategy.set_active(true);
    OnViewStrategy();
  }
}
