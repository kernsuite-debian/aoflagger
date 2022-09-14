#ifndef GUI_IMAGESET_H
#define GUI_IMAGESET_H

#include <cstring>
#include <string>
#include <memory>
#include <vector>

#include "imagesetindex.h"

#include "../structures/types.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

class ProgressListener;
struct MSOptions;

namespace imagesets {

class BaselineData {
 public:
  BaselineData(const TimeFrequencyData& data,
               const TimeFrequencyMetaDataCPtr& metaData,
               const ImageSetIndex& index)
      : _data(data), _metaData(metaData), _index(index) {}

  explicit BaselineData(const ImageSetIndex& index)
      : _data(), _metaData(), _index(index) {}

  BaselineData(const TimeFrequencyData& data,
               const TimeFrequencyMetaDataCPtr& metaData)
      : _data(data), _metaData(metaData), _index() {}

  explicit BaselineData(const TimeFrequencyMetaDataCPtr& metaData)
      : _data(), _metaData(metaData), _index() {}

  BaselineData() : _data(), _metaData(), _index() {}

  BaselineData(const BaselineData& source) = default;
  BaselineData(BaselineData&& source) = default;
  BaselineData& operator=(const BaselineData& source) = default;
  BaselineData& operator=(BaselineData&& source) = default;

  const TimeFrequencyData& Data() const { return _data; }
  void SetData(const TimeFrequencyData& data) { _data = data; }

  TimeFrequencyMetaDataCPtr MetaData() const { return _metaData; }
  void SetMetaData(TimeFrequencyMetaDataCPtr metaData) { _metaData = metaData; }

  const ImageSetIndex& Index() const { return _index; }
  ImageSetIndex& Index() { return _index; }

  void SetIndex(const ImageSetIndex& newIndex) { _index = newIndex; }

 private:
  TimeFrequencyData _data;
  TimeFrequencyMetaDataCPtr _metaData;
  ImageSetIndex _index;
};

class ImageSet {
 public:
  virtual ~ImageSet(){};
  virtual std::unique_ptr<ImageSet> Clone() = 0;

  virtual std::string Description(const ImageSetIndex& index) const = 0;
  virtual size_t Size() const = 0;

  /**
   * Initialize is used to initialize the image set after it has been created
   * and after all possible options have been set that might influence
   * initialization (such as number of parts to read).
   */
  virtual void Initialize() = 0;
  virtual std::string Name() const = 0;
  virtual std::vector<std::string> Files() const = 0;
  virtual std::string TelescopeName() = 0;

  virtual void AddReadRequest(const ImageSetIndex& index) = 0;
  virtual void PerformReadRequests(class ProgressListener& progress) = 0;
  virtual std::unique_ptr<BaselineData> GetNextRequested() = 0;

  virtual void AddWriteFlagsTask(const ImageSetIndex& /*index*/,
                                 std::vector<Mask2DCPtr>& /*flags*/) {
    throw std::runtime_error("Not implemented");
  }
  virtual void PerformWriteFlagsTask() {
    throw std::runtime_error("Not implemented");
  }
  virtual void PerformWriteDataTask(
      const ImageSetIndex& /*index*/, std::vector<Image2DCPtr> /*_realImages*/,
      std::vector<Image2DCPtr> /*_imaginaryImages*/) {
    throw std::runtime_error("Not implemented");
  }
  /**
   * If an imageset has the concept of cross correlations, this returns true.
   * When true, aoflagger will by default only process baselines that are formed
   * from two different antennas. If false, it will process all baselines by
   * default, which is appropriate for single-dish image sets.
   */
  virtual bool HasCrossCorrelations() const { return true; }

  static std::unique_ptr<ImageSet> Create(const std::vector<std::string>& files,
                                          const MSOptions& options);
  static bool IsSdhdfFile(const std::string& file);
  static bool IsFitsFile(const std::string& file);
  static bool IsBHFitsFile(const std::string& file);
  static bool IsRCPRawFile(const std::string& file);
  static bool IsTKPRawFile(const std::string& file);
  static bool IsRawDescFile(const std::string& file);
  static bool IsParmFile(const std::string& file);
  static bool IsTimeFrequencyStatFile(const std::string& file);
  static bool IsMSFile(const std::string& file);
  static bool IsNoiseStatFile(const std::string& file);
  static bool IsPngFile(const std::string& file);
  static bool IsFilterBankFile(const std::string& file);
  static bool IsQualityStatSet(const std::string& file);
  static bool IsRFIBaselineSet(const std::string& file);

  ImageSetIndex StartIndex() const { return ImageSetIndex(Size()); }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         const TimeFrequencyData& data) {
    std::vector<Mask2DCPtr> flags;
    for (size_t i = 0; i != data.MaskCount(); ++i)
      flags.push_back(data.GetMask(i));
    AddWriteFlagsTask(index, flags);
  }

  void PerformWriteDataTask(const ImageSetIndex& index,
                            const TimeFrequencyData& data) {
    std::vector<Image2DCPtr> realImages, imaginaryImages;
    for (size_t i = 0; i != data.PolarizationCount(); ++i) {
      TimeFrequencyData polData(data.MakeFromPolarizationIndex(i));
      realImages.push_back(polData.GetRealPart());
      imaginaryImages.push_back(polData.GetImaginaryPart());
    }
    PerformWriteDataTask(index, realImages, imaginaryImages);
  }
};

}  // namespace imagesets

#endif
