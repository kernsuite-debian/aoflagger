#include "rfiguiwindow.h"

#include <gtkmm/aboutdialog.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/messagedialog.h>

#include <filesystem>

#include "../msio/baselinematrixloader.h"

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"
#include "../structures/segmentedimage.h"

#include "../lua/telescopefile.h"

#include "../imagesets/msimageset.h"

#include "../algorithms/baselineselector.h"
#include "../algorithms/morphology.h"
#include "../algorithms/fringetestcreater.h"
#include "../algorithms/polarizationstatistics.h"
#include "../algorithms/resampling.h"
#include "../algorithms/thresholdtools.h"
#include "../algorithms/timefrequencystatistics.h"

#include "controllers/imagecomparisoncontroller.h"
#include "controllers/rfiguicontroller.h"

#include "gotowindow.h"
#include "histogramwindow.h"
#include "imagepropertieswindow.h"
#include "numinputdialog.h"
#include "opendialog.h"
#include "plotwindow.h"
#include "progresswindow.h"
#include "rfiguimenu.h"
#include "simulatedialog.h"

#include "../imaging/model.h"
#include "../imaging/observatorium.h"

#include "../quality/histogramcollection.h"

#include <version.h>

#include <iostream>

using algorithms::FringeTestCreater;
using algorithms::Morphology;
using algorithms::ThresholdConfig;
using algorithms::ThresholdTools;
using algorithms::TimeFrequencyStatistics;

