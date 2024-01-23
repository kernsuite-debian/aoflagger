#ifndef IMAGECOMPARISONCONTROLLER_H
#define IMAGECOMPARISONCONTROLLER_H

#include <gtkmm/drawingarea.h>

#include <utility>
#include <vector>

#include "../../structures/image2d.h"
#include "../../structures/timefrequencydata.h"
#include "../../structures/timefrequencymetadata.h"
#include "../../structures/segmentedimage.h"

#include "../maskedheatmap.h"

class ImageComparisonController {
 public:
  ImageComparisonController();
  void Clear();
  void SetNewData(const class TimeFrequencyData& image,
                  TimeFrequencyMetaDataCPtr metaData);

  /**
   * The currently visible data. This includes the masks that are activated and
   * the data is trimmed to the zoomed region. In case none of the masks are
   * visualized, the returned data has an unset mask.
   */
  TimeFrequencyData GetActiveData() const;

  /**
   * Same as @ref GetActiveData(), but full size ("zoomed out").
   */
  TimeFrequencyData GetActiveDataFullSize() const;

  /**
   * The full-size "original" data, including the original mask.
   */
  TimeFrequencyData& OriginalData() { return _dataList.front().data; }
  const TimeFrequencyData& OriginalData() const {
    return _dataList.front().data;
  }

  void SetAltMaskData(const TimeFrequencyData& data) {
    if (_visualizedImage == 0)
      _dataList.back().data = data;
    else
      _dataList[_visualizedImage].data = data;
    updateVisualizedImageAndMask();
  }

  /**
   * The full-size alternative mask
   */
  TimeFrequencyData AltMaskData() const {
    if (_visualizedImage == 0) {
      if (_dataList.size() > 1) {
        return _dataList.back().data;
      } else {
        TimeFrequencyData empty = _dataList.back().data;
        empty.SetNoMask();
        return empty;
      }
    } else {
      return _dataList[_visualizedImage].data;
    }
  }

  size_t AddVisualization(const std::string& label,
                          const TimeFrequencyData& data) {
    _dataList.emplace_back(label, data);
    size_t v = _dataList.size() - 1;
    // If the first image is visualized, adding a new last visualization
    // affects the visualized flags on top of the first image.
    if (_visualizedImage == 0) updateVisualizedImageAndMask();
    _visualizationListChange.emit();
    return v;
  }

  template <typename T>
  void SetVisualizationData(size_t index, T&& data) {
    _dataList[index].data = std::forward<T>(data);
    if (index == 0 || _visualizedImage == index ||
        (_visualizedImage == 0 && index == _dataList.size() - 1))
      updateVisualizedImageAndMask();
  }

  /**
   * Currently visualized data. This is the full set, so "zoomed out".
   * In contrast to GetActiveData(), these data always include the mask
   * that is part of the visualization.
   */
  const TimeFrequencyData& VisualizedData() const {
    return _dataList[_visualizedImage].data;
  }

  void SetVisualizedData(const TimeFrequencyData& data) {
    _dataList[_visualizedImage].data = data;
    updateVisualizedImageAndMask();
  }

  size_t VisualizationCount() const { return _dataList.size(); }
  size_t VisualizedIndex() const { return _visualizedImage; }

  const TimeFrequencyData& GetVisualizationData(size_t index) const {
    return _dataList[index].data;
  }
  const std::string& GetVisualizationLabel(size_t index) {
    return _dataList[index].label;
  }

  void SetVisualization(size_t visualizationIndex) {
    if (_visualizedImage != visualizationIndex &&
        _visualizedImage < _dataList.size()) {
      _visualizedImage = visualizationIndex;
      updateVisualizedImageAndMask();
    }
  }
  void TryVisualizePolarizations(bool& pp, bool& pq, bool& qp, bool& qq) const;
  void SetVisualizedPolarization(bool pp, bool pq, bool qp, bool qq) {
    if (_showPP != pp || _showPQ != pq || _showQP != qp || _showQQ != qq) {
      _showPP = pp;
      _showPQ = pq;
      _showQP = qp;
      _showQQ = qq;
      updateVisualizedImageAndMask();
    }
  }

  void ClearAllButOriginal();

  MaskedHeatMap& Plot() { return _plot; }
  const MaskedHeatMap& Plot() const { return _plot; }

  sigc::signal<void>& VisualizationListChange() {
    return _visualizationListChange;
  }

 private:
  void getFirstAvailablePolarization(bool& pp, bool& pq, bool& qp,
                                     bool& qq) const;
  void updateVisualizedImageAndMask();
  void updateVisualizedImage();
  Mask2DCPtr getSelectedPolarizationMask(const TimeFrequencyData& data) const;
  void getActiveMask(TimeFrequencyData& data) const {
    bool orActive =
        _plot.ShowOriginalMask() && _dataList[0].data.MaskCount() != 0;
    size_t altMaskIndex = _visualizedImage;
    if (altMaskIndex == 0) altMaskIndex = _dataList.size() - 1;
    bool altActive = _plot.ShowAlternativeMask() && altMaskIndex != 0 &&
                     _dataList[altMaskIndex].data.MaskCount() != 0;
    if (orActive && altActive) {
      data.SetMask(_dataList[0].data);
      data.JoinMask(_dataList[altMaskIndex].data);
    } else if (orActive) {
      data.SetMask(_dataList[0].data);
    } else if (altActive) {
      data.SetMask(_dataList[altMaskIndex].data);
    } else {
      data.SetMasksToValue<false>();
    }
  }
  MaskedHeatMap _plot;
  bool _showPP, _showPQ, _showQP, _showQQ;
  size_t _visualizedImage;
  struct DataEntry {
    DataEntry(const std::string& _label, const TimeFrequencyData& _data) {
      label = _label;
      data = _data;
    }
    std::string label;
    TimeFrequencyData data;
  };
  std::vector<DataEntry> _dataList;
  TimeFrequencyMetaDataCPtr _metaData;
  sigc::signal<void> _visualizationListChange;
};

#endif
