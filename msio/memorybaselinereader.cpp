#include "memorybaselinereader.h"

#include "msselection.h"

#include "../util/logger.h"
#include "../util/progress/dummyprogresslistener.h"
#include "../util/stopwatch.h"

#include <aocommon/system.h>

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <vector>

void MemoryBaselineReader::PrepareReadWrite(ProgressListener& progress) {
  if (!_isRead) {
    progress.OnStartTask("Reading measurement set into memory");
    readSet(progress);
    _isRead = true;
  }
}

void MemoryBaselineReader::PerformReadRequests(ProgressListener& progress) {
  PrepareReadWrite(progress);

  for (size_t i = 0; i != _readRequests.size(); ++i) {
    const ReadRequest& request = _readRequests[i];
    const BaselineID id(request.antenna1, request.antenna2,
                        request.spectralWindow, request.sequenceId);
    const std::map<BaselineID, std::unique_ptr<Result>>::const_iterator
        requestedBaselineIter = _baselines.find(id);
    if (requestedBaselineIter == _baselines.end()) {
      std::ostringstream errorStr;
      errorStr << "Exception in PerformReadRequests(): requested baseline is "
                  "not available in measurement set "
                  "(antenna1="
               << request.antenna1 << ", antenna2=" << request.antenna2
               << ", "
                  "spw="
               << request.spectralWindow
               << ", sequenceId=" << request.sequenceId << ")";
      throw std::runtime_error(errorStr.str());
    } else {
      _results.push_back(*requestedBaselineIter->second);
    }
  }

  _readRequests.clear();
  progress.OnFinish();
}

