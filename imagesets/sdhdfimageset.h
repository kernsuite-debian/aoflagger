#ifndef SDHDF_IMAGE_SET_H
#define SDHDF_IMAGE_SET_H

#include <queue>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "imageset.h"

namespace H5 {
class DataSet;
class H5File;
}  // namespace H5

namespace imagesets {

/**
 * Implements the single-dish hdf5 format.
 * The description of the format is here:
 * https://bitbucket.csiro.au/projects/CPDA/repos/sdhdf_tools/browse/docs
 */
class SdhdfImageSet final : public ImageSet {
 public:
  SdhdfImageSet(const std::string& path);
  ~SdhdfImageSet() override;
  size_t Size() const override { return _indexTable.size(); }
  std::string Description(const ImageSetIndex& index) const override;
  std::unique_ptr<ImageSet> Clone() override {
    return std::unique_ptr<ImageSet>(new SdhdfImageSet(*this));
  }
  void Initialize() override;

  std::string Name() const override { return "SDHDF"; }

  std::vector<std::string> Files() const override {
    return std::vector<std::string>{_path};
  }

  std::string TelescopeName() override { return _telescopeName; }

  bool HasCrossCorrelations() const override { return false; }

  void AddReadRequest(const ImageSetIndex& index) override {
    _requests.emplace_back(index);
  }
  void PerformReadRequests(class ProgressListener& progress) override;

  std::unique_ptr<BaselineData> GetNextRequested() override {
    BaselineData baseline(std::move(_baselineBuffer.front()));
    _baselineBuffer.pop();
    return std::make_unique<BaselineData>(baseline);
  }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override;
  void PerformWriteFlagsTask() override {}

 private:
  /**
   * Open the data set @c name inside @c file, and store the result in @c
   * dataSet. If the data set does not exist, @c false is returned.
   */
  static bool tryOpen(H5::DataSet& dataSet, H5::H5File& file,
                      const std::string& name);

  struct Header {
    int nBeams;
    char telescopeName[64];
  };

  struct BandParams {
    char label[64];
    double centreFrequency;
    double lowFrequency;
    double highFrequency;
    int nChannels;
    int nPolarizations;
  };

  struct Beam {
    std::vector<BandParams> bandParams;
  };

  const std::string _path;
  std::vector<ImageSetIndex> _requests;
  std::vector<Beam> _beams;
  std::vector<std::pair<size_t /*beam*/, size_t /*band*/>> _indexTable;
  std::queue<BaselineData> _baselineBuffer;
  std::string _telescopeName;

  BaselineData loadData(ProgressListener& progress, const ImageSetIndex& index);
};
}  // namespace imagesets

#endif
