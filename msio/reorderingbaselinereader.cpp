#include "reorderingbaselinereader.h"

#include "../structures/timefrequencydata.h"

#include "../util/logger.h"
#include "../util/stopwatch.h"
#include "../util/progress/dummyprogresslistener.h"

#include "reorderedfilebuffer.h"
#include "msselection.h"

#include <aocommon/system.h>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <stdexcept>
#include <vector>

#include <fcntl.h>

ReorderingBaselineReader::ReorderingBaselineReader(const std::string& msFile)
    : BaselineReader(msFile),
      direct_reader_(msFile),
      sequence_index_table_(),
      ms_is_reordered_(false),
      remove_reordered_files_(false),
      reordered_data_files_have_changed_(false),
      reordered_flag_files_have_changed_(false),
      read_uvw_(false) {
  // In order to use multiple readers at the same time the temporary files need
  // unique names. Use the address of the object to generate a unique prefix.
  const std::string uid = std::to_string(reinterpret_cast<uintptr_t>(this));

  data_filename_ = uid + "-aoflagger-data.tmp";
  flag_filename_ = uid + "-aoflagger-flag.tmp";
  meta_filename_ = uid + "-ao-msinfo.tmp";
}

ReorderingBaselineReader::~ReorderingBaselineReader() {
  WriteToMs();
  removeTemporaryFiles();
}

void ReorderingBaselineReader::WriteToMs() {
  DummyProgressListener dummy;
  if (reordered_data_files_have_changed_) updateOriginalMSData(dummy);
  if (reordered_flag_files_have_changed_) updateOriginalMSFlags(dummy);
}

void ReorderingBaselineReader::PrepareReadWrite(ProgressListener& progress) {
  if (!ms_is_reordered_) {
    reorderMS(progress);
  }
}

void ReorderingBaselineReader::PerformReadRequests(
    class ProgressListener& progress) {
  initializeMeta();

  PrepareReadWrite(dummy_progress_);

  _results.clear();
  for (size_t i = 0; i < _readRequests.size(); ++i) {
    const ReadRequest request = _readRequests[i];
    _results.push_back(Result());
    const size_t width = ObservationTimes(request.sequenceId).size();
    for (size_t p = 0; p < Polarizations().size(); ++p) {
      if (ReadData()) {
        _results[i]._realImages.push_back(Image2D::CreateZeroImagePtr(
            width, MetaData().FrequencyCount(request.spectralWindow)));
        _results[i]._imaginaryImages.push_back(Image2D::CreateZeroImagePtr(
            width, MetaData().FrequencyCount(request.spectralWindow)));
      }
      if (ReadFlags()) {
        // The flags should be initialized to true, as a baseline might
        // miss some time scans that other baselines do have, and these
        // should be flagged.
        _results[i]._flags.push_back(Mask2D::CreateSetMaskPtr<true>(
            width, MetaData().FrequencyCount(request.spectralWindow)));
      }
    }
    if (read_uvw_) {
      _results[i]._uvw =
          direct_reader_.ReadUVW(request.antenna1, request.antenna2,
                                 request.spectralWindow, request.sequenceId);
    } else {
      _results[i]._uvw.clear();
      for (unsigned j = 0; j < width; ++j)
        _results[i]._uvw.emplace_back(0.0, 0.0, 0.0);
    }

    std::ifstream dataFile(data_filename_, std::ifstream::binary);
    std::ifstream flagFile(flag_filename_, std::ifstream::binary);
    const size_t index = sequence_index_table_->Value(
        request.antenna1, request.antenna2, request.spectralWindow,
        request.sequenceId);
    const size_t filePos = file_positions_[index];
    dataFile.seekg(filePos * (sizeof(float) * 2), std::ios_base::beg);
    flagFile.seekg(filePos * sizeof(bool), std::ios_base::beg);

    const size_t bufferSize =
        MetaData().FrequencyCount(request.spectralWindow) *
        Polarizations().size();
    for (size_t x = 0; x < width; ++x) {
      std::vector<float> dataBuffer(bufferSize * 2);
      std::vector<char> flagBuffer(bufferSize);
      dataFile.read((char*)&dataBuffer[0], bufferSize * sizeof(float) * 2);
      size_t dataBufferPtr = 0;
      flagFile.read((char*)&flagBuffer[0], bufferSize * sizeof(bool));
      size_t flagBufferPtr = 0;
      for (size_t f = 0; f < MetaData().FrequencyCount(request.spectralWindow);
           ++f) {
        for (size_t p = 0; p < Polarizations().size(); ++p) {
          _results[i]._realImages[p]->SetValue(x, f, dataBuffer[dataBufferPtr]);
          ++dataBufferPtr;
          _results[i]._imaginaryImages[p]->SetValue(x, f,
                                                    dataBuffer[dataBufferPtr]);
          ++dataBufferPtr;
          _results[i]._flags[p]->SetValue(x, f, flagBuffer[flagBufferPtr]);
          ++flagBufferPtr;
        }
      }
    }
  }

  _readRequests.clear();
  progress.OnFinish();
}

