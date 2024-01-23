#ifndef BASELINEREADER_H
#define BASELINEREADER_H

#include "../structures/antennainfo.h"
#include "../structures/image2d.h"
#include "../structures/mask2d.h"
#include "../structures/msmetadata.h"

#include <aocommon/polarization.h>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

typedef std::shared_ptr<class BaselineReader> BaselineReaderPtr;
typedef std::shared_ptr<const class BaselineReader> BaselineReaderCPtr;

class BaselineReader {
 public:
  explicit BaselineReader(const std::string& msFile);
  virtual ~BaselineReader();

  /**
   * Has the measurement set been modified?
   *
   * When it's been modified the changes need to be written to the measurement
   * set. By default the destructor of the subclasses should execute this
   * operation. In order to allow writing to multiple measurement sets in
   * parallel the functionality is exposed.
   */
  virtual bool IsModified() const = 0;

  /**
   * Writes the changes to the measurement set.
   *
   * @post @c IsModified() == @c false.
   */
  virtual void WriteToMs() = 0;

  /**
   * Prepares the reader before usage.
   *
   * Some readers have a preparation step that can be done in parallel. Calling
   * this function is optional; when not called manually the reader shall
   * execute the preparation itself.
   *
   * @note When no @a progress is needed use the @ref dummy_progress_.
   */
  virtual void PrepareReadWrite(class ProgressListener& progress) = 0;

  static class DummyProgressListener dummy_progress_;

  bool ReadFlags() const { return _readFlags; }
  void SetReadFlags(bool readFlags) { _readFlags = readFlags; }

  bool ReadData() const { return _readData; }
  void SetReadData(bool readData) { _readData = readData; }

  const std::string& DataColumnName() const { return _dataColumnName; }
  void SetDataColumnName(const std::string& name) { _dataColumnName = name; }

  const std::vector<aocommon::PolarizationEnum>& Polarizations() {
    initializePolarizations();
    return _polarizations;
  }

  casacore::MeasurementSet OpenMS(bool writeAccess = false) const {
    if (writeAccess)
      return casacore::MeasurementSet(_msMetaData.Path(),
                                      casacore::TableLock::PermanentLockingWait,
                                      casacore::Table::Update);
    else
      return casacore::MeasurementSet(
          _msMetaData.Path(), casacore::TableLock::PermanentLockingWait);
  }

  MSMetaData& MetaData() { return _msMetaData; }

  const std::map<double, size_t>& ObservationTimes(size_t sequenceId) const {
    return _observationTimes[sequenceId];
  }

  std::vector<double> ObservationTimes(size_t startIndex,
                                       size_t endIndex) const {
    std::vector<double> times;
    times.insert(times.begin(), _observationTimesVector.begin() + startIndex,
                 _observationTimesVector.begin() + endIndex);
    return times;
  }

