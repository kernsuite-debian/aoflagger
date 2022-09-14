#ifndef MS_META_DATA_H
#define MS_META_DATA_H

#include "antennainfo.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class MSMetaData {
 public:
  class Sequence;

  explicit MSMetaData(const std::string& path)
      : _path(path), _isMainTableDataInitialized(false) {
    initializeOtherData();
  }

  ~MSMetaData();

  void SetIntervalStart(size_t index) { _intervalStart = index; }
  void SetIntervalEnd(size_t index) { _intervalEnd = index; }

  size_t FrequencyCount(size_t bandIndex) {
    return _bands[bandIndex].channels.size();
  }

  size_t TimestepCount() {
    InitializeMainTableData();
    return _observationTimes.size();
  }

  size_t TimestepCount(size_t sequenceId) {
    InitializeMainTableData();
    return _observationTimesPerSequence[sequenceId].size();
  }

  size_t PolarizationCount() const { return PolarizationCount(Path()); }

  static size_t PolarizationCount(const std::string& filename);

  size_t AntennaCount() const { return _antennas.size(); }

  size_t BandCount() const { return _bands.size(); }

  size_t FieldCount() const { return _fields.size(); }

  static size_t BandCount(const std::string& filename);

  /**
   * Get number of sequences. A sequence is a contiguous number of scans
   * on the same field. Thus, the next sequence starts as soon as the
   * fieldId changes (possibly to a previous field)
   */
  size_t SequenceCount() {
    InitializeMainTableData();
    return _observationTimesPerSequence.size();
  }

  const AntennaInfo& GetAntennaInfo(unsigned antennaId) const {
    return _antennas[antennaId];
  }

  const BandInfo& GetBandInfo(unsigned bandIndex) const {
    return _bands[bandIndex];
  }

  const FieldInfo& GetFieldInfo(unsigned fieldIndex) const {
    return _fields[fieldIndex];
  }

  void GetDataDescToBandVector(std::vector<size_t>& dataDescToBand);

  std::string Path() const { return _path; }

  /**
   * Does additional initialization of the metadata.
   *
   * Some 'const' versions of the direct getters for the internal vectors
   * require a call to this function. Otherwise they will return empty
   * containers.
   */
  void InitializeMainTableData();

  void GetBaselines(std::vector<std::pair<size_t, size_t>>& baselines) {
    InitializeMainTableData();
    baselines = _baselines;
  }

  /// @pre InitializeMainTableData is called.
  const std::vector<std::pair<size_t, size_t>>& GetBaselines() const {
    return _baselines;
  }

  const std::vector<Sequence>& GetSequences() {
    InitializeMainTableData();
    return _sequences;
  }

  /// @pre InitializeMainTableData is called.
  const std::vector<Sequence>& GetSequences() const { return _sequences; }

  const std::set<double>& GetObservationTimes() {
    InitializeMainTableData();
    return _observationTimes;
  }

  /// @pre InitializeMainTableData is called.
  const std::set<double>& GetObservationTimes() const {
    return _observationTimes;
  }

  const std::set<double>& GetObservationTimesSet(size_t sequenceId) {
    InitializeMainTableData();
    return _observationTimesPerSequence[sequenceId];
  }

  /// @pre InitializeMainTableData is called.
  const std::vector<std::set<double>>& GetObservationTimesPerSequence() const {
    return _observationTimesPerSequence;
  }

  const std::vector<AntennaInfo>& GetAntennas() const { return _antennas; }
  const std::vector<BandInfo>& GetBands() const { return _bands; }
  const std::vector<FieldInfo>& GetFields() const { return _fields; }

  bool HasAOFlaggerHistory();

  void GetAOFlaggerHistory(std::ostream& stream);

  void AddAOFlaggerHistory(const std::string& strategy,
                           const std::string& commandline);

  std::string GetStationName() const;

  bool IsChannelZeroRubish();

  class Sequence {
   public:
    Sequence(unsigned _antenna1, unsigned _antenna2, unsigned _spw,
             unsigned _sequenceId, unsigned _fieldId)
        : antenna1(_antenna1),
          antenna2(_antenna2),
          spw(_spw),
          sequenceId(_sequenceId),
          fieldId(_fieldId) {}

    unsigned antenna1, antenna2;
    unsigned spw;
    unsigned sequenceId;
    unsigned fieldId;

    bool operator<(const Sequence& rhs) const {
      if (antenna1 < rhs.antenna1)
        return true;
      else if (antenna1 == rhs.antenna1) {
        if (antenna2 < rhs.antenna2)
          return true;
        else if (antenna2 == rhs.antenna2) {
          if (spw < rhs.spw)
            return true;
          else if (spw == rhs.spw) {
            return sequenceId < rhs.sequenceId;
          }
        }
      }
      return false;
    }

    bool operator==(const Sequence& rhs) const {
      return antenna1 == rhs.antenna1 && antenna2 == rhs.antenna2 &&
             spw == rhs.spw && sequenceId == rhs.sequenceId;
    }
  };

  static std::string GetTelescopeName(casacore::MeasurementSet& ms);

 private:
  void initializeOtherData();

  void initializeAntennas(casacore::MeasurementSet& ms);
  void initializeBands(casacore::MeasurementSet& ms);
  void initializeFields(casacore::MeasurementSet& ms);

  const std::string _path;

  bool _isMainTableDataInitialized;

  std::vector<std::pair<size_t, size_t>> _baselines;

  std::set<double> _observationTimes;

  std::vector<std::set<double>> _observationTimesPerSequence;

  std::vector<AntennaInfo> _antennas;

  std::vector<BandInfo> _bands;

  std::vector<FieldInfo> _fields;

  std::vector<Sequence> _sequences;

  std::optional<size_t> _intervalStart, _intervalEnd;
};

#endif