void ReorderingBaselineReader::PerformFlagWriteRequests() {
  for (size_t i = 0; i != _writeRequests.size(); ++i) {
    const FlagWriteRequest request = _writeRequests[i];
    performFlagWriteTask(request.flags, request.antenna1, request.antenna2,
                         request.spectralWindow, request.sequenceId);
  }
  _writeRequests.clear();
}

void ReorderingBaselineReader::reorderMS(ProgressListener& progress) {
  initializeMeta();
  progress.OnStartTask("Reordering measurement set");
  const std::filesystem::path path(meta_filename_);
  bool reorderRequired = true;

  if (std::filesystem::exists(path)) {
    std::ifstream str(path.string().c_str());
    std::string name;
    std::getline(str, name);
    if (std::filesystem::equivalent(std::filesystem::path(name),
                                    MetaData().Path())) {
      Logger::Debug << "Measurement set has already been reordered; using old "
                       "temporary files.\n";
      reorderRequired = false;
      ms_is_reordered_ = true;
      remove_reordered_files_ = false;
      reordered_data_files_have_changed_ = false;
      reordered_flag_files_have_changed_ = false;
    }
  }

  if (reorderRequired) {
    reorderFull(progress);
    std::ofstream str(path.string().c_str());
    str << MetaData().Path() << '\n';
  } else {
    size_t fileSize;
    makeLookupTables(fileSize);
  }
}

void ReorderingBaselineReader::makeLookupTables(size_t& fileSize) {
  std::vector<MSMetaData::Sequence> sequences = MetaData().GetSequences();
  const size_t antennaCount = MetaData().AntennaCount(),
               polarizationCount = Polarizations().size(),
               bandCount = MetaData().BandCount(),
               sequencesPerBaselineCount = MetaData().SequenceCount();

  sequence_index_table_.reset(new SeqIndexLookupTable(
      antennaCount, bandCount, sequencesPerBaselineCount));
  fileSize = 0;
  for (size_t i = 0; i < sequences.size(); ++i) {
    // Initialize look-up table to get index into Sequence-array quickly
    const MSMetaData::Sequence& s = sequences[i];
    sequence_index_table_->Value(s.antenna1, s.antenna2, s.spw, s.sequenceId) =
        i;

    // Initialize look-up table to go from sequence array to file position. Is
    // in samples, so multiple times sizeof(bool) or ..(float)) for exact
    // position.
    file_positions_.push_back(fileSize);
    fileSize += ObservationTimes(s.sequenceId).size() *
                MetaData().FrequencyCount(s.spw) * polarizationCount;
  }
}

