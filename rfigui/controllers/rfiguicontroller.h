#ifndef RFIGUI_CONTROLLER_H
#define RFIGUI_CONTROLLER_H

#include <sigc++/signal.h>

#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"

#include "../../imagesets/imagesetindex.h"

#include "../settings.h"

#include "../../algorithms/enums.h"

#include "imagecomparisoncontroller.h"

#include <optional>
#include <mutex>
#include <thread>

namespace imagesets {
class BaselineData;
}

struct MSOptions;
class XYPlot;

class RFIGuiController {
 public:
  RFIGuiController();
  ~RFIGuiController();

  void AttachWindow(class RFIGuiWindow* rfiGuiWindow) {
    _rfiGuiWindow = rfiGuiWindow;
  }

  bool AreOriginalFlagsShown() const { return _showOriginalFlags; }
  void SetShowOriginalFlags(bool showFlags) {
    if (_showOriginalFlags != showFlags) {
      _showOriginalFlags = showFlags;
      _signalStateChange();
    }
  }

  bool AreAlternativeFlagsShown() const { return _showAlternativeFlags; }
  void SetShowAlternativeFlags(bool showFlags) {
    if (_showAlternativeFlags != showFlags) {
      _showAlternativeFlags = showFlags;
      _signalStateChange();
    }
  }

  bool IsPPShown() const { return _showPP; }
  bool IsPQShown() const { return _showPQ; }
  bool IsQPShown() const { return _showQP; }
  bool IsQQShown() const { return _showQQ; }
  void SetShowPP(bool showPP) {
    if (showPP != _showPP) {
      if (showPP || _showPQ || _showQP || _showQQ) {
        _showPP = showPP;
        if (_showPP) {
          _showPQ = false;
          _showQP = false;
        }
      }
      CheckPolarizations(true);
    }
  }
  void SetShowPQ(bool showPQ) {
    if (showPQ != _showPQ) {
      if (showPQ || _showPP || _showQP || _showQQ) {
        _showPQ = showPQ;
        if (_showPQ) {
          _showPP = false;
          _showQQ = false;
        }
      }
      CheckPolarizations(true);
    }
  }
  void SetShowQP(bool showQP) {
    if (showQP != _showQP) {
      if (showQP || _showPP || _showPQ || _showQQ) {
        _showQP = showQP;
        if (_showQP) {
          _showPP = false;
          _showQQ = false;
        }
      }
      CheckPolarizations(true);
    }
  }
  void SetShowQQ(bool showQQ) {
    if (showQQ != _showQQ) {
      if (showQQ || _showPP || _showPQ || _showQP) {
        _showQQ = showQQ;
        if (_showQQ) {
          _showPQ = false;
          _showQP = false;
        }
      }
      CheckPolarizations(true);
    }
  }
  sigc::signal<void>& SignalStateChange() { return _signalStateChange; }
  sigc::signal<void>& SignalRecentFilesChanged() {
    return _signalRecentFilesChanged;
  }

  void PlotDist();
  void PlotLogLogDist();
  void PlotMeanSpectrum() { plotMeanSpectrum(false); }
  void PlotSumSpectrum() { plotMeanSpectrum(true); }
  void PlotPowerSpectrum();
  void PlotFrequencyScatter();
  void PlotTimeMean();
  void DrawTimeMean(XYPlot& plot);
  void PlotTimeScatter();
  void PlotSingularValues();

  void OpenMS(const std::vector<std::string>& filenames,
              const MSOptions& options);

  void OpenTestSet(algorithms::RFITestSet rfiSet,
                   algorithms::BackgroundTestSet backgroundSet);

  std::vector<std::string> RecentFiles() const;

  bool IsImageLoaded() const;

  TimeFrequencyData ActiveData() const;
  TimeFrequencyData OriginalData() const;

  TimeFrequencyMetaDataCPtr SelectedMetaData() const;

  class PlotManager& PlotManager() {
    return *_plotManager;
  }

  void ExecutePythonStrategy();
  void ExecuteLuaStrategy(class ProgressListener& listener);
  void JoinLuaThread();

  void CloseImageSet();

  bool HasImageSet() const { return _imageSet != nullptr; }

  void SetImageSet(std::unique_ptr<imagesets::ImageSet> newImageSet);

  void SetImageSetIndex(const imagesets::ImageSetIndex& newImageSetIndex);

  imagesets::ImageSet& GetImageSet() const { return *_imageSet; }

  const imagesets::ImageSetIndex& GetImageSetIndex() const {
    return _imageSetIndex;
  }

  void LoadCurrentTFDataAsync(class ProgressListener& progress);

  void LoadCurrentTFDataFinish(bool success);

  std::mutex& IOMutex() { return _ioMutex; }

  ImageComparisonController& TFController() { return _tfController; }

  void Open(const std::vector<std::string>& filenames);
  void SaveBaselineFlags();

  void CheckPolarizations(bool forceSignal = false);

  void GetAvailablePolarizations(bool& pp, bool& pq, bool& qp, bool& qq) const;

  void SaveBaseline(const std::string& filename);

  void NewDefaultStrategy();
  void NewEmptyStrategy();
  void NewTemplateStrategy();
  void OpenStrategy(const std::string& filename);
  bool HasOpenStrategy() const { return !_openStrategyFilename.empty(); }
  const std::string& StrategyFilename() const { return _openStrategyFilename; }
  void SaveStrategy();
  void SaveStrategyAs(const std::string& filename);

  std::string GetWorkStrategyText() const;
  void SetWorkStrategyText(const std::string& text);

 private:
  void plotMeanSpectrum(bool weight);
  void setRecentFile(const std::string& filename);
  void open(std::unique_ptr<imagesets::ImageSet> imageSet);

  void OpenMsConcatenated(const std::vector<std::string>& filenames,
                          const MSOptions& options);

  bool _showOriginalFlags, _showAlternativeFlags;
  bool _showPP, _showPQ, _showQP, _showQQ;

  sigc::signal<void> _signalStateChange;
  sigc::signal<void> _signalRecentFilesChanged;
  class RFIGuiWindow* _rfiGuiWindow;
  class ImageComparisonController _tfController;

  Settings _settings;
  std::unique_ptr<class PlotManager> _plotManager;
  std::unique_ptr<imagesets::ImageSet> _imageSet;
  imagesets::ImageSetIndex _imageSetIndex;
  std::unique_ptr<imagesets::BaselineData> _tempLoadedBaseline;
  std::mutex _ioMutex;
  std::thread _processingThread;
  TimeFrequencyData _scriptTFData;
  std::unique_ptr<class ScriptData> _scriptData;
  std::string _openStrategyFilename;
};

#endif