void MemoryBaselineReader::readSet(ProgressListener& progress) {
  const Stopwatch watch(true);

  initializeMeta();

  const casacore::MeasurementSet table(OpenMS());

  casacore::ScalarColumn<int> ant1Column(
      table,
      casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA1)),
      ant2Column(table, casacore::MeasurementSet::columnName(
                            casacore::MSMainEnums::ANTENNA2)),
      dataDescIdColumn(table, casacore::MeasurementSet::columnName(
                                  casacore::MSMainEnums::DATA_DESC_ID));
  casacore::ArrayColumn<casacore::Complex> dataColumn(table, DataColumnName());
  casacore::ArrayColumn<bool> flagColumn(
      table, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
  casacore::ArrayColumn<double> uvwColumn(
      table, casacore::MeasurementSet::columnName(casacore::MSMainEnums::UVW));

  size_t antennaCount = MetaData().AntennaCount(),
         polarizationCount = Polarizations().size(),
         bandCount = MetaData().BandCount(),
         sequenceCount = MetaData().SequenceCount(), intStart = IntervalStart(),
         intEnd = IntervalEnd();

  std::vector<size_t> dataDescIdToSpw;
  MetaData().GetDataDescToBandVector(dataDescIdToSpw);

  std::vector<BandInfo> bandInfos(bandCount);
  for (size_t b = 0; b != bandCount; ++b)
    bandInfos[b] = MetaData().GetBandInfo(b);

  // Initialize the look-up matrix
  // to quickly access the elements (without the map-lookup)
  typedef std::unique_ptr<Result> MatrixElement;
  typedef std::vector<MatrixElement> MatrixRow;
  typedef std::vector<MatrixRow> BaselineMatrix;
  typedef std::vector<BaselineMatrix> BaselineCube;

  BaselineCube baselineCube(sequenceCount * bandCount);

  for (size_t s = 0; s != sequenceCount; ++s) {
    for (size_t b = 0; b != bandCount; ++b) {
      BaselineMatrix& matrix = baselineCube[s * bandCount + b];
      matrix.resize(antennaCount);

      for (size_t a1 = 0; a1 != antennaCount; ++a1) {
        matrix[a1].resize(antennaCount);
        for (size_t a2 = 0; a2 != antennaCount; ++a2) matrix[a1][a2] = nullptr;
      }
    }
  }

  // The actual reading of the data
  Logger::Debug << "Reading the data (interval={" << intStart << "..." << intEnd
                << "})...\n";

  casacore::Array<casacore::Complex> dataArray;
  casacore::Array<bool> flagArray;

  casacore::MeasurementSet ms(OpenMS());
  MSSelection msSelection(ms, ObservationTimesPerSequence(), progress);
  msSelection.Process([&](size_t rowIndex, size_t sequenceId,
                          size_t timeIndexInSequence) {
    size_t ant1 = ant1Column(rowIndex);
    size_t ant2 = ant2Column(rowIndex);
    const size_t spw = dataDescIdToSpw[dataDescIdColumn(rowIndex)];
    const size_t spwFieldIndex = spw + sequenceId * bandCount;
    if (ant1 > ant2) std::swap(ant1, ant2);
    std::unique_ptr<Result>& result = baselineCube[spwFieldIndex][ant1][ant2];
    if (result == nullptr) {
      const size_t timeStepCount = ObservationTimes(sequenceId).size();
      const size_t nFreq = MetaData().FrequencyCount(spw);
      result.reset(new Result());
      for (size_t p = 0; p != polarizationCount; ++p) {
        result->_realImages.emplace_back(
            Image2D::CreateZeroImagePtr(timeStepCount, nFreq));
        result->_imaginaryImages.emplace_back(
            Image2D::CreateZeroImagePtr(timeStepCount, nFreq));
        result->_flags.emplace_back(
            Mask2D::CreateSetMaskPtr<true>(timeStepCount, nFreq));
      }
      result->_bandInfo = bandInfos[spw];
      result->_uvw.resize(timeStepCount);
    }

    dataArray = dataColumn.get(rowIndex);
    flagArray = flagColumn.get(rowIndex);

    casacore::Array<double> uvwArray = uvwColumn.get(rowIndex);
    casacore::Array<double>::const_contiter uvwPtr = uvwArray.cbegin();
    UVW uvw;
    uvw.u = *uvwPtr;
    ++uvwPtr;
    uvw.v = *uvwPtr;
    ++uvwPtr;
    uvw.w = *uvwPtr;
    result->_uvw[timeIndexInSequence] = uvw;

    for (size_t p = 0; p != polarizationCount; ++p) {
      casacore::Array<casacore::Complex>::const_contiter dataPtr =
          dataArray.cbegin();
      casacore::Array<bool>::const_contiter flagPtr = flagArray.cbegin();

      Image2D& real = *result->_realImages[p];
      Image2D& imag = *result->_imaginaryImages[p];
      Mask2D& mask = *result->_flags[p];
      const size_t imgStride = real.Stride();
      const size_t mskStride = mask.Stride();
      num_t* realOutPtr = real.ValuePtr(timeIndexInSequence, 0);
      num_t* imagOutPtr = imag.ValuePtr(timeIndexInSequence, 0);
      bool* flagOutPtr = mask.ValuePtr(timeIndexInSequence, 0);

      for (size_t i = 0; i != p; ++i) {
        ++dataPtr;
        ++flagPtr;
      }
      const size_t frequencyCount = bandInfos[spw].channels.size();
      for (size_t ch = 0; ch != frequencyCount; ++ch) {
        *realOutPtr = dataPtr->real();
        *imagOutPtr = dataPtr->imag();
        *flagOutPtr = *flagPtr;

        realOutPtr += imgStride;
        imagOutPtr += imgStride;
        flagOutPtr += mskStride;

        for (size_t i = 0; i != polarizationCount; ++i) {
          ++dataPtr;
          ++flagPtr;
        }
      }
    }
  });

  // Move elements from matrix into the baseline map.
  for (size_t s = 0; s != sequenceCount; ++s) {
    for (size_t b = 0; b != bandCount; ++b) {
      const size_t fbIndex = s * bandCount + b;
      for (size_t a1 = 0; a1 != antennaCount; ++a1) {
        for (size_t a2 = a1; a2 != antennaCount; ++a2) {
          std::unique_ptr<Result>& result = baselineCube[fbIndex][a1][a2];
          if (result) {
            _baselines.emplace(BaselineID(a1, a2, b, s), std::move(result));
          }
        }
      }
    }
  }
  _areFlagsChanged = false;

  Logger::Debug << "Reading took " << watch.ToString() << ".\n";
}

