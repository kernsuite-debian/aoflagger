#ifndef INDIRECTBASELINEREADER_H
#define INDIRECTBASELINEREADER_H

#include <fstream>
#include <map>
#include <memory>
#include <vector>
#include <stdexcept>

#include "baselinereader.h"
#include "directbaselinereader.h"

class IndirectBaselineReader : public BaselineReader {
 public:
  explicit IndirectBaselineReader(const std::string& msFile);
  ~IndirectBaselineReader();

  bool IsModified() const override {
    return reordered_data_files_have_changed_ ||
           reordered_flag_files_have_changed_;
  }

  void WriteToMs() override;

  void PrepareReadWrite(ProgressListener& progress) override;

  virtual void PerformReadRequests(
      class ProgressListener& progress) final override;
  virtual void PerformFlagWriteRequests() final override;
  virtual void PerformDataWriteTask(std::vector<Image2DCPtr> _realImages,
                                    std::vector<Image2DCPtr> _imaginaryImages,
                                    size_t antenna1, size_t antenna2,
                                    size_t spectralWindow,
                                    size_t sequenceId) final override;

  virtual size_t GetMinRecommendedBufferSize(
      size_t /*threadCount*/) final override {
    return 1;
  }
  virtual size_t GetMaxRecommendedBufferSize(
      size_t /*threadCount*/) final override {
    return 2;
  }

  void SetReadUVW(bool readUVW) { read_uvw_ = readUVW; }

 private:
  struct ReorderInfo {
    std::unique_ptr<std::ofstream> dataFile;
    std::unique_ptr<std::ofstream> flagFile;
  };
  struct UpdateInfo {
    std::unique_ptr<std::ifstream> dataFile;
    std::unique_ptr<std::ifstream> flagFile;
  };
  class SeqIndexLookupTable {
   public:
    SeqIndexLookupTable(size_t antennaCount, size_t spectralWindowCount,
                        size_t sequenceCount)
        : _antennaCount(antennaCount), _table(sequenceCount) {
      size_t maxBaselineCount = antennaCount * antennaCount;
      for (size_t i = 0; i != sequenceCount; ++i) {
        std::vector<std::vector<size_t>>& spwTable = _table[i];
        spwTable.resize(spectralWindowCount);
        for (size_t j = 0; j != spectralWindowCount; ++j) {
          std::vector<size_t>& baselTable = spwTable[j];
          baselTable.resize(maxBaselineCount);
        }
      }
    }
    size_t& Value(size_t antenna1, size_t antenna2, size_t spectralWindow,
                  size_t sequenceId) {
      return _table[sequenceId][spectralWindow]
                   [antenna1 * _antennaCount + antenna2];
    }

   private:
    size_t _antennaCount;
    std::vector<std::vector<std::vector<size_t>>> _table;
  };
  void reorderMS(class ProgressListener& progress);
  void reorderFull(class ProgressListener& progress);
  void makeLookupTables(size_t& fileSize);
  void updateOriginalMSData(class ProgressListener& progress);
  void updateOriginalMSFlags(class ProgressListener& progress);
  void performFlagWriteTask(std::vector<Mask2DCPtr> flags, unsigned antenna1,
                            unsigned antenna2, unsigned spw,
                            unsigned sequenceId);

  template <bool UpdateData, bool UpdateFlags>
  void updateOriginalMS(class ProgressListener& progress);

  void removeTemporaryFiles();

  static void preAllocate(const std::string& filename, size_t fileSize);

  DirectBaselineReader direct_reader_;
  std::unique_ptr<SeqIndexLookupTable> sequence_index_table_;
  std::vector<size_t> file_positions_;
  std::string data_filename_;
  std::string flag_filename_;
  std::string meta_filename_;
  bool ms_is_reordered_;
  bool remove_reordered_files_;
  bool reordered_data_files_have_changed_;
  bool reordered_flag_files_have_changed_;
  bool read_uvw_;
};

#endif  // INDIRECTBASELINEREADER_H
