#include "rfiguicontroller.h"

#include "../../algorithms/svdmitigater.h"
#include "../../algorithms/testsetgenerator.h"

#include "../../python/pythonstrategy.h"

#include "../../imagesets/h5imageset.h"
#include "../../imagesets/imageset.h"
#include "../../imagesets/joinedspwset.h"
#include "../../imagesets/msimageset.h"
#include "../../imagesets/msoptions.h"
#include "../../imagesets/multibandmsimageset.h"

#include "../../lua/luastrategy.h"
#include "../../lua/scriptdata.h"
#include "../../lua/telescopefile.h"

#include "../../msio/singlebaselinefile.h"

#include "../../plot/plotmanager.h"

#include "../../quality/histogramcollection.h"

#include "../../structures/spatialmatrixmetadata.h"

#include "../../util/multiplot.h"
#include "../../util/progress/progresslistener.h"
#include "../../util/process.h"
#include "../../util/rfiplots.h"

#include "../rfiguiwindow.h"

#include "../../external/npy.hpp"

#include "imagecomparisoncontroller.h"

#include <aocommon/system.h>

#include <gtkmm/messagedialog.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <system_error>
#include <thread>

using algorithms::SVDMitigater;
using algorithms::TestSetGenerator;

RFIGuiController::RFIGuiController()
    : _showOriginalFlags(true),
      _showAlternativeFlags(true),
      _showPP(true),
      _showPQ(false),
      _showQP(false),
      _showQQ(true),
      _rfiGuiWindow(nullptr),
      _tfController() {
  _plotManager.reset(new class PlotManager());

  _settings.Load();
  NewDefaultStrategy();
}

RFIGuiController::~RFIGuiController() {
  try {
    _settings.Save();
  } catch (std::exception& e) {
    Logger::Error << "Failed to write config file: " << e.what() << '\n';
  }
}

bool RFIGuiController::IsImageLoaded() const {
  return _tfController.Plot().HasImage();
}

TimeFrequencyData RFIGuiController::ActiveData() const {
  return _tfController.GetActiveData();
}

TimeFrequencyData RFIGuiController::OriginalData() const {
  return _tfController.OriginalData();
}

TimeFrequencyMetaDataCPtr RFIGuiController::SelectedMetaData() const {
  return _tfController.Plot().GetSelectedMetaData();
}

void RFIGuiController::plotMeanSpectrum(bool weight) {
  if (IsImageLoaded()) {
    const std::string title = weight ? "Sum spectrum" : "Mean spectrum";
    XYPlot& plot = _plotManager->NewPlot2D(title);

    const TimeFrequencyData data = ActiveData();
    Mask2DCPtr mask =
        Mask2D::CreateSetMaskPtr<false>(data.ImageWidth(), data.ImageHeight());
    XYPointSet& beforeSet = plot.StartLine("Without flagging");
    if (weight)
      RFIPlots::MakeMeanSpectrumPlot<true>(beforeSet, data, mask,
                                           SelectedMetaData());
    else
      RFIPlots::MakeMeanSpectrumPlot<false>(beforeSet, data, mask,
                                            SelectedMetaData());

    mask.reset(new Mask2D(*data.GetSingleMask()));
    if (!mask->AllFalse()) {
      XYPointSet& afterSet = plot.StartLine("Flagged");
      if (weight)
        RFIPlots::MakeMeanSpectrumPlot<true>(afterSet, data, mask,
                                             SelectedMetaData());
      else
        RFIPlots::MakeMeanSpectrumPlot<false>(afterSet, data, mask,
                                              SelectedMetaData());
    }

    _plotManager->Update();
  }
}

void RFIGuiController::PlotDist() {
  if (IsImageLoaded()) {
    XYPlot& plot = _plotManager->NewPlot2D("Distribution");

    const TimeFrequencyData activeData = ActiveData();
    const Image2DCPtr image = activeData.GetSingleImage();
    Mask2DPtr mask =
        Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
    XYPointSet& totalSet = plot.StartLine("Total");
    RFIPlots::MakeDistPlot(totalSet, image, mask);

    XYPointSet& uncontaminatedSet = plot.StartLine("Uncontaminated");
    mask.reset(new Mask2D(*activeData.GetSingleMask()));
    RFIPlots::MakeDistPlot(uncontaminatedSet, image, mask);

    mask->Invert();
    XYPointSet& rfiSet = plot.StartLine("RFI");
    RFIPlots::MakeDistPlot(rfiSet, image, mask);

    _plotManager->Update();
  }
}