RFIGuiWindow::RFIGuiWindow(RFIGuiController* controller,
                           const std::string& argv0)
    : _controller(controller),
      _mainVBox(Gtk::ORIENTATION_VERTICAL),
      _timeFrequencyWidget(_controller->TFController().Plot()),
      _plotWindow(new PlotWindow(_controller->PlotManager())),
      _menu() {
  _controller->AttachWindow(this);

  const Glib::RefPtr<Gtk::IconTheme> iconTheme = Gtk::IconTheme::get_default();

  const std::filesystem::path argv0Root =
      std::filesystem::path(argv0).remove_filename();
  if (!std::filesystem::equivalent(
          argv0Root / "..", std::filesystem::path(AOFLAGGER_INSTALL_PATH))) {
    const std::filesystem::path iconPathC =
        std::filesystem::path(AOFLAGGER_INSTALL_PATH) / "share/icons";
    iconTheme->prepend_search_path(iconPathC.string());
  }
  const std::filesystem::path iconPathA = argv0Root / "../share/icons";
  iconTheme->prepend_search_path(iconPathA.string());
  const std::filesystem::path iconPathB = argv0Root / "../data/icons";
  iconTheme->prepend_search_path(iconPathB.string());

  // Now that the icon search path is correct, the menu can be created
  _menu.reset(new RFIGuiMenu());

  set_default_size(800, 600);
  set_default_icon_name("aoflagger");

  _menu->SetOriginalFlagsActive(_controller->AreOriginalFlagsShown());
  _menu->SetAlternativeFlagsActive(_controller->AreAlternativeFlagsShown());

  // File
  _menu->OnOpen.connect([&]() { onOpen(); });
  _menu->OnSaveBaselineFlags.connect([&]() { onSaveBaselineFlags(); });
  _menu->OnClose.connect([&]() { onClose(); });
  _menu->OnExportBaseline.connect([&]() { onExportBaseline(); });
  _menu->OnQuit.connect([&]() { onQuit(); });
  _menu->OnAbout.connect([&]() { onHelpAbout(); });
  _menu->OnOpenRecent.connect([&](size_t index) { onOpenRecent(index); });

  // View
  _menu->OnViewData.connect([&]() { onViewData(); });
  _menu->OnViewStrategy.connect([&]() { onViewStrategy(); });
  _menu->OnViewTimePlot.connect([&]() { onViewTimePlot(); });
  _menu->OnImagePropertiesPressed.connect(
      [&]() { onImagePropertiesPressed(); });
  _menu->OnToggleFlags.connect([&]() { onToggleFlags(); });
  _menu->OnZoomFit.connect([&]() { onZoomFit(); });
  _menu->OnZoomIn.connect([&]() { onZoomIn(); });
  _menu->OnZoomOut.connect([&]() { onZoomOut(); });
  _menu->OnZoomSelect.connect([&]() { onZoomSelect(); });
  _menu->OnShowStats.connect([&]() { onShowStats(); });

  // Strategy
  _menu->OnExecuteLuaStrategy.connect([&]() { onExecuteLuaStrategy(); });
  _menu->OnExecutePythonStrategy.connect([&]() { onExecutePythonStrategy(); });

  _menu->OnStrategyNewEmpty.connect([&]() { onStrategyNewEmpty(); });
  _menu->OnStrategyNewTemplate.connect([&]() { onStrategyNewTemplate(); });
  _menu->OnStrategyNewDefault.connect([&]() { onStrategyNewDefault(); });
  _menu->OnStrategyOpen.connect([&]() { onStrategyOpen(); });
  _menu->OnStrategyOpenDefault.connect(
      [&](const std::string& name) { onStrategyOpenDefault(name); });
  _menu->OnStrategySave.connect([&]() { onStrategySave(); });
  _menu->OnStrategySaveAs.connect([&]() { onStrategySaveAs(); });

  // Plot
  _menu->OnPlotDistPressed.connect([&]() { onPlotDistPressed(); });
  _menu->OnPlotLogLogDistPressed.connect([&]() { onPlotLogLogDistPressed(); });
  _menu->OnPlotMeanSpectrumPressed.connect(
      [&]() { onPlotMeanSpectrumPressed(); });
  _menu->OnPlotSumSpectrumPressed.connect(
      [&]() { onPlotSumSpectrumPressed(); });
  _menu->OnPlotPowerSpectrumPressed.connect(
      [&]() { onPlotPowerSpectrumPressed(); });
  _menu->OnPlotFrequencyScatterPressed.connect(
      [&]() { onPlotFrequencyScatterPressed(); });
  _menu->OnPlotTimeMeanPressed.connect([&]() { onPlotTimeMeanPressed(); });
  _menu->OnPlotTimeScatterPressed.connect(
      [&]() { onPlotTimeScatterPressed(); });
  _menu->OnPlotSingularValuesPressed.connect(
      [&]() { onPlotSingularValuesPressed(); });

  // Browse
  _menu->OnLoadPrevious.connect([&]() { onLoadPrevious(); });
  _menu->OnReloadPressed.connect([&]() { onReloadPressed(); });
  _menu->OnLoadNext.connect([&]() { onLoadNext(); });
  _menu->OnGoToPressed.connect([&]() { onGoToPressed(); });
  _menu->OnLoadLongestBaselinePressed.connect(
      [&]() { onLoadExtremeBaseline(true); });
  _menu->OnLoadMedianBaselinePressed.connect([&]() { onLoadMedianBaseline(); });
  _menu->OnLoadShortestBaselinePressed.connect(
      [&]() { onLoadExtremeBaseline(false); });

  // Simulate
  _menu->OnSimulate.connect([&]() { onSimulate(); });
  _menu->OnOpenTestSetA.connect([&]() { onOpenTestSetA(); });
  _menu->OnOpenTestSetB.connect([&]() { onOpenTestSetB(); });
  _menu->OnOpenTestSetC.connect([&]() { onOpenTestSetC(); });
  _menu->OnOpenTestSetD.connect([&]() { onOpenTestSetD(); });
  _menu->OnOpenTestSetE.connect([&]() { onOpenTestSetE(); });
  _menu->OnOpenTestSetF.connect([&]() { onOpenTestSetF(); });
  _menu->OnOpenTestSetG.connect([&]() { onOpenTestSetG(); });
  _menu->OnOpenTestSetH.connect([&]() { onOpenTestSetH(); });
  _menu->OnOpenTestSetNoise.connect([&]() { onOpenTestSetNoise(); });

  _menu->OnAddStaticFringe.connect([&]() { onAddStaticFringe(); });
  _menu->OnAdd1SigmaFringe.connect([&]() { onAdd1SigmaFringe(); });
  _menu->OnSetToOne.connect([&]() { onSetToOne(); });
  _menu->OnSetToI.connect([&]() { onSetToI(); });
  _menu->OnSetToOnePlusI.connect([&]() { onSetToOnePlusI(); });
  _menu->OnAddCorrelatorFault.connect([&]() { onAddCorrelatorFault(); });
  _menu->OnAddNaNs.connect([&]() { onAddNaNs(); });
  _menu->OnMultiplyData.connect([&]() { onMultiplyData(); });

  // Data
  _menu->OnVisualizedToOriginalPressed.connect(
      [&]() { onVisualizedToOriginalPressed(); });
  _menu->OnKeepRealPressed.connect([&]() { onKeepRealPressed(); });
  _menu->OnKeepImaginaryPressed.connect([&]() { onKeepImaginaryPressed(); });
  _menu->OnKeepPhasePressed.connect([&]() { onKeepPhasePressed(); });
  _menu->OnUnrollPhaseButtonPressed.connect(
      [&]() { onUnrollPhaseButtonPressed(); });

  _menu->OnKeepStokesIPressed.connect([&]() { onKeepStokesIPressed(); });
  _menu->OnKeepStokesQPressed.connect([&]() { onKeepStokesQPressed(); });
  _menu->OnKeepStokesUPressed.connect([&]() { onKeepStokesUPressed(); });
  _menu->OnKeepStokesVPressed.connect([&]() { onKeepStokesVPressed(); });
  _menu->OnKeepRRPressed.connect([&]() { onKeepRRPressed(); });
  _menu->OnKeepRLPressed.connect([&]() { onKeepRLPressed(); });
  _menu->OnKeepLRPressed.connect([&]() { onKeepLRPressed(); });
  _menu->OnKeepLLPressed.connect([&]() { onKeepLLPressed(); });
  _menu->OnKeepXXPressed.connect([&]() { onKeepXXPressed(); });
  _menu->OnKeepXYPressed.connect([&]() { onKeepXYPressed(); });
  _menu->OnKeepYXPressed.connect([&]() { onKeepYXPressed(); });
  _menu->OnKeepYYPressed.connect([&]() { onKeepYYPressed(); });

  _menu->OnStoreData.connect([&]() { onStoreData(); });
  _menu->OnRecallData.connect([&]() { onRecallData(); });
  _menu->OnSubtractDataFromMem.connect([&]() { onSubtractDataFromMem(); });
  _menu->OnClearOriginalFlagsPressed.connect(
      [&]() { onClearOriginalFlagsPressed(); });
  _menu->OnClearAltFlagsPressed.connect([&]() { onClearAltFlagsPressed(); });

  _menu->OnTemporalAveragingPressed.connect([&]() { handleAveraging(false); });
  _menu->OnSpectralAveragingPressed.connect([&]() { handleAveraging(true); });

  // Actions
  _menu->OnSegment.connect([&]() { onSegment(); });
  _menu->OnCluster.connect([&]() { onCluster(); });
  _menu->OnClassify.connect([&]() { onClassify(); });
  _menu->OnRemoveSmallSegments.connect([&]() { onRemoveSmallSegments(); });

  // Toolbar signals (some are already covered)
  _menu->OnTogglePolarizations.connect([&]() { onTogglePolarizations(); });
  _menu->OnToggleImage.connect([&]() { onToggleImage(); });
  _menu->OnSelectImage.connect([&]() { onSelectImage(); });

  _mainVBox.pack_start(_menu->Menu(), Gtk::PACK_SHRINK);
  _mainVBox.pack_start(_menu->Toolbar(), Gtk::PACK_SHRINK);

  _mainVBox.pack_start(_timeFrequencyWidget, Gtk::PACK_EXPAND_WIDGET);
  _timeFrequencyWidget.GetHeatMapWidget().OnMouseMovedEvent().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::onTFWidgetMouseMoved));
  _timeFrequencyWidget.GetHeatMapWidget().OnMouseLeaveEvent().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::setSetNameInStatusBar));
  _timeFrequencyWidget.GetHeatMapWidget().OnScrollEvent().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::onTFScroll));
  _timeFrequencyWidget.GetHeatMapWidget().Plot().OnZoomChanged().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::onTFZoomChanged));
  MaskedHeatMap& map = _timeFrequencyWidget.GetMaskedHeatMap();
  map.SetShowXAxisDescription(false);
  map.SetXAxisType(AxisType::kTime);
  map.SetShowYAxisDescription(false);
  map.SetShowZAxisDescription(false);

  _strategyEditor.SetText(_controller->GetWorkStrategyText());
  _strategyEditor.ResetChangedStatus();
  _mainVBox.pack_start(_strategyEditor, Gtk::PACK_EXPAND_WIDGET);

  _controller->TFController().VisualizationListChange().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::updateTFVisualizationMenu));

  _mainVBox.pack_end(_statusbar, Gtk::PACK_SHRINK);
  _statusbar.push("Ready. For suggestions, contact offringa@gmail.com .");

  add(_mainVBox);
  _mainVBox.show_all();
  _strategyEditor.hide();
  _timeFrequencyWidget.DisableTimePlot();

  onTFZoomChanged();

  _controller->SignalStateChange().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::onControllerStateChange));

  _controller->SignalRecentFilesChanged().connect(
      sigc::mem_fun(*this, &RFIGuiWindow::updateRecentFiles));

  updateTFVisualizationMenu();
  updateRecentFiles();
  updateOpenStrategyMenu();

  signal_delete_event().connect(
      [&](GdkEventAny* event) { return onClose(event); });
}

RFIGuiWindow::~RFIGuiWindow() {}

bool RFIGuiWindow::onClose(GdkEventAny* event) { return !askToSaveChanges(); }