  void AddReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow,
                      size_t sequenceId);
  void AddReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow,
                      size_t sequenceId, size_t startIndex, size_t endIndex) {
    addReadRequest(antenna1, antenna2, spectralWindow, sequenceId, startIndex,
                   endIndex);
  }
  virtual void PerformReadRequests(class ProgressListener& progress) = 0;

  void AddWriteTask(std::vector<Mask2DCPtr> flags, size_t antenna1,
                    size_t antenna2, size_t spectralWindow, size_t sequenceId) {
    initializePolarizations();
    if (flags.size() != _polarizations.size()) {
      std::stringstream s;
      s << "Trying to write image with " << flags.size()
        << " polarizations to a measurement set with " << _polarizations.size();
      throw std::runtime_error(s.str());
    }
    FlagWriteRequest task;
    task.flags = flags;
    task.antenna1 = antenna1;
    task.antenna2 = antenna2;
    task.spectralWindow = spectralWindow;
    task.sequenceId = sequenceId;
    task.startIndex = 0;
    task.endIndex = flags[0]->Width();
    task.leftBorder = 0;
    task.rightBorder = 0;
    _writeRequests.push_back(task);
  }

  virtual void PerformFlagWriteRequests() = 0;

  virtual void PerformDataWriteTask(std::vector<Image2DCPtr> _realImages,
                                    std::vector<Image2DCPtr> _imaginaryImages,
                                    size_t antenna1, size_t antenna2,
                                    size_t spectralWindow,
                                    size_t sequenceId) = 0;

  TimeFrequencyData GetNextResult(std::vector<UVW>& uvw);

  virtual size_t GetMinRecommendedBufferSize(size_t threadCount) {
    return threadCount;
  }

  virtual size_t GetMaxRecommendedBufferSize(size_t threadCount) {
    return 2 * threadCount;
  }

  static uint64_t MeasurementSetDataSize(const std::string& filename);

  /**
   * Returns an estimate of the size of the measurement set.
   *
   * This estimate can be used to see whether the memory reader can be used.
   *
   * The \a start and \a end are an optional, this allows using
   * \ref Options::startTimestep and \ref Options::endTimestep in this function
   * call.
   */
  static uint64_t MeasurementSetIntervalDataSize(const string& filename,
                                                 std::optional<size_t> start,
                                                 std::optional<size_t> end);

  void SetInterval(std::optional<size_t> start, std::optional<size_t> end) {
    _intervalStart = start;
    _intervalEnd = end;
    if (_intervalStart) _msMetaData.SetIntervalStart(IntervalStart());
    if (_intervalEnd) _msMetaData.SetIntervalEnd(IntervalEnd());
  }

  bool HasIntervalStart() const { return (bool)_intervalStart; }
  bool HasIntervalEnd() const { return (bool)_intervalEnd; }

  size_t IntervalStart() const {
    if (HasIntervalStart())
      return *_intervalStart;
    else
      return 0;
  }
  size_t IntervalEnd() const {
    if (HasIntervalEnd())
      return *_intervalEnd;
    else
      return _observationTimesVector.size();
  }

 protected:
  struct ReadRequest {
    int antenna1;
    int antenna2;
    int spectralWindow;
    unsigned sequenceId;
    size_t startIndex;
    size_t endIndex;
  };

  struct FlagWriteRequest {
    FlagWriteRequest() = default;
    FlagWriteRequest(const FlagWriteRequest& source)
        : flags(source.flags),
          antenna1(source.antenna1),
          antenna2(source.antenna2),
          spectralWindow(source.spectralWindow),
          sequenceId(source.sequenceId),
          startIndex(source.startIndex),
          endIndex(source.endIndex),
          leftBorder(source.leftBorder),
          rightBorder(source.rightBorder) {}
    std::vector<Mask2DCPtr> flags;
    int antenna1;
    int antenna2;
    int spectralWindow;
    unsigned sequenceId;
    size_t startIndex;
    size_t endIndex;
    size_t leftBorder;
    size_t rightBorder;
  };

  struct Result {
    Result() = default;
    Result(const Result& source)
        : _realImages(source._realImages),
          _imaginaryImages(source._imaginaryImages),
          _flags(source._flags),
          _uvw(source._uvw),
          _bandInfo(source._bandInfo) {}
    std::vector<Image2DPtr> _realImages;
    std::vector<Image2DPtr> _imaginaryImages;
    std::vector<Mask2DPtr> _flags;
    std::vector<UVW> _uvw;
    BandInfo _bandInfo;
  };

  void initializeMeta() {
    initObservationTimes();
    initializePolarizations();
  }

  const std::vector<std::map<double, size_t>>& ObservationTimesPerSequence()
      const {
    return _observationTimes;
  }

  std::vector<ReadRequest> _readRequests;
  std::vector<FlagWriteRequest> _writeRequests;
  std::vector<Result> _results;

 private:
  BaselineReader(const BaselineReader&) = delete;
  BaselineReader& operator=(const BaselineReader&) = delete;

  void initializePolarizations();
  void initObservationTimes();

  void addReadRequest(size_t antenna1, size_t antenna2, size_t spectralWindow,
                      size_t sequenceId, size_t startIndex, size_t endIndex) {
    ReadRequest request;
    request.antenna1 = antenna1;
    request.antenna2 = antenna2;
    request.spectralWindow = spectralWindow;
    request.sequenceId = sequenceId;
    request.startIndex = startIndex;
    request.endIndex = endIndex;
    _readRequests.push_back(request);
  }

  MSMetaData _msMetaData;

  std::string _dataColumnName;
  bool _readData, _readFlags;

  std::vector<std::map<double, size_t>> _observationTimes;
  std::vector<double> _observationTimesVector;
  std::vector<aocommon::PolarizationEnum> _polarizations;
  std::optional<size_t> _intervalStart, _intervalEnd;
};

#endif  // BASELINEREADER_H