void RFIGuiController::PlotLogLogDist() {
  if (IsImageLoaded()) {
    const TimeFrequencyData activeData = ActiveData();
    HistogramCollection histograms(activeData.PolarizationCount());
    for (unsigned p = 0; p != activeData.PolarizationCount(); ++p) {
      const TimeFrequencyData polData(activeData.MakeFromPolarizationIndex(p));
      const Image2DCPtr image = polData.GetSingleImage();
      const Mask2DCPtr mask = Mask2D::MakePtr(*polData.GetSingleMask());
      histograms.Add(0, 1, p, image, mask);
    }
    _rfiGuiWindow->ShowHistogram(histograms);
  }
}

void RFIGuiController::PlotPowerSpectrum() {
  if (IsImageLoaded()) {
    XYPlot& plot = _plotManager->NewPlot2D("Power spectrum");
    plot.YAxis().SetLogarithmic(true);

    const TimeFrequencyData data = ActiveData();
    std::array<Image2DCPtr, 2> images;
    if (data.ComplexRepresentation() == TimeFrequencyData::ComplexParts) {
      images = data.GetSingleComplexImage();
    } else {
      images[0] = data.GetSingleImage();
      images[1] = images[0];
    }
    Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(images[0]->Width(),
                                                     images[0]->Height());
    XYPointSet& beforeSet = plot.StartLine("Flags not applied");
    RFIPlots::MakePowerSpectrumPlot(beforeSet, *images[0], *images[1], *mask,
                                    SelectedMetaData().get());

    mask = Mask2D::MakePtr(*data.GetSingleMask());
    if (!mask->AllFalse()) {
      XYPointSet& afterSet = plot.StartLine("Flags applied");
      RFIPlots::MakePowerSpectrumPlot(afterSet, *images[0], *images[1], *mask,
                                      SelectedMetaData().get());
    }

    _plotManager->Update();
  }
}

void RFIGuiController::PlotFrequencyScatter() {
  if (IsImageLoaded()) {
    MultiPlot plot(_plotManager->NewPlot2D("Frequency scatter"), 4);
    RFIPlots::MakeFrequencyScatterPlot(plot, ActiveData(), SelectedMetaData());
    plot.Finish();
    _plotManager->Update();
  }
}

void RFIGuiController::DrawTimeMean(XYPlot& plot) {
  plot.SetTitle("Mean over time");
  plot.YAxis().SetLogarithmic(true);
  plot.XAxis().SetType(AxisType::kTime);

  const TimeFrequencyData activeData = ActiveData();
  const Image2DCPtr image = activeData.GetSingleImage();
  Mask2DPtr mask =
      Mask2D::CreateSetMaskPtr<false>(image->Width(), image->Height());
  XYPointSet& totalPlot = plot.StartLine("Without flagging");
  RFIPlots::MakePowerTimePlot(totalPlot, image, mask, SelectedMetaData());

  mask = Mask2D::MakePtr(*activeData.GetSingleMask());
  if (!mask->AllFalse()) {
    XYPointSet& uncontaminatedPlot = plot.StartLine("With flagging");
    RFIPlots::MakePowerTimePlot(uncontaminatedPlot, image, mask,
                                SelectedMetaData());
  }
}

void RFIGuiController::PlotTimeMean() {
  if (IsImageLoaded()) {
    XYPlot& plot = _plotManager->NewPlot2D("Mean over time");
    DrawTimeMean(plot);
    _plotManager->Update();
  }
}

void RFIGuiController::PlotTimeScatter() {
  if (IsImageLoaded()) {
    MultiPlot plot(_plotManager->NewPlot2D("Time scatter"), 4);
    RFIPlots::MakeTimeScatterPlot(plot, ActiveData(), SelectedMetaData());
    plot.Finish();
    _plotManager->Update();
  }
}

void RFIGuiController::PlotSingularValues() {
  if (IsImageLoaded()) {
    XYPlot& plot = _plotManager->NewPlot2D("Singular values");

    SVDMitigater::CreateSingularValueGraph(ActiveData(), plot);
    _plotManager->Update();
  }
}

void RFIGuiController::open(std::unique_ptr<imagesets::ImageSet> imageSet) {
  std::vector<std::string> filenames = imageSet->Files();
  if (filenames.size() == 1)
    Logger::Info << "Opened " << filenames[0] << '\n';
  else
    Logger::Info << "Opening multiple files.\n";

  SetImageSet(std::move(imageSet));

  if (filenames.size() == 1) setRecentFile(filenames[0]);
}

