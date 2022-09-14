#ifndef MEMORY_BASELINE_READER_H
#define MEMORY_BASELINE_READER_H

#include <map>
#include <vector>
#include <stdexcept>

#include "../structures/antennainfo.h"

#include "baselinereader.h"

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

class MemoryBaselineReader final : public BaselineReader {
 public:
  explicit MemoryBaselineReader(const std::string& msFile)
      : BaselineReader(msFile), _isRead(false), _areFlagsChanged(false) {}

  ~MemoryBaselineReader() {
    if (_areFlagsChanged) {
      WriteToMs();
    }
  }

  void PrepareReadWrite(ProgressListener& progress) override;

  void PerformReadRequests(class ProgressListener& progress) override;

  void PerformFlagWriteRequests() override;

  void PerformDataWriteTask(
      [[maybe_unused]] std::vector<Image2DCPtr> realImages,
      [[maybe_unused]] std::vector<Image2DCPtr> imaginaryImages,
      [[maybe_unused]] size_t antenna1, [[maybe_unused]] size_t antenna2,
      [[maybe_unused]] size_t spectralWindow,
      [[maybe_unused]] size_t sequenceId) override {
    throw std::runtime_error(
        "The full mem reader can not write data back to file: use the indirect "
        "reader");
  }

  static bool IsEnoughMemoryAvailable(const std::string& msFile);
  static bool IsEnoughMemoryAvailable(uint64_t size);

  size_t GetMinRecommendedBufferSize(size_t /*threadCount*/) override {
    return 1;
  }
  size_t GetMaxRecommendedBufferSize(size_t /*threadCount*/) override {
    return 2;
  }

  bool IsModified() const override { return _areFlagsChanged; }

  void WriteToMs() override;

 private:
  void readSet(class ProgressListener& progress);
  void clear();

  bool _isRead, _areFlagsChanged;

  class BaselineID {
   public:
    BaselineID(unsigned a1, unsigned a2, unsigned _spw, unsigned seqId)
        : antenna1(a1), antenna2(a2), spw(_spw), sequenceId(seqId) {
      if (antenna1 > antenna2) std::swap(antenna1, antenna2);
    }
    unsigned antenna1, antenna2, spw, sequenceId;

    bool operator<(const BaselineID& other) const {
      if (antenna1 < other.antenna1)
        return true;
      else if (antenna1 == other.antenna1) {
        if (antenna2 < other.antenna2)
          return true;
        else if (antenna2 == other.antenna2) {
          if (spw < other.spw)
            return true;
          else if (spw == other.spw)
            return sequenceId < other.sequenceId;
        }
      }
      return false;
    }
  };

  std::map<BaselineID, std::unique_ptr<BaselineReader::Result>> _baselines;
};

#endif  // MEMORY_BASELINE_READER_H