void ReorderingBaselineReader::preAllocate(const std::string& filename,
                                           size_t fileSize) {
  Logger::Debug << "Pre-allocating " << (fileSize / (1024 * 1024))
                << " MB...\n";
  const int fd =
      open(filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IWUSR | S_IRUSR);
  if (fd < 0) {
    std::ostringstream s;
    s << "Error while opening file '" << filename
      << "', check access rights and free space";
    throw std::runtime_error(s.str());
  }
#if defined(HAVE_POSIX_FALLOCATE)
  const int allocResult = posix_fallocate(fd, 0, fileSize);
  close(fd);
  if (allocResult != 0) {
    Logger::Warn
        << "Could not allocate temporary file '" << filename
        << "': posix_fallocate returned " << allocResult
        << ".\n"
           "Tried to allocate "
        << (fileSize / (1024 * 1024))
        << " MB.\n"
           "Disk could be full or filesystem could not support fallocate.\n";
  }
#else
  close(fd);
  Logger::Warn << "Compiled without posix_fallocate() support: skipping "
                  "pre-allocation.\n";
#endif
}

void ReorderingBaselineReader::reorderFull(ProgressListener& progressListener) {
  const Stopwatch watch(true);

  casacore::MeasurementSet ms = OpenMS();

  casacore::ArrayColumn<bool> flagColumn(ms, "FLAG");
  casacore::ScalarColumn<int> dataDescIdColumn(ms, "DATA_DESC_ID");
  casacore::ScalarColumn<int> antenna1Column(ms, "ANTENNA1");
  casacore::ScalarColumn<int> antenna2Column(ms, "ANTENNA2");
  casacore::ArrayColumn<casacore::Complex> dataColumn(ms, DataColumnName());

  if (ms.nrow() == 0)
    throw std::runtime_error("Measurement set is empty (zero rows)");

  std::vector<size_t> dataIdToSpw;
  MetaData().GetDataDescToBandVector(dataIdToSpw);

  size_t fileSize;
  makeLookupTables(fileSize);

  Logger::Debug << "Opening temporary files.\n";
  ReorderInfo reorderInfo;
  preAllocate(data_filename_, fileSize * sizeof(float) * 2);
  reorderInfo.dataFile.reset(new std::ofstream(
      data_filename_,
      std::ofstream::binary | std::ios_base::in | std::ios_base::out));
  if (reorderInfo.dataFile->fail())
    throw std::runtime_error(
        "Error: failed to open temporary data files for writing! Check access "
        "rights and free disk space.");

  preAllocate(flag_filename_, fileSize * sizeof(bool));
  reorderInfo.flagFile.reset(new std::ofstream(
      flag_filename_,
      std::ofstream::binary | std::ios_base::in | std::ios_base::out));
  if (reorderInfo.flagFile->fail())
    throw std::runtime_error(
        "Error: failed to open temporary data files for writing! Check access "
        "rights and free disk space.");

  Logger::Debug << "Reordering data set...\n";

  const size_t bufferMem = std::min<size_t>(
      aocommon::system::TotalMemory() / 10, 1024l * 1024l * 1024l);
  ReorderedFileBuffer dataFile(reorderInfo.dataFile.get(), bufferMem);
  ReorderedFileBuffer flagFile(reorderInfo.flagFile.get(), bufferMem / 8);

  std::vector<std::size_t> writeFilePositions = file_positions_;
  std::vector<std::size_t> timePositions(file_positions_.size(), size_t(-1));
  size_t polarizationCount = Polarizations().size();

  MSSelection msSelection(ms, ObservationTimesPerSequence(), progressListener);

  msSelection.Process(
      [&](size_t rowIndex, size_t sequenceId, size_t timeIndexInSequence) {
        size_t antenna1 = antenna1Column(rowIndex),
               antenna2 = antenna2Column(rowIndex),
               spw = dataIdToSpw[dataDescIdColumn(rowIndex)],
               channelCount = MetaData().FrequencyCount(spw),
               arrayIndex = sequence_index_table_->Value(antenna1, antenna2,
                                                         spw, sequenceId),
               sampleCount = channelCount * polarizationCount;
        size_t& filePos = writeFilePositions[arrayIndex];
        size_t& timePos = timePositions[arrayIndex];

        casacore::Array<casacore::Complex> data = dataColumn(rowIndex);
        casacore::Array<bool> flag = flagColumn(rowIndex);

        dataFile.seekp(filePos * (sizeof(float) * 2));
        flagFile.seekp(filePos * sizeof(bool));

        // If this baseline missed some time steps, pad the files
        // (we can't just skip over, because the flags should be set to true)
        ++timePos;
        while (timePos < timeIndexInSequence) {
          const std::vector<float> nullData(sampleCount * 2, 0.0);
          const std::vector<char> nullFlags(sampleCount, (char)true);
          dataFile.write(reinterpret_cast<const char*>(&*nullData.begin()),
                         sampleCount * 2 * sizeof(float));
          flagFile.write(reinterpret_cast<const char*>(&*nullFlags.begin()),
                         sampleCount * sizeof(bool));
          ++timePos;
          filePos += sampleCount;
        }

        dataFile.write(reinterpret_cast<const char*>(&*data.cbegin()),
                       sampleCount * 2 * sizeof(float));

        flagFile.write(reinterpret_cast<const char*>(&*flag.cbegin()),
                       sampleCount * sizeof(bool));

        filePos += sampleCount;
      });

  const uint64_t dataSetSize =
      (uint64_t)fileSize * (uint64_t)(sizeof(float) * 2 + sizeof(bool));
  Logger::Debug << "Done reordering data set of " << dataSetSize / (1024 * 1024)
                << " MB in " << watch.Seconds() << " s ("
                << (long double)dataSetSize /
                       (1024.0L * 1024.0L * watch.Seconds())
                << " MB/s)\n";
  ms_is_reordered_ = true;
  remove_reordered_files_ = true;
  reordered_data_files_have_changed_ = false;
  reordered_flag_files_have_changed_ = false;
}

