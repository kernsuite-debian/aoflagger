#ifndef FITSIMAGESET_H
#define FITSIMAGESET_H

#include <vector>
#include <set>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "imageset.h"

#include "../structures/antennainfo.h"
#include "../structures/types.h"

#include "../util/progress/progresslistener.h"

namespace imagesets {

class FitsImageSet final : public ImageSet {
 public:
  explicit FitsImageSet(const std::string& file);
  ~FitsImageSet();

  void Initialize() override;

  std::unique_ptr<ImageSet> Clone() override;

  size_t Size() const override { return _baselines.size() * _bandCount; }

  std::string Description(const ImageSetIndex& index) const override;

  std::string Name() const override { return Files().front(); }
  std::vector<std::string> Files() const override;

  void AddReadRequest(const ImageSetIndex& index) override {
    _readRequests.emplace_back(index);
  }

  void PerformReadRequests(ProgressListener& progress) override {
    // Immediately clear the _readRequests, to have it empty in case of
    // exceptions
    const std::vector<ImageSetIndex> requests = std::move(_readRequests);
    _readRequests.clear();
    for (size_t i = 0; i != requests.size(); ++i) {
      _baselineData.emplace_back(loadData(requests[i]));
      progress.OnProgress(i + 1, requests.size());
    }
    progress.OnFinish();
  }

  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> data(new BaselineData(_baselineData.back()));
    _baselineData.resize(_baselineData.size() - 1);
    return data;
  }
  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override;
  void PerformWriteFlagsTask() override;
  void PerformWriteDataTask(const ImageSetIndex&, std::vector<Image2DCPtr>,
                            std::vector<Image2DCPtr>) override {
    throw std::runtime_error("Not implemented");
  }
  std::string TelescopeName() override;
  bool HasCrossCorrelations() const override { return false; }
  const std::vector<std::pair<size_t, size_t>>& Baselines() const {
    return _baselines;
  }
  size_t BandCount() const { return _bandCount; }
  class AntennaInfo GetAntennaInfo(unsigned antennaIndex) const {
    return _antennaInfos[antennaIndex];
  }
  const std::string& SourceName() const { return _sourceName; }
  bool IsDynSpectrumType() const { return _fitsType == DynSpectrumType; }

 private:
  FitsImageSet(const FitsImageSet& source);
  BaselineData loadData(const ImageSetIndex& index);

  void ReadPrimarySingleTable(TimeFrequencyData& data,
                              TimeFrequencyMetaData& metaData);
  void ReadTable(TimeFrequencyData& data, TimeFrequencyMetaData& metaData,
                 size_t bandIndex);
  void ReadAntennaTable(TimeFrequencyMetaData& metaData);
  void ReadFrequencyTable(TimeFrequencyData& data,
                          TimeFrequencyMetaData& metaData);
  void ReadCalibrationTable();
  void ReadSingleDishTable(TimeFrequencyData& data,
                           TimeFrequencyMetaData& metaData, size_t ifIndex);
  void ReadDynSpectrum(TimeFrequencyData& data,
                       TimeFrequencyMetaData& metaData);

  TimeFrequencyData ReadPrimaryGroupTable(size_t baselineIndex, int band,
                                          int stokes,
                                          TimeFrequencyMetaData& metaData);

  void saveSingleDishFlags(const std::vector<Mask2DCPtr>& flags,
                           size_t ifIndex);
  void saveDynSpectrumFlags(const std::vector<Mask2DCPtr>& flags);

  std::shared_ptr<class FitsFile> _file;
  std::vector<std::pair<size_t, size_t>> _baselines;
  size_t _bandCount;
  std::vector<AntennaInfo> _antennaInfos;
  std::map<int, BandInfo> _bandInfos;
  std::vector<int> _bandIndexToNumber;
  size_t _currentBaselineIndex, _currentBandIndex;
  double _frequencyOffset;
  std::string _sourceName;

  std::vector<ImageSetIndex> _readRequests;
  std::vector<BaselineData> _baselineData;
  enum { UVFitsType, SDFitsType, DynSpectrumType } _fitsType;
};

}  // namespace imagesets

#endif