bool RFIGuiWindow::askToSaveChanges() {
  if (_strategyEditor.IsChanged()) {
    Gtk::MessageDialog dialog(
        "The strategy was changed without saving. Do you want to save your "
        "changes?",
        false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
    dialog.add_button("Don't save", Gtk::RESPONSE_CLOSE);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_YES);
    const int result = dialog.run();
    if (result == Gtk::RESPONSE_CANCEL) {
      return false;
    } else if (result == Gtk::RESPONSE_CLOSE) {
      return true;
    } else {
      onStrategySave();
      // In case the strategy had no name yet, the onStrategySave() call will
      // show a file dialog where the user might press cancel. In that case,
      // the operation is cancelled:
      return !_strategyEditor.IsChanged();
    }
  } else {
    return true;
  }
}

void RFIGuiWindow::onOpen() {
  OpenDialog openDialog(*this);

  const int result = openDialog.run();

  if (result == Gtk::RESPONSE_OK) {
    OpenMS(openDialog.Selection(), openDialog.GetOptions());
  }
}

void RFIGuiWindow::onOpenRecent(size_t index) {
  OpenPaths(std::vector<std::string>{_controller->RecentFiles()[index]});
}

void RFIGuiWindow::updateRecentFiles() {
  const std::vector<std::string> files = _controller->RecentFiles();
  std::vector<std::string> leafs;
  leafs.reserve(files.size());
  for (const std::string& f : files) {
    std::error_code err_code;
    std::filesystem::path path =
        std::filesystem::canonical(std::filesystem::path(f), err_code);
    if (!err_code) leafs.emplace_back(path.filename().string());
  }
  _menu->SetRecentFiles(leafs);
}

void RFIGuiWindow::updateOpenStrategyMenu() {
  const std::vector<TelescopeFile::TelescopeId> telescopeIds =
      TelescopeFile::List();
  std::vector<std::string> strategies;
  for (const TelescopeFile::TelescopeId id : telescopeIds) {
    strategies.emplace_back(TelescopeFile::TelescopeName(id));
  }
  _menu->SetStrategyDefaults(strategies);
}

void RFIGuiWindow::onClose() { _controller->CloseImageSet(); }