void ReorderingBaselineReader::removeTemporaryFiles() {
  if (ms_is_reordered_ && remove_reordered_files_) {
    std::filesystem::remove(meta_filename_);
    std::filesystem::remove(data_filename_);
    std::filesystem::remove(flag_filename_);
    Logger::Debug << "Temporary files removed.\n";
  }
  ms_is_reordered_ = false;
  remove_reordered_files_ = false;
  reordered_data_files_have_changed_ = false;
  reordered_flag_files_have_changed_ = false;
}

void ReorderingBaselineReader::PerformDataWriteTask(
    std::vector<Image2DCPtr> _realImages,
    std::vector<Image2DCPtr> _imaginaryImages, size_t antenna1, size_t antenna2,
    size_t spectralWindow, size_t sequenceId) {
  initializeMeta();

  Logger::Debug
      << "Performing data write task with indirect baseline reader...\n";

  const size_t polarizationCount = Polarizations().size();

  if (_realImages.size() != polarizationCount ||
      _imaginaryImages.size() != polarizationCount)
    throw std::runtime_error(
        "PerformDataWriteTask: input format did not match number of "
        "polarizations in measurement set");

  for (size_t i = 1; i < _realImages.size(); ++i) {
    if (_realImages[0]->Width() != _realImages[i]->Width() ||
        _realImages[0]->Height() != _realImages[i]->Height() ||
        _realImages[0]->Width() != _imaginaryImages[i]->Width() ||
        _realImages[0]->Height() != _imaginaryImages[i]->Height())
      throw std::runtime_error(
          "PerformDataWriteTask: width and/or height of input images did not "
          "match");
  }

  PrepareReadWrite(dummy_progress_);

  const size_t width = _realImages[0]->Width();
  const size_t bufferSize =
      MetaData().FrequencyCount(spectralWindow) * Polarizations().size();

  std::ofstream dataFile(data_filename_, std::ofstream::binary |
                                             std::ios_base::in |
                                             std::ios_base::out);
  const size_t index = sequence_index_table_->Value(antenna1, antenna2,
                                                    spectralWindow, sequenceId);
  const size_t filePos = file_positions_[index];
  dataFile.seekp(filePos * (sizeof(float) * 2), std::ios_base::beg);

  std::vector<float> dataBuffer(bufferSize * 2);
  for (size_t x = 0; x < width; ++x) {
    size_t dataBufferPtr = 0;
    for (size_t f = 0; f < MetaData().FrequencyCount(spectralWindow); ++f) {
      for (size_t p = 0; p < Polarizations().size(); ++p) {
        dataBuffer[dataBufferPtr] = _realImages[p]->Value(x, f);
        ++dataBufferPtr;
        dataBuffer[dataBufferPtr] = _imaginaryImages[p]->Value(x, f);
        ++dataBufferPtr;
      }
    }

    dataFile.write(reinterpret_cast<char*>(&dataBuffer[0]),
                   bufferSize * sizeof(float) * 2);
    if (dataFile.bad())
      throw std::runtime_error(
          "Error: failed to update temporary data files! Check access rights "
          "and free disk space.");
  }

  reordered_data_files_have_changed_ = true;

  Logger::Debug << "Done writing.\n";
}