void RFIGuiController::Open(const std::vector<std::string>& filenames) {
  std::unique_lock<std::mutex> lock(_ioMutex);
  MSOptions options;
  options.ioMode = DirectReadMode;
  options.baselineIntegration.enable = false;
  std::unique_ptr<imagesets::ImageSet> imageSet(
      imagesets::ImageSet::Create(filenames, options));
  imageSet->Initialize();
  lock.unlock();

  open(std::move(imageSet));
}

void RFIGuiController::OpenMsConcatenated(
    const std::vector<std::string>& filenames, const MSOptions& options) {
  constexpr size_t kMaxIoThreads = 16;
  const size_t n_io_threads = std::min<size_t>(
      {kMaxIoThreads, aocommon::system::ProcessorCount(), filenames.size()});

  std::unique_ptr<imagesets::ImageSet> image_set =
      std::make_unique<imagesets::MultiBandMsImageSet>(
          filenames, options.ioMode, options.intervalStart,
          options.intervalStart, n_io_threads);

  image_set->Initialize();

  open(std::move(image_set));
}

void RFIGuiController::OpenMS(const std::vector<std::string>& filenames,
                              const MSOptions& options) {
  if (options.concatenateFrequency) {
    OpenMsConcatenated(filenames, options);
    return;
  }

  std::unique_ptr<imagesets::ImageSet> imageSet(
      imagesets::ImageSet::Create(filenames, options));

  if (imagesets::H5ImageSet* h5ImageSet =
          dynamic_cast<imagesets::H5ImageSet*>(imageSet.get());
      h5ImageSet != nullptr) {
    h5ImageSet->SetInterval(options.intervalStart, options.intervalEnd);
  } else if (imagesets::MSImageSet* msImageSet =
                 dynamic_cast<imagesets::MSImageSet*>(imageSet.get());
             msImageSet != nullptr) {
    msImageSet->SetDataColumnName(options.dataColumnName);
    msImageSet->SetInterval(options.intervalStart, options.intervalEnd);

    msImageSet->SetReadUVW(true);

    if (options.combineSPWs) {
      msImageSet->Initialize();
      imageSet.release();
      std::unique_ptr<imagesets::MSImageSet> msImageSetPtr(msImageSet);
      imageSet.reset(new imagesets::JoinedSPWSet(std::move(msImageSetPtr)));
    }
  }
  imageSet->Initialize();

  open(std::move(imageSet));
}

void RFIGuiController::SaveBaselineFlags() {
  _imageSet->AddWriteFlagsTask(_imageSetIndex,
                               _tfController.GetActiveDataFullSize());
  _imageSet->PerformWriteFlagsTask();
}

void RFIGuiController::setRecentFile(const std::string& filename) {
  std::string absFilename = std::filesystem::absolute(filename).string();
  std::vector<std::string> files = _settings.RecentFiles();
  files.emplace(files.begin(), absFilename);
  if (files.size() > 10) files.resize(10);
  for (size_t i = 1; i != files.size(); ++i) {
    std::error_code ec;
    if (files[i] == absFilename ||
        std::filesystem::equivalent(files[i], absFilename, ec)) {
      files.erase(files.begin() + i);
      break;
    }
  }
  _settings.SetRecentFiles(files);
  _signalRecentFilesChanged.emit();
}

std::vector<std::string> RFIGuiController::RecentFiles() const {
  std::vector<std::string> files = _settings.RecentFiles();
  std::vector<std::string> shownFiles;
  shownFiles.reserve(files.size());
  for (size_t i = 0; i != files.size(); ++i) {
    if (!files[i].empty()) {
      // Don't return the current open file as a recent file
      std::error_code ec;
      const bool isOpen =
          (HasImageSet() &&
           (files[i] == _imageSet->Files()[0] ||
            std::filesystem::equivalent(files[i], _imageSet->Files()[0], ec)));
      if (!isOpen) shownFiles.emplace_back(files[i]);
    }
  }
  return shownFiles;
}

void RFIGuiController::OpenTestSet(
    algorithms::RFITestSet rfiSet,
    algorithms::BackgroundTestSet backgroundSet) {
  size_t width = 1024, height = 512;
  TimeFrequencyMetaDataCPtr metaData;
  if (IsImageLoaded()) {
    const TimeFrequencyData activeData = ActiveData();
    width = activeData.ImageWidth();
    height = activeData.ImageHeight();
    metaData = SelectedMetaData();
  } else {
    metaData.reset(new TimeFrequencyMetaData());
  }
  CloseImageSet();

  const TimeFrequencyData data =
      TestSetGenerator::MakeTestSet(rfiSet, backgroundSet, width, height);
  _tfController.SetNewData(data, metaData);
  const char* name = "Simulated test set";
  _tfController.Plot().SetTitleText(name);

  if (_rfiGuiWindow != nullptr) {
    _rfiGuiWindow->SetBaselineInfo(true, false, name, name);
  }
}