void RFIGuiWindow::onExportBaseline() {
  Gtk::FileChooserDialog dialog("Select baseline file",
                                Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog.set_transient_for(*this);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Save", Gtk::RESPONSE_ACCEPT);

  auto filter_rfibl = Gtk::FileFilter::create();
  filter_rfibl->set_name("Baseline files (*.rfibl)");
  filter_rfibl->add_mime_type("application/rfibl");
  filter_rfibl->add_pattern("*.rfibl");
  dialog.add_filter(filter_rfibl);

  auto filter_npy = Gtk::FileFilter::create();
  filter_npy->set_name("Numpy array (*.npy)");
  filter_npy->add_mime_type("application/npy");
  filter_npy->add_pattern("*.npy");
  dialog.add_filter(filter_npy);

  dialog.set_do_overwrite_confirmation(true);
  const int result = dialog.run();

  if (result == Gtk::RESPONSE_ACCEPT) {
    if (dialog.get_filter() == filter_rfibl)
      _controller->SaveBaselineAsRfibl(dialog.get_filename());
    else
      _controller->SaveBaselineAsNpy(dialog.get_filename());
  }
}

void RFIGuiWindow::OpenPaths(const std::vector<std::string>& paths) {
  if (paths.size() > 1 || imagesets::ImageSet::IsMSFile(paths.front())) {
    OpenDialog openDialog(*this);
    openDialog.SetSelection(paths);
    openDialog.ActivateOptionsTab();
    const int result = openDialog.run();

    if (result == Gtk::RESPONSE_OK) {
      OpenMS(openDialog.Selection(), openDialog.GetOptions());
    }
  } else {
    _controller->Open(paths);

    if (dynamic_cast<imagesets::IndexableSet*>(&_controller->GetImageSet()) !=
        nullptr)
      OpenGotoWindow();
    else
      loadWithProgress();
  }
}

void RFIGuiWindow::OpenMS(const std::vector<std::string>& filenames,
                          const MSOptions& options) {
  try {
    _controller->OpenMS(filenames, options);

    if (dynamic_cast<imagesets::IndexableSet*>(&_controller->GetImageSet()) !=
        nullptr)
      OpenGotoWindow();
    else
      loadWithProgress();
  } catch (std::exception& e) {
    Gtk::MessageDialog dialog(*this, e.what(), false, Gtk::MESSAGE_ERROR);
    dialog.run();
  }
}

void RFIGuiWindow::onSaveBaselineFlags() {
  Gtk::MessageDialog dialog(*this,
                            "Are you sure you want to change the currently "
                            "opened dataset and write the flags as displayed?",
                            false, Gtk::MESSAGE_QUESTION,
                            Gtk::BUTTONS_OK_CANCEL, true);
  if (dialog.run() == Gtk::RESPONSE_OK) _controller->SaveBaselineFlags();
}

TimeFrequencyData RFIGuiWindow::GetActiveData() const {
  return _controller->TFController().GetActiveData();
}
const TimeFrequencyData& RFIGuiWindow::GetOriginalData() const {
  return _controller->TFController().OriginalData();
}
ThresholdConfig& RFIGuiWindow::HighlightConfig() {
  return _controller->TFController().Plot().HighlightConfig();
}
void RFIGuiWindow::SetHighlighting(bool newValue) {
  _controller->TFController().Plot().SetHighlighting(newValue);
}
TimeFrequencyMetaDataCPtr RFIGuiWindow::SelectedMetaData() {
  return _controller->TFController().Plot().GetSelectedMetaData();
}

void RFIGuiWindow::onToggleFlags() {
  _controller->SetShowOriginalFlags(_menu->OriginalFlagsActive());
  _controller->SetShowAlternativeFlags(_menu->AlternativeFlagsActive());
}

void RFIGuiWindow::onTogglePolarizations() {
  _controller->SetShowPP(_menu->ShowPPActive());
  _controller->SetShowPQ(_menu->ShowPQActive());
  _controller->SetShowQP(_menu->ShowQPActive());
  _controller->SetShowQQ(_menu->ShowQQActive());
}

void RFIGuiWindow::setSetNameInStatusBar() {
  if (_controller->HasImageSet()) {
    _statusbar.pop();
    _statusbar.push(_imageSetName + ": " + _imageSetIndexDescription);
  }
}

void RFIGuiWindow::SetImageSetIndex(
    const imagesets::ImageSetIndex& newImageSetIndex) {
  _controller->SetImageSetIndex(newImageSetIndex);
  loadWithProgress();
}

void RFIGuiWindow::loadWithProgress() {
  _menu->ActivateDataMode();
  set_sensitive(false);
  _progressWindow.reset(new ProgressWindow());
  _progressWindow->SignalFinished().connect([&](bool success) {
    _taskThread.join();
    _progressWindow.reset();
    set_sensitive(true);
    _controller->LoadCurrentTFDataFinish(success);
  });
  _progressWindow->present();
  _taskThread = std::thread(
      [&]() { _controller->LoadCurrentTFDataAsync(*_progressWindow); });
}

void RFIGuiWindow::onLoadPrevious() {
  if (_controller->HasImageSet()) {
    std::unique_lock<std::mutex> lock(_controller->IOMutex());
    imagesets::ImageSetIndex index = _controller->GetImageSetIndex();
    index.Previous();
    _controller->SetImageSetIndex(index);
    lock.unlock();
    loadWithProgress();
  }
}

void RFIGuiWindow::onLoadNext() {
  if (_controller->HasImageSet()) {
    std::unique_lock<std::mutex> lock(_controller->IOMutex());
    imagesets::ImageSetIndex index = _controller->GetImageSetIndex();
    index.Next();
    _controller->SetImageSetIndex(index);
    lock.unlock();
    loadWithProgress();
  }
}

void RFIGuiWindow::onViewData() {
  _timeFrequencyWidget.show();
  _strategyEditor.hide();
}

void RFIGuiWindow::onViewStrategy() {
  _timeFrequencyWidget.hide();
  _strategyEditor.show();
}

void RFIGuiWindow::onViewTimePlot() {
  const bool viewTime = _menu->ViewTimePlot();
  if (viewTime) {
    _timeFrequencyWidget.EnableTimePlot();
    _controller->DrawTimeMean(_timeFrequencyWidget.TimePlot());
  } else {
    _timeFrequencyWidget.DisableTimePlot();
  }
}

void RFIGuiWindow::onExecuteLuaStrategy() {
  _controller->SetWorkStrategyText(_strategyEditor.GetText());

  _progressWindow.reset(new ProgressWindow());
  _progressWindow->SignalFinished().connect(
      [&](bool successfull) { onExecuteStrategyFinished(successfull); });
  _progressWindow->SignalError().connect(
      [&](const std::string& err) { onExecuteStrategyError(err); });
  _progressWindow->present();
  _menu->EnableRunButtons(false);
  _controller->ExecuteLuaStrategy(*_progressWindow);
}

void RFIGuiWindow::onExecuteStrategyFinished(bool successfull) {
  _controller->JoinLuaThread();
  if (successfull) _menu->ActivateDataMode();
  _menu->EnableRunButtons(true);
  _progressWindow.reset();
}

void RFIGuiWindow::onExecuteStrategyError(const std::string& error) {
  _menu->ActivateStrategyMode();
  const size_t colon = error.find(':');
  if (colon != error.npos) {
    const size_t lineNr = std::atoi(error.substr(colon + 1).c_str());
    _strategyEditor.HighlightLine(lineNr);
  }
  Gtk::MessageDialog dialog(*this, error, false, Gtk::MESSAGE_ERROR);
  dialog.run();
}

void RFIGuiWindow::openTestSet(algorithms::RFITestSet rfiSet,
                               algorithms::BackgroundTestSet backgroundSet) {
  _controller->OpenTestSet(rfiSet, backgroundSet);
}

void RFIGuiWindow::onClearOriginalFlagsPressed() {
  TimeFrequencyData data = _controller->TFController().GetVisualizationData(0);
  data.SetMasksToValue<false>();
  _controller->TFController().SetVisualizationData(0, std::move(data));
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onClearAltFlagsPressed() {
  TimeFrequencyData data(_controller->TFController().AltMaskData());
  data.SetMasksToValue<false>();
  _controller->TFController().SetAltMaskData(data);
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onVisualizedToOriginalPressed() {
  if (HasImage()) {
    const TimeFrequencyData data(_controller->TFController().GetActiveData());
    _controller->TFController().SetNewData(
        std::move(data),
        _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
  }
}

void RFIGuiWindow::onAddStaticFringe() {
  try {
    if (HasImage()) {
      const TimeFrequencyMetaDataCPtr metaData = SelectedMetaData();
      TimeFrequencyData data(GetActiveData());
      FringeTestCreater::AddStaticFringe(data, metaData, 1.0L);
      _controller->TFController().SetNewData(data, metaData);
      _timeFrequencyWidget.Update();
    }
  } catch (std::exception& e) {
    showError(e.what());
  }
}

void RFIGuiWindow::onAdd1SigmaFringe() {
  try {
    if (HasImage()) {
      const TimeFrequencyMetaDataCPtr metaData = SelectedMetaData();
      num_t mean, stddev;
      TimeFrequencyData data(GetActiveData());
      ThresholdTools::MeanAndStdDev(data.GetRealPart().get(),
                                    data.GetSingleMask().get(), mean, stddev);
      FringeTestCreater::AddStaticFringe(data, metaData, stddev);
      _controller->TFController().SetNewData(
          data, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
      _timeFrequencyWidget.Update();
    }
  } catch (std::exception& e) {
    showError(e.what());
  }
}

void RFIGuiWindow::onSetToOne() {
  try {
    const TimeFrequencyData data(GetActiveData());
    std::array<Image2DCPtr, 2> images = data.GetSingleComplexImage();
    Image2DPtr real = Image2D::MakePtr(*images[0]),
               imaginary = Image2D::MakePtr(*images[1]);
    real->SetAll(1.0);
    imaginary->SetAll(0.0);
    TimeFrequencyData newData(data.GetPolarization(0), real, imaginary);
    newData.SetMask(data);
    _controller->TFController().SetNewData(
        newData, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
    _timeFrequencyWidget.Update();
  } catch (std::exception& e) {
    showError(e.what());
  }
}

void RFIGuiWindow::onSetToI() {
  try {
    const TimeFrequencyData data(GetActiveData());
    std::array<Image2DCPtr, 2> images = data.GetSingleComplexImage();
    Image2DPtr real = Image2D::MakePtr(*images[0]),
               imaginary = Image2D::MakePtr(*images[0]);
    real->SetAll(0.0);
    imaginary->SetAll(1.0);
    TimeFrequencyData newData(data.GetPolarization(0), real, imaginary);
    newData.SetMask(data);
    _controller->TFController().SetNewData(
        newData, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
    _timeFrequencyWidget.Update();
  } catch (std::exception& e) {
    showError(e.what());
  }
}

void RFIGuiWindow::onSetToOnePlusI() {
  try {
    const TimeFrequencyData data(GetActiveData());
    std::array<Image2DCPtr, 2> images = data.GetSingleComplexImage();
    Image2DPtr real = Image2D::MakePtr(*images[0]),
               imaginary = Image2D::MakePtr(*images[0]);
    real->SetAll(1.0);
    imaginary->SetAll(1.0);
    TimeFrequencyData newData(data.GetPolarization(0), real, imaginary);
    newData.SetMask(data);
    _controller->TFController().SetNewData(
        newData, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
    _timeFrequencyWidget.Update();
  } catch (std::exception& e) {
    showError(e.what());
  }
}

void RFIGuiWindow::onAddCorrelatorFault() {
  Gtk::MessageDialog dialog(*this, "Enter affected timerange (ratios):", false,
                            Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL,
                            true);
  Gtk::Entry startTime;
  Gtk::Box hBox;
  startTime.set_text("0.25");
  hBox.pack_start(startTime);
  Gtk::Label dashLabel("-");
  hBox.pack_start(dashLabel);
  Gtk::Entry endTime;
  endTime.set_text("0.5");
  hBox.pack_start(endTime);
  Gtk::Box* box = dialog.get_content_area();
  box->pack_start(hBox);
  box->show_all_children();
  if (dialog.run() == Gtk::RESPONSE_OK) {
    TimeFrequencyData data(GetActiveData());
    const double startRatio = std::atof(startTime.get_text().c_str());
    const double endRatio = std::atof(endTime.get_text().c_str());
    const size_t startIndex = data.ImageWidth() * startRatio;
    const size_t endIndex = data.ImageWidth() * endRatio;
    for (size_t i = 0; i != data.ImageCount(); ++i) {
      Image2DPtr image(new Image2D(*data.GetImage(i)));
      const num_t addValue = 10.0 * image->GetStdDev();
      for (size_t y = 0; y != image->Height(); ++y) {
        for (size_t x = startIndex; x != endIndex; ++x)
          image->AddValue(x, y, addValue);
      }

      data.SetImage(i, std::move(image));
    }
    for (size_t i = 0; i != data.MaskCount(); ++i) {
      Mask2DPtr mask(new Mask2D(*data.GetMask(i)));
      for (size_t y = 0; y != mask->Height(); ++y) {
        for (size_t x = startIndex; x != endIndex; ++x)
          mask->SetValue(x, y, true);
      }
      data.SetMask(i, std::move(mask));
    }
    _controller->TFController().SetNewData(
        data, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
    _timeFrequencyWidget.Update();
  }
}

void RFIGuiWindow::onAddNaNs() {
  TimeFrequencyData data(GetActiveData());
  const size_t start_time = data.ImageWidth() * 1 / 4;
  const size_t end_time = data.ImageWidth() * 2 / 4;
  const size_t start_channel = data.ImageHeight() * 1 / 4;
  const size_t end_channel = data.ImageHeight() * 2 / 4;
  for (size_t i = 0; i != data.ImageCount(); ++i) {
    Image2DPtr image(new Image2D(*data.GetImage(i)));
    for (size_t y = start_channel; y != end_channel; ++y) {
      for (size_t x = start_time; x != end_time; ++x)
        image->SetValue(x, y, std::numeric_limits<num_t>::quiet_NaN());
    }

    data.SetImage(i, std::move(image));
  }
  for (size_t i = 0; i != data.MaskCount(); ++i) {
    Mask2DPtr mask(new Mask2D(*data.GetMask(i)));
    for (size_t y = start_channel; y != end_channel; ++y) {
      for (size_t x = start_time; x != end_time; ++x)
        mask->SetValue(x, y, true);
    }
    data.SetMask(i, std::move(mask));
  }
  _controller->TFController().SetNewData(
      data, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onShowStats() {
  if (_timeFrequencyWidget.HasImage()) {
    const TimeFrequencyData activeData = GetActiveData();
    TimeFrequencyStatistics statistics(activeData);
    std::stringstream s;
    s << "Percentage flagged: "
      << TimeFrequencyStatistics::FormatRatio(statistics.GetFlaggedRatio())
      << "\n";

    Mask2DCPtr original = _controller->TFController().Plot().OriginalMask(),
               alternative =
                   _controller->TFController().Plot().AlternativeMask();
    Mask2DPtr intersect;
    if (original != nullptr && alternative != nullptr) {
      intersect = Mask2D::MakePtr(*original);
      intersect->Intersect(*alternative);

      const unsigned intCount = intersect->GetCount<true>();
      if (intCount != 0) {
        if (*original != *alternative) {
          s << "Overlap between original and alternative: "
            << TimeFrequencyStatistics::FormatRatio(
                   (double)intCount /
                   ((double)(original->Width() * original->Height())))
            << "\n"
            << "(relative to alternative flags: "
            << TimeFrequencyStatistics::FormatRatio(
                   (double)intCount / ((double)(alternative->GetCount<true>())))
            << ")\n";
        }
      }
    }

    const Image2DCPtr powerImg = activeData.GetSingleImage();
    const Mask2DCPtr mask = activeData.GetSingleMask();
    double power = 0.0;
    for (unsigned y = 0; y < powerImg->Height(); ++y) {
      for (unsigned x = 0; x < powerImg->Width(); ++x) {
        if (!mask->Value(x, y) && std::isfinite(powerImg->Value(x, y))) {
          power += powerImg->Value(x, y);
        }
      }
    }
    s << "Total unflagged power: " << power << "\n";
    Gtk::MessageDialog dialog(*this, s.str(), false, Gtk::MESSAGE_INFO);
    dialog.run();
  }
}

void RFIGuiWindow::onPlotDistPressed() { _controller->PlotDist(); }

void RFIGuiWindow::onPlotLogLogDistPressed() { _controller->PlotLogLogDist(); }

void RFIGuiWindow::onPlotPowerSpectrumPressed() {
  _controller->PlotPowerSpectrum();
}

void RFIGuiWindow::onPlotFrequencyScatterPressed() {
  _controller->PlotFrequencyScatter();
}

void RFIGuiWindow::onPlotTimeMeanPressed() { _controller->PlotTimeMean(); }

void RFIGuiWindow::onPlotTimeScatterPressed() {
  _controller->PlotTimeScatter();
}

void RFIGuiWindow::onPlotSingularValuesPressed() {
  _controller->PlotSingularValues();
}

void RFIGuiWindow::ShowHistogram(HistogramCollection& histogramCollection) {
  if (_histogramWindow == nullptr)
    _histogramWindow.reset(new HistogramWindow(histogramCollection));
  else
    _histogramWindow->SetStatistics(histogramCollection);
  _histogramWindow->show();
}

void RFIGuiWindow::onImagePropertiesPressed() {
  _imagePropertiesWindow.reset(
      new ImagePropertiesWindow(_timeFrequencyWidget.GetHeatMapWidget(),
                                "Time-frequency plotting options"));
  _imagePropertiesWindow->show();
}

void RFIGuiWindow::onPlotMeanSpectrumPressed() {
  _controller->PlotMeanSpectrum();
}

void RFIGuiWindow::onPlotSumSpectrumPressed() {
  _controller->PlotSumSpectrum();
}

void RFIGuiWindow::keepPhasePart(
    enum TimeFrequencyData::ComplexRepresentation phaseRepresentation) {
  if (HasImage()) {
    try {
      _controller->TFController().SetNewData(
          _controller->TFController().GetActiveData().Make(phaseRepresentation),
          _controller->TFController().Plot().GetSelectedMetaData());
      _timeFrequencyWidget.Update();
    } catch (std::exception& e) {
      std::stringstream errstr;
      errstr << "The data that was currently in memory could not be converted "
                "to the requested "
                "type. The error given by the converter was:\n"
             << e.what() << "\n\n"
             << "Note that if the original data should be convertable to this "
                "type, but "
                "you have already used one of the 'Keep ..' buttons, you first "
                "need to reload "
                "the full data with Goto -> Load.\n\n"
                "(alternatively, if loading takes a lot of time, you can use "
                "the Store and Recall"
                " options in the Data menu)";
      showError(errstr.str());
    }
  }
}

void RFIGuiWindow::updatePolarizations() {
  _controller->CheckPolarizations();
  bool pp, pq, qp, qq;
  _controller->GetAvailablePolarizations(pp, pq, qp, qq);
  _menu->SetShowPPSensitive(pp);
  _menu->SetShowPQSensitive(pq);
  _menu->SetShowQPSensitive(qp);
  _menu->SetShowQQSensitive(qq);
}

void RFIGuiWindow::keepPolarisation(aocommon::PolarizationEnum polarisation) {
  if (HasImage()) {
    try {
      _controller->TFController().SetNewData(
          _controller->TFController().GetActiveData().Make(polarisation),
          _controller->TFController().Plot().GetSelectedMetaData());
      updatePolarizations();
      _timeFrequencyWidget.Update();
    } catch (std::exception& e) {
      std::stringstream errstr;
      errstr << "The data that was currently in memory could not be converted "
                "to the requested "
                "polarization. The error given by the converter was:\n"
             << e.what() << "\n\n"
             << "Note that if the original data should be convertable to this "
                "polarization, but "
                "you have already used one of the 'Keep ..' buttons, you first "
                "need to reload "
                "the full data with Goto -> Load.\n\n"
                "(alternatively, if loading takes a lot of time, you can use "
                "the Store and Recall"
                " options in the Data menu)";
      showError(errstr.str());
    }
  }
}

void RFIGuiWindow::onGoToPressed() {
  if (_controller->HasImageSet()) {
    _menu->ActivateDataMode();
    imagesets::IndexableSet* msSet =
        dynamic_cast<imagesets::IndexableSet*>(&_controller->GetImageSet());
    if (msSet != nullptr) {
      _gotoWindow.reset(new GoToWindow(*this));
      _gotoWindow->present();
    } else {
      showError("Can not goto in this image set; format does not support goto");
    }
  }
}

void RFIGuiWindow::onReloadPressed() {
  if (_controller->HasImageSet()) {
    loadWithProgress();
  }
}

void RFIGuiWindow::onLoadExtremeBaseline(bool longest) {
  if (_controller->HasImageSet()) {
    imagesets::ImageSetIndex index = _controller->GetImageSetIndex();
    const bool available = imagesets::IndexableSet::FindExtremeBaseline(
        &_controller->GetImageSet(), index, longest);
    if (available) {
      _controller->SetImageSetIndex(index);
      loadWithProgress();
    }
  }
}

void RFIGuiWindow::onLoadMedianBaseline() {
  if (_controller->HasImageSet()) {
    imagesets::ImageSetIndex index = _controller->GetImageSetIndex();
    const bool available = imagesets::IndexableSet::FindMedianBaseline(
        &_controller->GetImageSet(), index);
    if (available) {
      _controller->SetImageSetIndex(index);
      loadWithProgress();
    }
  }
}

void RFIGuiWindow::onTFWidgetMouseMoved(double x, double y) {
  const MaskedHeatMap& heatMap = _timeFrequencyWidget.GetMaskedHeatMap();
  const Image2DCPtr image = heatMap.GetImage2D();
  size_t imageX;
  size_t imageY;
  if (heatMap.UnitToImage(x, y, imageX, imageY)) {
    const num_t v = image->Value(imageX, imageY);
    _statusbar.pop();
    std::stringstream s;
    s << "x=" << imageX << ",y=" << imageY << ",value=" << v;
    const TimeFrequencyMetaDataCPtr metaData =
        _timeFrequencyWidget.GetMaskedHeatMap().GetFullMetaData();
    if (metaData != nullptr) {
      if (metaData->HasObservationTimes() && metaData->HasBand()) {
        const std::vector<double>& times = metaData->ObservationTimes();
        s << " (t=" << Date::AipsMJDToString(times[imageX]) << ", f="
          << Frequency::ToString(metaData->Band().channels[imageY].frequencyHz);
      }

      if (metaData->HasUVW()) {
        const UVW uvw = metaData->UVW()[imageX];
        s << ", uvw=" << uvw.u << "," << uvw.v << "," << uvw.w;
      }
      s << ')';
    }
    _statusbar.push(s.str(), 0);
  }
}

void RFIGuiWindow::onMultiplyData() {
  TimeFrequencyData data(GetActiveData());
  data.MultiplyImages(2.0L);
  _controller->TFController().SetNewData(
      data, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSegment() {
  const SegmentedImagePtr segmentedImage = SegmentedImage::CreateUnsetPtr(
      GetOriginalData().ImageWidth(), GetOriginalData().ImageHeight());
  Morphology morphology;
  morphology.SegmentByLengthRatio(GetActiveData().GetSingleMask().get(),
                                  segmentedImage);
  _timeFrequencyWidget.GetMaskedHeatMap().SetSegmentedImage(segmentedImage);
  Update();
}

void RFIGuiWindow::onCluster() {
  const SegmentedImagePtr segmentedImage =
      _timeFrequencyWidget.GetMaskedHeatMap().GetSegmentedImage();
  if (segmentedImage) {
    Morphology morphology;
    morphology.Cluster(segmentedImage);
    _timeFrequencyWidget.GetMaskedHeatMap().SetSegmentedImage(segmentedImage);
    Update();
  }
}

void RFIGuiWindow::onClassify() {
  const SegmentedImagePtr segmentedImage =
      _timeFrequencyWidget.GetMaskedHeatMap().GetSegmentedImage();
  if (segmentedImage) {
    Morphology morphology;
    morphology.Classify(segmentedImage);
    _timeFrequencyWidget.GetMaskedHeatMap().SetSegmentedImage(segmentedImage);
    Update();
  }
}

void RFIGuiWindow::onRemoveSmallSegments() {
  const SegmentedImagePtr segmentedImage =
      _timeFrequencyWidget.GetMaskedHeatMap().GetSegmentedImage();
  if (segmentedImage) {
    Morphology morphology;
    morphology.RemoveSmallSegments(segmentedImage, 4);
    _timeFrequencyWidget.GetMaskedHeatMap().SetSegmentedImage(segmentedImage);
    Update();
  }
}

void RFIGuiWindow::onTFScroll(double x, double y, int direction) {
  if (direction < 0) {
    _timeFrequencyWidget.GetMaskedHeatMap().ZoomInOn(x, y);
    _timeFrequencyWidget.Update();
  } else if (direction > 0) {
    _timeFrequencyWidget.GetMaskedHeatMap().ZoomOut();
    _timeFrequencyWidget.Update();
  }
}

void RFIGuiWindow::onUnrollPhaseButtonPressed() {
  if (HasImage()) {
    TimeFrequencyData data = GetActiveData().Make(TimeFrequencyData::PhasePart);
    for (unsigned i = 0; i < data.ImageCount(); ++i) {
      const Image2DPtr image = Image2D::MakePtr(*data.GetImage(i));
      ThresholdTools::UnrollPhase(image.get());
      data.SetImage(i, image);
    }
    _controller->TFController().SetNewData(
        data, _timeFrequencyWidget.GetMaskedHeatMap().GetSelectedMetaData());
    _timeFrequencyWidget.Update();
  }
}

void RFIGuiWindow::showError(const std::string& description) {
  Gtk::MessageDialog dialog(*this, description, false, Gtk::MESSAGE_ERROR);
  dialog.run();
}

void RFIGuiWindow::onSimulate() {
  SimulateDialog simDialog;
  if (simDialog.run() == Gtk::RESPONSE_OK) {
    _controller->TFController().SetNewData(
        simDialog.Make(),
        TimeFrequencyMetaDataPtr(new TimeFrequencyMetaData()));
    const char* name = "Simulated test set";
    _controller->TFController().Plot().SetTitleText(name);
    SetBaselineInfo(true, false, name, name);
  }
}

void RFIGuiWindow::onStoreData() {
  if (HasImage()) {
    _storedData = _controller->TFController().GetActiveData();
    _storedMetaData = _controller->TFController().Plot().GetSelectedMetaData();
  }
}

void RFIGuiWindow::onRecallData() {
  _controller->TFController().SetNewData(_storedData, _storedMetaData);
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSubtractDataFromMem() {
  if (HasImage()) {
    const TimeFrequencyData diffData = TimeFrequencyData::MakeFromDiff(
        _storedData, _controller->TFController().GetActiveData());
    _controller->TFController().SetNewData(diffData, _storedMetaData);
    _timeFrequencyWidget.Update();
  }
}

void RFIGuiWindow::onControllerStateChange() {
  _menu->BlockVisualizationSignals();

  _menu->SetOriginalFlagsActive(_controller->AreOriginalFlagsShown());
  _timeFrequencyWidget.GetMaskedHeatMap().SetShowOriginalMask(
      _controller->AreOriginalFlagsShown());

  _menu->SetAlternativeFlagsActive(_controller->AreAlternativeFlagsShown());
  _timeFrequencyWidget.GetMaskedHeatMap().SetShowAlternativeMask(
      _controller->AreAlternativeFlagsShown());

  _menu->SetShowPPActive(_controller->IsPPShown());
  _menu->SetShowPQActive(_controller->IsPQShown());
  _menu->SetShowQPActive(_controller->IsQPShown());
  _menu->SetShowQQActive(_controller->IsQQShown());

  _controller->TFController().SetVisualizedPolarization(
      _controller->IsPPShown(), _controller->IsPQShown(),
      _controller->IsQPShown(), _controller->IsQQShown());

  _menu->UnblockVisualizationSignals();

  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomFit() {
  _timeFrequencyWidget.GetMaskedHeatMap().ZoomFit();
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomIn() {
  if (_timeFrequencyWidget.GetHeatMapWidget().IsMouseInImage())
    _timeFrequencyWidget.GetMaskedHeatMap().ZoomInOn(
        _timeFrequencyWidget.GetHeatMapWidget().MouseX(),
        _timeFrequencyWidget.GetHeatMapWidget().MouseY());
  else
    _timeFrequencyWidget.GetMaskedHeatMap().ZoomIn();
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomOut() {
  _timeFrequencyWidget.GetMaskedHeatMap().ZoomOut();
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onZoomSelect() {
  MaskedHeatMap& heatMap = _timeFrequencyWidget.GetMaskedHeatMap();
  if (!heatMap.HasImage()) return;
  const Image2DCPtr image = heatMap.GetImage2D();
  const size_t nTimes = image->Width();
  const size_t nChannels = image->Height();
  std::unique_ptr<NumInputDialog> dialog(
      new NumInputDialog("Select zoom region", "Start timestep: ", 0.0));
  if (dialog->run() != Gtk::RESPONSE_OK) return;
  const double startTimestep = dialog->Value();
  dialog.reset(
      new NumInputDialog("Select zoom region", "End timestep: ", nTimes));
  if (dialog->run() != Gtk::RESPONSE_OK) return;
  const double endTimestep = dialog->Value();
  dialog.reset(
      new NumInputDialog("Select zoom region", "Start channel: ", 0.0));
  if (dialog->run() != Gtk::RESPONSE_OK) return;
  const double startChannel = dialog->Value();
  dialog.reset(
      new NumInputDialog("Select zoom region", "End channel: ", nChannels));
  if (dialog->run() != Gtk::RESPONSE_OK) return;
  const double endChannel = dialog->Value();
  double x1, y1, x2, y2;
  heatMap.ImageToUnit(startTimestep, startChannel, x1, y1);
  heatMap.ImageToUnit(endTimestep, endChannel, x2, y2);
  heatMap.ZoomTo(x1, y1, x2, y2);
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onTFZoomChanged() {
  const bool s = !_timeFrequencyWidget.GetMaskedHeatMap().IsZoomedOut();
  const bool i = _timeFrequencyWidget.GetMaskedHeatMap().HasImage();
  _menu->SetZoomToFitSensitive(s && i);
  _menu->SetZoomOutSensitive(s && i);
  _menu->SetZoomInSensitive(i);
}

void RFIGuiWindow::onHelpAbout() {
  Gtk::AboutDialog aboutDialog;

  std::vector<Glib::ustring> authors;
  authors.push_back("André Offringa <offringa@gmail.com>");
  aboutDialog.set_authors(authors);

  const std::string release_year =
      std::string(AOFLAGGER_VERSION_DATE_STR).substr(0, 4);
  aboutDialog.set_copyright("Copyright 2008 - " + release_year +
                            " A. R. Offringa");
  aboutDialog.set_license_type(Gtk::LICENSE_GPL_3_0);
  aboutDialog.set_logo_icon_name("aoflagger");
  aboutDialog.set_program_name("AOFlagger's RFI Gui");
  aboutDialog.set_version("AOFlagger " AOFLAGGER_VERSION_STR
                          " (" AOFLAGGER_VERSION_DATE_STR ") ");
  aboutDialog.set_website("https://readthedocs.org/projects/aoflagger/");

  aboutDialog.run();
}

void RFIGuiWindow::onExecutePythonStrategy() {
  try {
    _controller->ExecutePythonStrategy();
  } catch (std::exception& e) {
    showError(e.what());
  }
}

void RFIGuiWindow::SetBaselineInfo(bool isEmpty, bool hasMultipleBaselines,
                                   const std::string& name,
                                   const std::string& description) {
  _menu->SetPreviousSensitive(!isEmpty && hasMultipleBaselines);
  _menu->SetReloadSensitive(!isEmpty);
  _menu->SetNextSensitive(!isEmpty && hasMultipleBaselines);
  _imageSetName = name;
  _imageSetIndexDescription = description;
  setSetNameInStatusBar();
  updatePolarizations();
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::onSelectImage() {
  const size_t index = getActiveTFVisualization();
  _controller->TFController().SetVisualization(index);
  _timeFrequencyWidget.Update();
}

void RFIGuiWindow::updateTFVisualizationMenu() {
  Gtk::Menu& menu = _menu->VisualizationMenu();
  const std::vector<Gtk::Widget*> children = menu.get_children();
  for (Gtk::Widget* child : children) menu.remove(*child);

  _tfVisualizationMenuItems.clear();
  Gtk::RadioButtonGroup group;
  for (size_t i = 0; i != _controller->TFController().VisualizationCount();
       ++i) {
    const std::string label =
        _controller->TFController().GetVisualizationLabel(i);
    _tfVisualizationMenuItems.emplace_back(std::unique_ptr<Gtk::RadioMenuItem>(
        new Gtk::RadioMenuItem(group, label)));
    Gtk::RadioMenuItem& item = *_tfVisualizationMenuItems.back();
    item.signal_activate().connect(
        sigc::mem_fun(*this, &RFIGuiWindow::onSelectImage));
    menu.add(item);
  }

  _menu->SetSelectVisualizationSensitive(_tfVisualizationMenuItems.size() > 1);

  _tfVisualizationMenuItems.front()->activate();
  menu.show_all_children();
}

void RFIGuiWindow::onToggleImage() {
  size_t index = getActiveTFVisualization();
  ++index;
  if (index == _tfVisualizationMenuItems.size()) index = 0;
  _tfVisualizationMenuItems[index]->set_active(true);
}

size_t RFIGuiWindow::getActiveTFVisualization() {
  for (size_t index = 0; index != _tfVisualizationMenuItems.size(); ++index) {
    if (_tfVisualizationMenuItems[index]->get_active()) return index;
  }
  return 0;
}

void RFIGuiWindow::onStrategyNewEmpty() {
  if (askToSaveChanges()) {
    _controller->NewEmptyStrategy();
    _strategyEditor.SetText("");
    _strategyEditor.ResetChangedStatus();
    _menu->ActivateStrategyMode();
  }
}

void RFIGuiWindow::onStrategyNewTemplate() {
  if (askToSaveChanges()) {
    _controller->NewTemplateStrategy();
    _strategyEditor.SetText(_controller->GetWorkStrategyText());
    _strategyEditor.ResetChangedStatus();
    _menu->ActivateStrategyMode();
  }
}

void RFIGuiWindow::onStrategyNewDefault() {
  if (askToSaveChanges()) {
    _controller->NewDefaultStrategy();
    _strategyEditor.SetText(_controller->GetWorkStrategyText());
    _strategyEditor.ResetChangedStatus();
    _menu->ActivateStrategyMode();
  }
}

void RFIGuiWindow::onStrategyOpen() {
  if (askToSaveChanges()) {
    Gtk::FileChooserDialog dialog("Select strategy to open");
    dialog.set_transient_for(*this);

    auto filter_text = Gtk::FileFilter::create();
    filter_text->set_name("Lua strategy files (*.lua)");
    filter_text->add_mime_type("text/x-lua");
    dialog.add_filter(filter_text);

    // Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    const int result = dialog.run();

    if (result == Gtk::RESPONSE_OK) {
      _controller->OpenStrategy(dialog.get_filename());
      _strategyEditor.SetText(_controller->GetWorkStrategyText());
      _strategyEditor.ResetChangedStatus();
      _menu->ActivateStrategyMode();
    }
  }
}

void RFIGuiWindow::onStrategyOpenDefault(const std::string& name) {
  if (askToSaveChanges()) {
    const TelescopeFile::TelescopeId id =
        TelescopeFile::TelescopeIdFromName(name);
    const std::string filename = TelescopeFile::FindStrategy(id);
    if (filename.empty()) {
      showError("Could not find default strategy file for telescope '" + name +
                "' -- aoflagger is probably not installed properly");
    } else {
      _controller->OpenStrategy(filename);
      _strategyEditor.SetText(_controller->GetWorkStrategyText());
      _strategyEditor.ResetChangedStatus();
      _menu->ActivateStrategyMode();
    }
  }
}

void RFIGuiWindow::onStrategySave() {
  if (_controller->HasOpenStrategy()) {
    _controller->SetWorkStrategyText(_strategyEditor.GetText());
    _controller->SaveStrategy();
    _strategyEditor.ResetChangedStatus();
  } else {
    onStrategySaveAs();
  }
}

void RFIGuiWindow::onStrategySaveAs() {
  Gtk::FileChooserDialog dialog("Select filename for strategy",
                                Gtk::FILE_CHOOSER_ACTION_SAVE);
  dialog.set_transient_for(*this);
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Save", Gtk::RESPONSE_ACCEPT);

  auto filter_text = Gtk::FileFilter::create();
  filter_text->set_name("Lua strategy files (*.lua)");
  filter_text->add_mime_type("text/x-lua");
  dialog.add_filter(filter_text);
  dialog.set_do_overwrite_confirmation(true);
  if (_controller->HasOpenStrategy())
    dialog.set_current_name(_controller->StrategyFilename());

  const int result = dialog.run();

  if (result == Gtk::RESPONSE_ACCEPT) {
    _controller->SetWorkStrategyText(_strategyEditor.GetText());
    _controller->SaveStrategyAs(dialog.get_filename());
    _strategyEditor.ResetChangedStatus();
  }
}

void RFIGuiWindow::handleAveraging(bool spectrally) {
  if (HasImage()) {
    Gtk::MessageDialog dialog("Enter averaging factor", false,
                              Gtk::MessageType::MESSAGE_QUESTION,
                              Gtk::ButtonsType::BUTTONS_OK_CANCEL);
    Gtk::Entry entry;
    entry.set_text("2");
    entry.set_max_width_chars(8);
    dialog.get_message_area()->pack_end(entry);
    entry.show();
    if (dialog.run() == Gtk::RESPONSE_OK) {
      const int factor = std::atoi(entry.get_text().c_str());
      if (factor > 1) {
        TimeFrequencyData data =
            _controller->TFController().GetActiveDataFullSize();
        const TimeFrequencyMetaDataPtr meta(new TimeFrequencyMetaData(
            *_timeFrequencyWidget.GetMaskedHeatMap().GetFullMetaData()));
        if (spectrally)
          algorithms::downsample_masked(data, meta.get(), 1, factor);
        else
          algorithms::downsample_masked(data, meta.get(), factor, 1);
        _controller->TFController().SetNewData(data, meta);
        _timeFrequencyWidget.Update();
      }
    }
  }
}