void ReorderingBaselineReader::performFlagWriteTask(
    std::vector<Mask2DCPtr> flags, unsigned antenna1, unsigned antenna2,
    unsigned spw, unsigned sequenceId) {
  initializeMeta();

  const unsigned polarizationCount = Polarizations().size();

  if (flags.size() != polarizationCount)
    throw std::runtime_error(
        "PerformDataWriteTask: input format did not match number of "
        "polarizations in measurement set");

  for (size_t i = 1; i < flags.size(); ++i) {
    if (flags[0]->Width() != flags[i]->Width() ||
        flags[0]->Height() != flags[i]->Height())
      throw std::runtime_error(
          "PerformDataWriteTask: width and/or height of input images did not "
          "match");
  }

  PrepareReadWrite(dummy_progress_);

  const size_t width = flags[0]->Width();
  const size_t bufferSize =
      MetaData().FrequencyCount(spw) * Polarizations().size();

  std::ofstream flagFile(flag_filename_, std::ofstream::binary |
                                             std::ios_base::in |
                                             std::ios_base::out);
  const size_t index =
      sequence_index_table_->Value(antenna1, antenna2, spw, sequenceId);
  const size_t filePos = file_positions_[index];
  flagFile.seekp(filePos * (sizeof(bool)), std::ios_base::beg);

  const std::unique_ptr<bool[]> flagBuffer(new bool[bufferSize]);
  for (size_t x = 0; x < width; ++x) {
    size_t flagBufferPtr = 0;
    for (size_t f = 0; f < MetaData().FrequencyCount(spw); ++f) {
      for (size_t p = 0; p < polarizationCount; ++p) {
        flagBuffer[flagBufferPtr] = flags[p]->Value(x, f);
        ++flagBufferPtr;
      }
    }

    flagFile.write(reinterpret_cast<char*>(flagBuffer.get()),
                   bufferSize * sizeof(bool));
    if (flagFile.bad())
      throw std::runtime_error(
          "Error: failed to update temporary flag files! Check access rights "
          "and free disk space.");
  }

  reordered_flag_files_have_changed_ = true;
}