void RFIGuiController::ExecutePythonStrategy() {
  PythonStrategy pythonStrategy;
  _tfController.ClearAllButOriginal();
  TimeFrequencyData data = OriginalData();

  ScriptData scriptData;
  scriptData.SetCanVisualize(true);
  pythonStrategy.Execute(data, SelectedMetaData(), scriptData);
  scriptData.SortVisualizations();
  for (size_t i = 0; i != scriptData.VisualizationCount(); ++i) {
    auto v = scriptData.GetVisualization(i);
    _tfController.AddVisualization(std::get<0>(v), std::get<1>(v));
  }

  _tfController.AddVisualization("Script result", data);
  _rfiGuiWindow->GetTimeFrequencyWidget().Update();
}

void RFIGuiController::ExecuteLuaStrategy(class ProgressListener& listener) {
  _tfController.ClearAllButOriginal();
  _processingThread = std::thread([&]() {
    try {
      _scriptTFData = OriginalData();
      const TimeFrequencyMetaDataCPtr metaData =
          _tfController.Plot().GetFullMetaData();

      _scriptData.reset(new ScriptData());
      _scriptData->SetCanVisualize(true);
      _scriptData->SetProgressListener(listener);
      LuaStrategy luaStrategy;
      luaStrategy.Initialize();
      luaStrategy.LoadFile(_settings.GetStrategyFilename().c_str());
      luaStrategy.Execute(_scriptTFData, metaData, *_scriptData, "execute");
      listener.OnFinish();
    } catch (std::exception& e) {
      listener.OnException(e);
    }
  });
}

void RFIGuiController::JoinLuaThread() {
  _processingThread.join();
  _scriptData->SortVisualizations();
  for (size_t i = 0; i != _scriptData->VisualizationCount(); ++i) {
    auto v = _scriptData->GetVisualization(i);
    _tfController.AddVisualization(std::get<0>(v), std::get<1>(v));
  }
  _scriptData.reset();

  _tfController.AddVisualization("Script result", std::move(_scriptTFData));
  _rfiGuiWindow->GetTimeFrequencyWidget().Update();
  _scriptTFData = TimeFrequencyData();
}

void RFIGuiController::SetImageSet(
    std::unique_ptr<imagesets::ImageSet> newImageSet) {
  _imageSetIndex = newImageSet->StartIndex();
  _imageSet = std::move(newImageSet);
}

void RFIGuiController::SetImageSetIndex(
    const imagesets::ImageSetIndex& newImageSetIndex) {
  _imageSetIndex = newImageSetIndex;
}

void RFIGuiController::CloseImageSet() {
  _imageSet.reset();
  _tfController.Clear();
  if (_rfiGuiWindow != nullptr)
    _rfiGuiWindow->SetBaselineInfo(true, false, "", "No data loaded");
  // Closing a file causes the recent files to change, since
  // the closed file now becomes a recent file.
  _signalRecentFilesChanged.emit();
}

void RFIGuiController::LoadCurrentTFDataAsync(ProgressListener& progress) {
  try {
    if (HasImageSet()) {
      const std::lock_guard<std::mutex> lock(_ioMutex);
      _imageSet->AddReadRequest(_imageSetIndex);
      _imageSet->PerformReadRequests(progress);
      _tempLoadedBaseline = _imageSet->GetNextRequested();
    }
  } catch (std::exception& exception) {
    _tempLoadedBaseline.reset();
    progress.OnException(exception);
  }
}

void RFIGuiController::LoadCurrentTFDataFinish(bool success) {
  if (success) {
    _tfController.SetNewData(std::move(_tempLoadedBaseline->Data()),
                             _tempLoadedBaseline->MetaData());
    _tempLoadedBaseline.reset();

    // We store these seperate, as they might access the measurement set. This
    // is not only faster (the names are used in the onMouse.. events) but also
    // less dangerous, since the set can be simultaneously accessed by another
    // thread. (thus the io mutex should be locked before calling below
    // statements).
    std::string name, description;
    std::unique_lock<std::mutex> lock(_ioMutex);
    name = GetImageSet().Name();
    description = GetImageSet().Description(GetImageSetIndex());
    lock.unlock();

    _tfController.Plot().SetTitleText(description);

    if (_rfiGuiWindow != nullptr) {
      // Disable forward/back buttons when only one baseline is available
      imagesets::ImageSetIndex firstIndex = _imageSet->StartIndex();
      firstIndex.Next();
      const bool multipleBaselines = !firstIndex.HasWrapped();

      _rfiGuiWindow->SetBaselineInfo(false, multipleBaselines, name,
                                     description);
    }
  } else {
    CloseImageSet();
  }
}

