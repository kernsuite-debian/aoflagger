#include "directbaselinereader.h"

#include "msselection.h"

#include "../structures/timefrequencydata.h"

#include "../util/logger.h"
#include "../util/progress/dummyprogresslistener.h"
#include "../util/stopwatch.h"

#include <casacore/tables/DataMan/TiledStManAccessor.h>
#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <vector>
#include <set>
#include <stdexcept>

DirectBaselineReader::DirectBaselineReader(const std::string& msFile)
    : BaselineReader(msFile), _ms(OpenMS()) {}

void DirectBaselineReader::initBaselineCache() {
  // Pass one time through the entire measurement set and store the rownumbers
  // of the baselines.
  Logger::Debug << "Determining sequence positions within file for direct "
                   "baseline reader...\n";
  std::vector<size_t> dataIdToSpw;
  MetaData().GetDataDescToBandVector(dataIdToSpw);

  casacore::ScalarColumn<int> antenna1Column(_ms, "ANTENNA1");
  casacore::ScalarColumn<int> antenna2Column(_ms, "ANTENNA2");
  casacore::ScalarColumn<int> dataDescIdColumn(_ms, "DATA_DESC_ID");

  DummyProgressListener progress;  // TODO
  MSSelection msSelection(_ms, ObservationTimesPerSequence(), progress);
  msSelection.Process(
      [&](size_t rowIndex, size_t sequenceId, size_t /*timeIndexInSequence*/) {
        int antenna1 = antenna1Column(rowIndex),
            antenna2 = antenna2Column(rowIndex),
            dataDescId = dataDescIdColumn(rowIndex);
        int spectralWindow = dataIdToSpw[dataDescId];
        addRowToBaselineCache(antenna1, antenna2, spectralWindow, sequenceId,
                              rowIndex);
      });
}

void DirectBaselineReader::addRowToBaselineCache(size_t antenna1,
                                                 size_t antenna2,
                                                 size_t spectralWindow,
                                                 size_t sequenceId,
                                                 size_t row) {
  BaselineCacheIndex searchItem;
  searchItem.antenna1 = antenna1;
  searchItem.antenna2 = antenna2;
  searchItem.spectralWindow = spectralWindow;
  searchItem.sequenceId = sequenceId;
  std::map<BaselineCacheIndex, BaselineCacheValue>::iterator cacheItemIter =
      _baselineCache.find(searchItem);
  if (cacheItemIter == _baselineCache.end()) {
    BaselineCacheValue cacheValue;
    cacheValue.rows.push_back(row);
    _baselineCache.insert(std::make_pair(searchItem, cacheValue));
  } else {
    cacheItemIter->second.rows.push_back(row);
  }
}

void DirectBaselineReader::addRequestRows(
    ReadRequest request, size_t requestIndex,
    std::vector<std::pair<size_t, size_t>>& rows) {
  BaselineCacheIndex searchItem;
  searchItem.antenna1 = request.antenna1;
  searchItem.antenna2 = request.antenna2;
  searchItem.spectralWindow = request.spectralWindow;
  searchItem.sequenceId = request.sequenceId;
  auto cacheItemIter = _baselineCache.find(searchItem);
  if (cacheItemIter != _baselineCache.end()) {
    const std::vector<size_t>& cacheRows = cacheItemIter->second.rows;
    for (size_t j : cacheRows) rows.emplace_back(j, requestIndex);
  }
}

void DirectBaselineReader::addRequestRows(
    FlagWriteRequest request, size_t requestIndex,
    std::vector<std::pair<size_t, size_t>>& rows) {
  BaselineCacheIndex searchItem;
  searchItem.antenna1 = request.antenna1;
  searchItem.antenna2 = request.antenna2;
  searchItem.spectralWindow = request.spectralWindow;
  searchItem.sequenceId = request.sequenceId;
  auto cacheItemIter = _baselineCache.find(searchItem);
  if (cacheItemIter != _baselineCache.end()) {
    const std::vector<size_t>& cacheRows = cacheItemIter->second.rows;
    for (size_t j : cacheRows) rows.emplace_back(j, requestIndex);
  }
}

