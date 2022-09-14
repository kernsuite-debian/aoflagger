#ifndef PARM_IMAGE_H
#define PARM_IMAGE_H

#include <string>
#include <cstring>
#include <vector>
#include <deque>

#include "../structures/types.h"
#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "../lua/telescopefile.h"

#include "imageset.h"

namespace imagesets {

class ParmImageSet final : public ImageSet {
 public:
  ParmImageSet(const std::string& path) : _path(path), _parmTable(nullptr) {}
  ~ParmImageSet() override;
  size_t Size() const override { return AntennaCount(); }
  inline std::string Description(const ImageSetIndex& index) const override {
    return AntennaName(index.Value());
  }
  std::unique_ptr<ImageSet> Clone() override {
    throw std::runtime_error("Cannot copy set");
  }
  void Initialize() override;

  std::string Name() const override { return "Parmdb"; }

  std::vector<std::string> Files() const override {
    return std::vector<std::string>{_path};
  }

  std::string TelescopeName() override {
    return TelescopeFile::TelescopeName(TelescopeFile::GENERIC_TELESCOPE);
  }

  TimeFrequencyData* LoadData(const ImageSetIndex& index);

  void AddReadRequest(const ImageSetIndex& index) override {
    TimeFrequencyData* data = LoadData(index);
    BaselineData* baseline =
        new BaselineData(*data, TimeFrequencyMetaDataCPtr(), index);
    delete data;
    _baselineBuffer.push_back(baseline);
  }
  void PerformReadRequests(class ProgressListener&) override {}

  std::unique_ptr<BaselineData> GetNextRequested() override {
    std::unique_ptr<BaselineData> baseline(std::move(_baselineBuffer.front()));
    _baselineBuffer.pop_front();
    return baseline;
  }

  unsigned AntennaCount() const { return _antennas.size(); }
  std::string AntennaName(unsigned index) const { return _antennas[index]; }

 private:
  const std::string _path;
  std::vector<std::string> _antennas;
  class ParmTable* _parmTable;
  std::deque<BaselineData*> _baselineBuffer;
};
}  // namespace imagesets

#endif