void RFIGuiController::CheckPolarizations(bool forceSignal) {
  const bool pp = _showPP, pq = _showPQ, qp = _showQP, qq = _showQQ;
  if (!pp && !pq && !qp && !qq) {
    _showPP = true;
    _showQQ = true;
  }
  _tfController.TryVisualizePolarizations(_showPP, _showPQ, _showQP, _showQQ);
  if (forceSignal || _showPP != pp || _showPQ != pq || _showQP != qp ||
      _showQQ != qq)
    _signalStateChange();
}

void RFIGuiController::GetAvailablePolarizations(bool& pp, bool& pq, bool& qp,
                                                 bool& qq) const {
  bool b[4];
  for (size_t i = 0; i != 4; ++i) {
    // Set b to false except for b[i]
    for (size_t j = 0; j != 4; ++j) b[j] = (j == i);
    _tfController.TryVisualizePolarizations(b[0], b[1], b[2], b[3]);
    switch (i) {
      case 0:
        pp = b[0];
        break;
      case 1:
        pq = b[1];
        break;
      case 2:
        qp = b[2];
        break;
      case 3:
        qq = b[3];
        break;
    }
  }
}

void RFIGuiController::SaveBaselineAsRfibl(const std::string& filename) {
  SingleBaselineFile file;
  file.data = _tfController.OriginalData();
  file.metaData = *_tfController.Plot().GetFullMetaData();
  file.telescopeName = _imageSet->TelescopeName();
  std::ofstream stream(filename);
  file.Write(stream);
}

void RFIGuiController::SaveBaselineAsNpy(const std::string& filename) {
  const TimeFrequencyData& tf_data = _tfController.OriginalData();
  std::vector<std::complex<num_t>> data = ToComplexVector(tf_data);
  const std::array<unsigned long, 3> shape = {
      tf_data.PolarizationCount(), tf_data.ImageHeight(), tf_data.ImageWidth()};
  npy::SaveArrayAsNumpy(filename, false, shape.size(), shape.data(), data);
}

void RFIGuiController::NewDefaultStrategy() {
  _settings.InitializeWorkStrategy();
  _openStrategyFilename.clear();
}

void RFIGuiController::NewEmptyStrategy() {
  const std::ofstream file(_settings.GetStrategyFilename());
  if (!file)
    throw std::runtime_error("Error writing new strategy to work file");
  _openStrategyFilename.clear();
}

void RFIGuiController::NewTemplateStrategy() {
  std::ofstream file(_settings.GetStrategyFilename());
  file << LuaStrategy::GetTemplateScript();
  if (!file)
    throw std::runtime_error("Error writing new strategy to work file");
  _openStrategyFilename.clear();
}

void RFIGuiController::OpenStrategy(const std::string& filename) {
  std::error_code ec;
  if (std::filesystem::equivalent(_settings.GetStrategyFilename(), filename,
                                  ec))
    throw std::runtime_error("Can't open working Lua strategy");
  std::filesystem::copy_file(filename, _settings.GetStrategyFilename(),
                             std::filesystem::copy_options::overwrite_existing);
  _openStrategyFilename = filename;
}

void RFIGuiController::SaveStrategy() {
  std::filesystem::copy_file(_settings.GetStrategyFilename(),
                             _openStrategyFilename,
                             std::filesystem::copy_options::overwrite_existing);
}

void RFIGuiController::SaveStrategyAs(const std::string& filename) {
  std::filesystem::copy_file(_settings.GetStrategyFilename(), filename,
                             std::filesystem::copy_options::overwrite_existing);
  _openStrategyFilename = filename;
}

std::string RFIGuiController::GetWorkStrategyText() const {
  const std::ifstream file(_settings.GetStrategyFilename());
  if (!file) throw std::runtime_error("Error reading work strategy from file");
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

void RFIGuiController::SetWorkStrategyText(const std::string& text) {
  std::ofstream file(_settings.GetStrategyFilename());
  if (!file) throw std::runtime_error("Error writing work strategy to file");
  file << text;
}