void DirectBaselineReader::PerformReadRequests(ProgressListener& progress) {
  progress.OnStartTask("Reading measurement set");
  Stopwatch stopwatch(true);

  initializeMeta();
  if (_baselineCache.empty()) initBaselineCache();

  // Each element contains (row number, corresponding request index)
  std::vector<std::pair<size_t, size_t>> rows;

  for (size_t i = 0; i != _readRequests.size(); ++i)
    addRequestRows(_readRequests[i], i, rows);
  std::sort(rows.begin(), rows.end());

  _results.resize(_readRequests.size());
  for (size_t reqIndex = 0; reqIndex != _readRequests.size(); ++reqIndex) {
    const ReadRequest& request = _readRequests[reqIndex];
    Result& result = _results[reqIndex];
    size_t startIndex = request.startIndex, endIndex = request.endIndex,
           band = request.spectralWindow,
           channelCount = MetaData().FrequencyCount(band);

    size_t width = endIndex - startIndex;
    for (size_t p = 0; p < Polarizations().size(); ++p) {
      if (ReadData()) {
        result._realImages.emplace_back(
            Image2D::CreateZeroImagePtr(width, channelCount));
        result._imaginaryImages.emplace_back(
            Image2D::CreateZeroImagePtr(width, channelCount));
      }
      if (ReadFlags()) {
        // The flags should be initialized to true, as a baseline might
        // miss some time scans that other baselines do have, and these
        // should be flagged.
        result._flags.emplace_back(
            Mask2D::CreateSetMaskPtr<true>(width, channelCount));
      }
    }
    result._uvw.resize(width);
  }

  casacore::ScalarColumn<double> timeColumn(_ms, "TIME");
  casacore::ArrayColumn<float> weightColumn(_ms, "WEIGHT");
  casacore::ArrayColumn<double> uvwColumn(_ms, "UVW");
  casacore::ArrayColumn<bool> flagColumn(_ms, "FLAG");
  std::unique_ptr<casacore::ArrayColumn<casacore::Complex>> dataColumn;

  if (ReadData())
    dataColumn.reset(
        new casacore::ArrayColumn<casacore::Complex>(_ms, DataColumnName()));

  for (size_t i = 0; i != rows.size(); ++i) {
    progress.OnProgress(i, rows.size());
    std::pair<size_t, size_t> p = rows[i];
    size_t rowIndex = p.first;
    size_t requestIndex = p.second;

    double time = timeColumn(rowIndex);
    const ReadRequest& request = _readRequests[requestIndex];
    size_t timeIndex = ObservationTimes(request.sequenceId).find(time)->second,
           startIndex = request.startIndex, endIndex = request.endIndex,
           band = request.spectralWindow;
    bool timeIsSelected = timeIndex >= startIndex && timeIndex < endIndex;
    if (ReadData() && timeIsSelected) {
      // if(BaselineReader::DataKind() == WeightData)
      //	readWeights(requestIndex, timeIndex-startIndex,
      // MetaData().FrequencyCount(band), weightColumn(rowIndex)); else
      readTimeData(requestIndex, timeIndex - startIndex,
                   MetaData().FrequencyCount(band), (*dataColumn)(rowIndex));
    }
    if (ReadFlags() && timeIsSelected) {
      readTimeFlags(requestIndex, timeIndex - startIndex,
                    MetaData().FrequencyCount(band), flagColumn(rowIndex));
    }
    if (timeIsSelected) {
      casacore::Array<double> arr = uvwColumn(rowIndex);
      casacore::Array<double>::const_contiter i = arr.cbegin();
      _results[requestIndex]._uvw[timeIndex - startIndex].u = *i;
      ++i;
      _results[requestIndex]._uvw[timeIndex - startIndex].v = *i;
      ++i;
      _results[requestIndex]._uvw[timeIndex - startIndex].w = *i;
    }
  }

  _readRequests.clear();
  progress.OnFinish();
}

std::vector<UVW> DirectBaselineReader::ReadUVW(unsigned antenna1,
                                               unsigned antenna2,
                                               unsigned spectralWindow,
                                               unsigned sequenceId) {
  Stopwatch stopwatch(true);

  initializeMeta();
  if (_baselineCache.empty()) initBaselineCache();

  const std::map<double, size_t>& observationTimes =
      ObservationTimes(sequenceId);

  // Each element contains (row number, corresponding request index)
  std::vector<std::pair<size_t, size_t>> rows;
  ReadRequest request;
  request.antenna1 = antenna1;
  request.antenna2 = antenna2;
  request.spectralWindow = spectralWindow;
  request.sequenceId = sequenceId;
  request.startIndex = 0;
  request.endIndex = observationTimes.size();
  addRequestRows(request, 0, rows);
  std::sort(rows.begin(), rows.end());

  size_t width = observationTimes.size();

  casacore::ScalarColumn<double> timeColumn(_ms, "TIME");
  casacore::ArrayColumn<double> uvwColumn(_ms, "UVW");

  std::vector<UVW> uvws;
  uvws.resize(width);

  for (std::vector<std::pair<size_t, size_t>>::const_iterator i = rows.begin();
       i != rows.end(); ++i) {
    size_t rowIndex = i->first;

    double time = timeColumn(rowIndex);
    size_t timeIndex = observationTimes.find(time)->second;

    casacore::Array<double> arr = uvwColumn(rowIndex);
    casacore::Array<double>::const_contiter j = arr.cbegin();
    UVW& uvw = uvws[timeIndex];
    uvw.u = *j;
    ++j;
    uvw.v = *j;
    ++j;
    uvw.w = *j;
  }

  Logger::Debug << "Read of UVW took: " << stopwatch.ToString() << '\n';
  return uvws;
}