template <bool UpdateData, bool UpdateFlags>
void ReorderingBaselineReader::updateOriginalMS(ProgressListener& progress) {
  casacore::MeasurementSet ms = OpenMS();
  if (UpdateData || UpdateFlags) {
    ms.reopenRW();
  }

  const casacore::ScalarColumn<double> timeColumn(ms, "TIME");
  const casacore::ScalarColumn<int> antenna1Column(ms, "ANTENNA1");
  const casacore::ScalarColumn<int> antenna2Column(ms, "ANTENNA2");
  const casacore::ScalarColumn<int> fieldIdColumn(ms, "FIELD_ID");
  const casacore::ScalarColumn<int> dataDescIdColumn(ms, "DATA_DESC_ID");
  casacore::ArrayColumn<bool> flagColumn(ms, "FLAG");
  casacore::ArrayColumn<casacore::Complex> dataColumn(ms, DataColumnName());

  const std::vector<MSMetaData::Sequence> sequences = MetaData().GetSequences();
  std::vector<size_t> dataIdToSpw;
  MetaData().GetDataDescToBandVector(dataIdToSpw);

  const size_t polarizationCount = Polarizations().size();

  Logger::Debug << "Opening updated files\n";
  UpdateInfo updateInfo;

  if (UpdateData) {
    updateInfo.dataFile.reset(
        new std::ifstream(data_filename_, std::ifstream::binary));
    if (updateInfo.dataFile->fail())
      throw std::runtime_error("Failed to open temporary data file");
  }
  if (UpdateFlags) {
    updateInfo.flagFile.reset(
        new std::ifstream(flag_filename_, std::ifstream::binary));
    if (updateInfo.flagFile->fail())
      throw std::runtime_error("Failed to open temporary flag file");
  }

  std::vector<size_t> updatedFilePos = file_positions_;
  std::vector<size_t> timePositions(updatedFilePos.size(), size_t(-1));

  MSSelection msSelection(ms, ObservationTimesPerSequence(), progress);
  msSelection.Process([&](size_t rowIndex, size_t sequenceId,
                          size_t timeIndexInSequence) {
    size_t antenna1 = antenna1Column(rowIndex),
           antenna2 = antenna2Column(rowIndex),
           spw = dataIdToSpw[dataDescIdColumn(rowIndex)],
           channelCount = MetaData().FrequencyCount(spw),
           arrayIndex = sequence_index_table_->Value(antenna1, antenna2, spw,
                                                     sequenceId),
           sampleCount = channelCount * polarizationCount;
    size_t& filePos = updatedFilePos[arrayIndex];
    size_t& timePos = timePositions[arrayIndex];

    const casacore::IPosition shape(2, polarizationCount, channelCount);

    // Skip over samples in the temporary files that are missing in the
    // measurement set
    ++timePos;
    while (timePos < timeIndexInSequence) {
      filePos += sampleCount;
      ++timePos;
    }

    if (UpdateData) {
      casacore::Array<casacore::Complex> data(shape);

      std::ifstream& dataFile = *updateInfo.dataFile;
      dataFile.seekg(filePos * (sizeof(float) * 2), std::ios_base::beg);
      dataFile.read(reinterpret_cast<char*>(&*data.cbegin()),
                    sampleCount * 2 * sizeof(float));
      if (dataFile.fail())
        throw std::runtime_error("Error: failed to read temporary data files!");

      dataColumn.basePut(rowIndex, data);
    }
    if (UpdateFlags) {
      casacore::Array<bool> flagArray(shape);

      std::ifstream& flagFile = *updateInfo.flagFile;
      flagFile.seekg(filePos * sizeof(bool), std::ios_base::beg);
      flagFile.read(reinterpret_cast<char*>(&*flagArray.cbegin()),
                    sampleCount * sizeof(bool));
      if (flagFile.fail())
        throw std::runtime_error("Error: failed to read temporary flag files!");

      flagColumn.basePut(rowIndex, flagArray);
    }

    filePos += sampleCount;
  });

  Logger::Debug << "Freeing the data\n";

  // Close the files
  updateInfo.dataFile.reset();
  updateInfo.flagFile.reset();

  if (UpdateData) Logger::Debug << "Done updating measurement set data\n";
  if (UpdateFlags) Logger::Debug << "Done updating measurement set flags\n";
}

void ReorderingBaselineReader::updateOriginalMSData(
    ProgressListener& progress) {
  Logger::Debug << "Data was changed, need to update the original MS...\n";
  updateOriginalMS<true, false>(progress);
  reordered_data_files_have_changed_ = false;
}

void ReorderingBaselineReader::updateOriginalMSFlags(
    ProgressListener& progress) {
  const Stopwatch watch(true);
  Logger::Debug << "Flags were changed, need to update the original MS...\n";
  updateOriginalMS<false, true>(progress);
  reordered_flag_files_have_changed_ = false;
  Logger::Debug << "Storing flags toke: " << watch.ToString() << '\n';
}
