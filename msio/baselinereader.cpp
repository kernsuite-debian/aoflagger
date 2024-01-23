#include "baselinereader.h"

#include <set>
#include <stdexcept>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <casacore/tables/DataMan/IncrStManAccessor.h>
#include <casacore/tables/DataMan/StandardStManAccessor.h>
#include <casacore/tables/DataMan/TiledStManAccessor.h>
#include <casacore/tables/Tables/TableIter.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>
#include <casacore/tables/TaQL/ExprNode.h>

#include "../util/progress/dummyprogresslistener.h"
#include "../structures/timefrequencydata.h"

#include "../util/logger.h"

DummyProgressListener BaselineReader::dummy_progress_;

BaselineReader::BaselineReader(const std::string& msFile)
    : _msMetaData(msFile),
      _dataColumnName("DATA"),
      _readData(true),
      _readFlags(true),
      _polarizations() {}

BaselineReader::~BaselineReader() {}

void BaselineReader::initObservationTimes() {
  if (_observationTimes.empty()) {
    Logger::Debug << "Initializing observation times...\n";
    const size_t sequenceCount = _msMetaData.SequenceCount();
    _observationTimes.resize(sequenceCount);
    for (size_t sequenceId = 0; sequenceId != sequenceCount; ++sequenceId) {
      const std::set<double>& times =
          _msMetaData.GetObservationTimesSet(sequenceId);
      unsigned index = 0;
      for (const double t : times) {
        _observationTimes[sequenceId].emplace(t, index);
        _observationTimesVector.push_back(t);
        ++index;
      }
    }
  }
}

void BaselineReader::AddReadRequest(size_t antenna1, size_t antenna2,
                                    size_t spectralWindow, size_t sequenceId) {
  initObservationTimes();

  addReadRequest(antenna1, antenna2, spectralWindow, sequenceId, 0,
                 _observationTimes[sequenceId].size());
}

TimeFrequencyData BaselineReader::GetNextResult(std::vector<class UVW>& uvw) {
  const size_t requestIndex = 0;
  TimeFrequencyData data;
  data = TimeFrequencyData(_polarizations.data(), _polarizations.size(),
                           _results[requestIndex]._realImages.data(),
                           _results[requestIndex]._imaginaryImages.data());
  data.SetIndividualPolarizationMasks(_results[requestIndex]._flags.data());
  uvw = _results[0]._uvw;

  _results.erase(_results.begin() + requestIndex);

  return data;
}

void BaselineReader::initializePolarizations() {
  if (_polarizations.empty()) {
    casacore::MeasurementSet ms(_msMetaData.Path());

    const casacore::MSDataDescription ddTable = ms.dataDescription();
    if (ddTable.nrow() == 0)
      throw std::runtime_error("DataDescription table is empty");
    const casacore::ScalarColumn<int> polIdColumn(
        ddTable, casacore::MSDataDescription::columnName(
                     casacore::MSDataDescription::POLARIZATION_ID));
    const int polarizationId = polIdColumn(0);
    for (size_t row = 0; row != ddTable.nrow(); ++row) {
      if (polIdColumn(row) != polarizationId)
        throw std::runtime_error(
            "This measurement set has different polarizations listed in the "
            "datadescription table. This is non-standard, and AOFlagger cannot "
            "handle it.");
    }

    const casacore::Table polTable = ms.polarization();
    const casacore::ArrayColumn<int> corTypeColumn(polTable, "CORR_TYPE");
    casacore::Array<int> corType = corTypeColumn(polarizationId);
    const casacore::Array<int>::iterator iterend(corType.end());
    for (casacore::Array<int>::iterator iter = corType.begin(); iter != iterend;
         ++iter) {
      const aocommon::PolarizationEnum polarization =
          aocommon::Polarization::AipsIndexToEnum(*iter);
      _polarizations.push_back(polarization);
    }
  }
}

/**
 * Returns an estimate of the size of the data of the MS.
 *
 * One thing that might be problematic, is that this function isn't aware of
 * different spectral windows ("SPW") or different scans. Unlike Dp3, AOFlagger
 * is used also for telescopes like JVLA that don't necessarily store all times
 * together. The readers do support it; the unit of a single contiguous stream
 * of data is called a sequence there. In rare occassions, the timesteps might
 * not divide the measurement set "linearly" in those cases (e.g. the scan may
 * use 10 Spws, whereas the second one uses 1, and both might have the same nr
 * of timesteps -- in which case the second one would take only 1/11th of the
 * obs). This is a bit more rare though, and it is after all only an estimate,
 * so maybe we can live with this until we get a bug report :).
 */
static uint64_t GetMeasurementSetDataSize(casacore::MeasurementSet& ms) {
  const casacore::MSSpectralWindow spwTable = ms.spectralWindow();

  const casacore::ScalarColumn<int> numChanCol(
      spwTable, casacore::MSSpectralWindow::columnName(
                    casacore::MSSpectralWindowEnums::NUM_CHAN));
  const size_t channelCount = numChanCol.get(0);
  if (channelCount == 0) throw std::runtime_error("No channels in set");
  if (ms.nrow() == 0) throw std::runtime_error("Table has no rows (no data)");

  typedef float num_t;
  typedef std::complex<num_t> complex_t;
  const casacore::ScalarColumn<int> ant1Column(
      ms, ms.columnName(casacore::MSMainEnums::ANTENNA1));
  const casacore::ScalarColumn<int> ant2Column(
      ms, ms.columnName(casacore::MSMainEnums::ANTENNA2));
  const casacore::ArrayColumn<complex_t> dataColumn(
      ms, ms.columnName(casacore::MSMainEnums::DATA));

  casacore::IPosition dataShape = dataColumn.shape(0);
  const unsigned polarizationCount = dataShape[0];

  return (uint64_t)polarizationCount * (uint64_t)channelCount *
         (uint64_t)ms.nrow() * (uint64_t)(sizeof(num_t) * 2 + sizeof(bool));
}

uint64_t BaselineReader::MeasurementSetDataSize(const string& filename) {
  casacore::MeasurementSet ms(filename);

  return GetMeasurementSetDataSize(ms);
}

static size_t GetNTimeSteps(casacore::MeasurementSet& ms) {
  size_t result = 0;
  double time = -1.0;
  const casacore::ScalarColumn<double> time_column{
      ms, ms.columnName(casacore::MSMainEnums::TIME)};

  for (size_t i = 0; i != ms.nrow(); ++i)
    if (const double t = time_column(i); t != time) {
      ++result;
      time = t;
    }

  return result;
}

static uint64_t GetMeasurementSetIntervalDataSize(const string& filename,
                                                  size_t start, size_t end) {
  assert(start <= end && "Reqested begin and end aren't a valid range");
  casacore::MeasurementSet ms(filename);

  const uint64_t result = GetMeasurementSetDataSize(ms);
  const size_t time_steps_ms = GetNTimeSteps(ms);
  const size_t time_steps_requested = end - start;

  // When more time steps are requested than available use the available number.
  // Validate time_steps_ms to avoid a division by zero.
  if (time_steps_requested >= time_steps_ms || time_steps_ms == 0)
    return result;
  return result * static_cast<double>(time_steps_requested) / time_steps_ms;
}

uint64_t BaselineReader::MeasurementSetIntervalDataSize(
    const string& filename, std::optional<size_t> start,
    std::optional<size_t> end) {
  if (start) {
    assert(end && "The engagement status of start and end should be the same.");
    return GetMeasurementSetIntervalDataSize(filename, *start, *end);
  }

  return BaselineReader::MeasurementSetDataSize(filename);
}