void MemoryBaselineReader::PerformFlagWriteRequests() {
  PrepareReadWrite(dummy_progress_);

  for (size_t i = 0; i != _writeRequests.size(); ++i) {
    const FlagWriteRequest& request = _writeRequests[i];
    const BaselineID id(request.antenna1, request.antenna2,
                        request.spectralWindow, request.sequenceId);
    std::unique_ptr<Result>& result = _baselines[id];
    if (result->_flags.size() != request.flags.size())
      throw std::runtime_error("Polarizations do not match");
    for (size_t p = 0; p != result->_flags.size(); ++p)
      result->_flags[p].reset(new Mask2D(*request.flags[p]));
  }
  _areFlagsChanged = true;

  _writeRequests.clear();
}

void MemoryBaselineReader::WriteToMs() {
  casacore::MeasurementSet ms(OpenMS(true));

  casacore::ScalarColumn<int> ant1Column(
      ms,
      casacore::MeasurementSet::columnName(casacore::MSMainEnums::ANTENNA1)),
      ant2Column(ms, casacore::MeasurementSet::columnName(
                         casacore::MSMainEnums::ANTENNA2)),
      dataDescIdColumn(ms, casacore::MeasurementSet::columnName(
                               casacore::MSMainEnums::DATA_DESC_ID));
  casacore::ArrayColumn<bool> flagColumn(
      ms, casacore::MeasurementSet::columnName(casacore::MSMainEnums::FLAG));
  std::vector<size_t> dataIdToSpw;
  MetaData().GetDataDescToBandVector(dataIdToSpw);

  const size_t polarizationCount = Polarizations().size();

  Logger::Debug << "Flags have changed, writing them back to the set...\n";

  DummyProgressListener dummy;
  MSSelection msSelection(ms, ObservationTimesPerSequence(), dummy);
  msSelection.Process([&](size_t rowIndex, size_t sequenceId,
                          size_t timeIndexInSequence) {
    size_t ant1 = ant1Column(rowIndex);
    size_t ant2 = ant2Column(rowIndex);
    const size_t spw = dataIdToSpw[dataDescIdColumn(rowIndex)];
    if (ant1 > ant2) std::swap(ant1, ant2);

    const size_t frequencyCount = MetaData().FrequencyCount(spw);
    casacore::IPosition flagShape = casacore::IPosition(2);
    flagShape[0] = polarizationCount;
    flagShape[1] = frequencyCount;
    casacore::Array<bool> flagArray(flagShape);

    const BaselineID baselineID(ant1, ant2, spw, sequenceId);
    const std::map<BaselineID, std::unique_ptr<Result>>::iterator resultIter =
        _baselines.find(baselineID);
    std::unique_ptr<Result>& result = resultIter->second;

    casacore::Array<bool>::contiter flagPtr = flagArray.cbegin();

    std::vector<Mask2D*> masks(polarizationCount);
    for (size_t p = 0; p != polarizationCount; ++p)
      masks[p] = result->_flags[p].get();

    for (size_t ch = 0; ch != frequencyCount; ++ch) {
      for (size_t p = 0; p != polarizationCount; ++p) {
        *flagPtr = masks[p]->Value(timeIndexInSequence, ch);
        ++flagPtr;
      }
    }

    flagColumn.put(rowIndex, flagArray);
  });

  _areFlagsChanged = false;
}

bool MemoryBaselineReader::IsEnoughMemoryAvailable(uint64_t size) {
  const uint64_t totalMem = aocommon::system::TotalMemory();

  if (size * 2 >= totalMem) {
    Logger::Warn
        << (size / 1000000) << " MB required, but " << (totalMem / 1000000)
        << " MB available.\n"
           "Because this is not at least twice as much, the reordering "
           "mode (slower!) will be used.\n";
    return false;
  } else {
    Logger::Debug << (size / 1000000) << " MB required, "
                  << (totalMem / 1000000)
                  << " MB available: will use memory read mode.\n";
    return true;
  }
}
