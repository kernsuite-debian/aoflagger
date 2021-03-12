#ifndef BHFITSIMAGESET_H
#define BHFITSIMAGESET_H

#include <vector>
#include <set>
#include <stack>
#include <map>
#include <memory>

#include "imageset.h"

#include "../../structures/antennainfo.h"
#include "../../structures/types.h"

namespace rfiStrategy {

class BHFitsImageSet final : public ImageSet {
 public:
  explicit BHFitsImageSet(const std::string &file);
  ~BHFitsImageSet();
  void Initialize() override;

  std::unique_ptr<ImageSet> Clone() override;

  std::string Description(const ImageSetIndex &index) const override;

  std::string Name() const override { return "Bighorns fits file"; }
  std::vector<std::string> Files() const override;

  size_t Size() const override { return _timeRanges.size(); }

  const std::string &RangeName(size_t rangeIndex) const {
    return _timeRanges[rangeIndex].name;
  }

  void AddReadRequest(const ImageSetIndex &index) override {
    _baselineData.push(loadData(index));
  }
  void PerformReadRequests(class ProgressListener &) override {}
  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> data(new BaselineData(_baselineData.top()));
    _baselineData.pop();
    return data;
  }
  void AddWriteFlagsTask(const ImageSetIndex &index,
                         std::vector<Mask2DCPtr> &flags) override;
  void PerformWriteFlagsTask() override;
  void PerformWriteDataTask(const ImageSetIndex &, std::vector<Image2DCPtr>,
                            std::vector<Image2DCPtr>) override {
    throw std::runtime_error("Not implemented");
  }
  std::string TelescopeName() override { return "Bighorns"; }
  bool HasCrossCorrelations() const override { return false; }

 private:
  struct TimeRange {
    int start, end;
    std::string name;

    TimeRange() {}
    TimeRange(const TimeRange &source)
        : start(source.start), end(source.end), name(source.name) {}

    TimeRange &operator=(const TimeRange &source) {
      start = source.start;
      end = source.end;
      name = source.name;
      return *this;
    }
  };

  BHFitsImageSet(const BHFitsImageSet &source);
  BaselineData loadData(const ImageSetIndex &index);
  void loadImageData(TimeFrequencyData &data,
                     const TimeFrequencyMetaDataPtr &metaData,
                     const ImageSetIndex &index);
  std::pair<int, int> getRangeFromString(const std::string &rangeStr);
  std::string flagFilePath() const;

  std::shared_ptr<class FitsFile> _file;
  std::stack<BaselineData> _baselineData;
  std::vector<TimeRange> _timeRanges;
  int _width, _height;
};

}  // namespace rfiStrategy

#endif
