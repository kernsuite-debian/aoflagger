#ifndef DIRECTBASELINEREADER_H
#define DIRECTBASELINEREADER_H

#include <map>
#include <vector>
#include <stdexcept>

#include "baselinereader.h"

#include "../structures/antennainfo.h"
#include "../structures/image2d.h"
#include "../structures/mask2d.h"
#include "../structures/msmetadata.h"

class DirectBaselineReader final : public BaselineReader {
 public:
  explicit DirectBaselineReader(const std::string& msFile);

  bool IsModified() const override { return false; }

  void WriteToMs() override {}

  void PrepareReadWrite(ProgressListener&) override {}

  void PerformReadRequests(class ProgressListener& listener) override;
  void PerformFlagWriteRequests() override;
  void PerformDataWriteTask(
      [[maybe_unused]] std::vector<Image2DCPtr> realImages,
      [[maybe_unused]] std::vector<Image2DCPtr> imaginaryImages,
      [[maybe_unused]] size_t antenna1, [[maybe_unused]] size_t antenna2,
      [[maybe_unused]] size_t spectralWindow,
      [[maybe_unused]] size_t sequenceId) override {
    throw std::runtime_error(
        "The direct baseline reader can not write data back to file: use the "
        "indirect reader");
  }
  std::vector<UVW> ReadUVW(unsigned antenna1, unsigned antenna2,
                           unsigned spectralWindow, unsigned sequenceId);

 private:
  class BaselineCacheIndex {
   public:
    BaselineCacheIndex() {}
    BaselineCacheIndex(const BaselineCacheIndex& source)
        : antenna1(source.antenna1),
          antenna2(source.antenna2),
          spectralWindow(source.spectralWindow),
          sequenceId(source.sequenceId) {}
    bool operator==(const BaselineCacheIndex& rhs) const {
      return antenna1 == rhs.antenna1 && antenna2 == rhs.antenna2 &&
             spectralWindow == rhs.spectralWindow &&
             sequenceId == rhs.sequenceId;
    }
    bool operator<(const BaselineCacheIndex& rhs) const {
      if (antenna1 < rhs.antenna1)
        return true;
      else if (antenna1 == rhs.antenna1) {
        if (antenna2 < rhs.antenna2)
          return true;
        else if (antenna2 == rhs.antenna2) {
          if (spectralWindow < rhs.spectralWindow)
            return true;
          else if (spectralWindow == rhs.spectralWindow)
            return sequenceId < rhs.sequenceId;
        }
      }
      return false;
    }

    size_t antenna1, antenna2, spectralWindow, sequenceId;
  };

  struct BaselineCacheValue {
    std::vector<size_t> rows;
  };

  void initBaselineCache();

  void addRequestRows(ReadRequest request, size_t requestIndex,
                      std::vector<std::pair<size_t, size_t>>& rows);
  void addRequestRows(FlagWriteRequest request, size_t requestIndex,
                      std::vector<std::pair<size_t, size_t>>& rows);
  void addRowToBaselineCache(size_t antenna1, size_t antenna2,
                             size_t spectralWindow, size_t sequenceId,
                             size_t row);
  void readUVWData();

  void readTimeData(size_t requestIndex, size_t xOffset, size_t frequencyCount,
                    const casacore::Array<casacore::Complex>& data);
  void readTimeFlags(size_t requestIndex, size_t xOffset, size_t frequencyCount,
                     const casacore::Array<bool>& flag);
  void readWeights(size_t requestIndex, size_t xOffset, size_t frequencyCount,
                   const casacore::Array<float>& weight);

  std::map<BaselineCacheIndex, BaselineCacheValue> _baselineCache;
  casacore::MeasurementSet _ms;
};

#endif  // DIRECTBASELINEREADER_H
