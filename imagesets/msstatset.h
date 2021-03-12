#ifndef MS_STAT_SET_H
#define MS_STAT_SET_H

#include <memory>
#include <queue>

#include "../../msio/msstatreader.h"

#include "imageset.h"

namespace rfiStrategy {

class MSStatSet final : public ImageSet {
 public:
  MSStatSet(const std::string& path, const std::string& dataColumn,
            BaselineIntegration::Mode statistic,
            BaselineIntegration::Differencing diffType, bool includeAutos,
            bool includeFlags)
      : _path(path),
        _statistic(statistic),
        _diffType(diffType),
        _includeAutos(includeAutos),
        _includeFlags(includeFlags),
        _reader(new MSStatReader(_path,
                                 dataColumn.empty() ? "DATA" : dataColumn)) {}

  std::unique_ptr<ImageSet> Clone() override {
    return std::unique_ptr<ImageSet>(new MSStatSet(*this));
  }

  size_t Size() const override {
    return _reader->NBands() * _reader->NSequences();
  }

  std::string Description(const ImageSetIndex& index) const override {
    return "ms stat set";
  }  // TODO

  void Initialize() override {}

  std::string Name() const override { return _path; }

  std::vector<std::string> Files() const override {
    return std::vector<std::string>{_path};
  }

  std::string TelescopeName() override { return _reader->TelescopeName(); }

  void AddReadRequest(const ImageSetIndex& index) override {
    _requests.emplace_back(index);
  }

  void PerformReadRequests(class ProgressListener& progress) override {
    for (const ImageSetIndex& request : _requests) {
      size_t bandIndex = request.Value() % _reader->NBands(),
             seqIndex = request.Value() / _reader->NBands();
      MSStatReader::Result result =
          _reader->Read(_statistic, _diffType, seqIndex, bandIndex,
                        _includeAutos, _includeFlags, progress);
      _results.emplace(result.first, result.second, request);
    }
    _requests.clear();
  }

  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> result(
        new BaselineData(std::move(_results.front())));
    _results.pop();
    return result;
  }

  void AddWriteFlagsTask(const ImageSetIndex& index,
                         std::vector<Mask2DCPtr>& flags) override {
    size_t bandIndex = index.Value() % _reader->NBands(),
           seqIndex = index.Value() / _reader->NBands();
    _reader->StoreFlags(flags, _diffType, seqIndex, bandIndex, _includeAutos);
  }

  void PerformWriteFlagsTask() override {}

  bool HasCrossCorrelations() const override { return false; }

 private:
  std::string _path;
  BaselineIntegration::Mode _statistic;
  BaselineIntegration::Differencing _diffType;
  bool _includeAutos, _includeFlags;
  std::shared_ptr<MSStatReader> _reader;
  size_t _size;
  std::vector<ImageSetIndex> _requests;
  std::queue<BaselineData> _results;
};

}  // namespace rfiStrategy

#endif