void DirectBaselineReader::PerformFlagWriteRequests() {
  Stopwatch stopwatch(true);

  initializeMeta();

  if (_baselineCache.empty()) initBaselineCache();

  // Each element contains (row number, corresponding request index)
  std::vector<std::pair<size_t, size_t>> rows;

  for (size_t i = 0; i != _writeRequests.size(); ++i)
    addRequestRows(_writeRequests[i], i, rows);
  std::sort(rows.begin(), rows.end());

  _ms.reopenRW();
  casacore::ScalarColumn<double> timeColumn(_ms, "TIME");
  casacore::ArrayColumn<bool> flagColumn(_ms, "FLAG");

  for (const FlagWriteRequest& request : _writeRequests) {
    size_t band = request.spectralWindow;
    if (MetaData().FrequencyCount(band) != request.flags[0]->Height()) {
      std::cerr << "The frequency count in the measurement set ("
                << MetaData().FrequencyCount(band)
                << ") does not match the image!\n";
    }
    if (request.endIndex - request.startIndex != request.flags[0]->Width()) {
      std::cerr << "The number of time scans to write in the measurement set ("
                << (request.endIndex - request.startIndex)
                << ") does not match the image (" << request.flags[0]->Width()
                << ") !\n";
    }
  }

  size_t rowsWritten = 0;

  for (const std::pair<size_t, size_t>& row : rows) {
    size_t rowIndex = row.first;
    FlagWriteRequest& request = _writeRequests[row.second];
    double time = timeColumn(rowIndex);
    size_t timeIndex = ObservationTimes(request.sequenceId).find(time)->second;
    if (timeIndex >= request.startIndex + request.leftBorder &&
        timeIndex < request.endIndex - request.rightBorder) {
      casacore::Array<bool> flag = flagColumn(rowIndex);
      casacore::Array<bool>::iterator j = flag.begin();
      for (size_t f = 0;
           f < (size_t)MetaData().FrequencyCount(request.spectralWindow); ++f) {
        for (size_t p = 0; p < Polarizations().size(); ++p) {
          *j = request.flags[p]->Value(timeIndex - request.startIndex, f);
          ++j;
        }
      }
      flagColumn.basePut(rowIndex, flag);
      ++rowsWritten;
    }
  }
  _writeRequests.clear();

  Logger::Debug << rowsWritten << "/" << rows.size() << " rows written in "
                << stopwatch.ToString() << '\n';
}

void DirectBaselineReader::readTimeData(
    size_t requestIndex, size_t xOffset, size_t frequencyCount,
    const casacore::Array<casacore::Complex>& data) {
  casacore::Array<casacore::Complex>::const_contiter i = data.cbegin();

  size_t polarizationCount = Polarizations().size();

  for (size_t f = 0; f < (size_t)frequencyCount; ++f) {
    num_t rv, iv;

    for (size_t p = 0; p < polarizationCount; ++p) {
      const casacore::Complex& complex = *i;
      ++i;

      rv = complex.real();
      iv = complex.imag();
      _results[requestIndex]._realImages[p]->SetValue(xOffset, f, rv);
      _results[requestIndex]._imaginaryImages[p]->SetValue(xOffset, f, iv);
    }
  }
}

void DirectBaselineReader::readTimeFlags(size_t requestIndex, size_t xOffset,
                                         size_t frequencyCount,
                                         const casacore::Array<bool>& flag) {
  size_t polarizationCount = Polarizations().size();

  casacore::Array<bool>::const_iterator j = flag.begin();
  for (size_t f = 0; f < (size_t)frequencyCount; ++f) {
    for (size_t p = 0; p < polarizationCount; ++p) {
      bool v = *j;
      ++j;
      _results[requestIndex]._flags[p]->SetValue(xOffset, f, v);
    }
  }
}

void DirectBaselineReader::readWeights(size_t requestIndex, size_t xOffset,
                                       size_t frequencyCount,
                                       const casacore::Array<float>& weight) {
  size_t polarizationCount = Polarizations().size();

  casacore::Array<float>::const_iterator j = weight.begin();
  std::vector<float> values(polarizationCount);
  for (size_t p = 0; p < polarizationCount; ++p) {
    values[p] = *j;
    ++j;
  }
  for (size_t f = 0; f < (size_t)frequencyCount; ++f) {
    for (size_t p = 0; p < polarizationCount; ++p) {
      _results[requestIndex]._realImages[p]->SetValue(xOffset, f, values[p]);
      _results[requestIndex]._imaginaryImages[p]->SetValue(xOffset, f, 0.0);
    }
  }
}
