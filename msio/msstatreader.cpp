#include "msstatreader.h"

#include "multibanddata.h"

#include "../structures/mask2d.h"
#include "../structures/msmetadata.h"
#include "../structures/image2d.h"

#include "../util/progresslistener.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>
#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

using namespace aocommon;

MSStatReader::MSStatReader(const std::string& filename,
                           const std::string& dataColumn)
    : _filename(filename), _dataColumn(dataColumn) {
  casacore::MeasurementSet ms(filename);

  _nBands = ms.spectralWindow().nrow();

  casacore::ScalarColumn<int> fieldIdCol(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::FIELD_ID));
  casacore::ScalarColumn<double> timeCol(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::TIME));

  int curField = -1;
  for (size_t row = 0; row != ms.nrow(); ++row) {
    int field = fieldIdCol(row);
    if (field != curField) {
      _sequenceStart.emplace_back(row);
      curField = field;
    }
  }
  _sequenceStart.emplace_back(ms.nrow());

  _telescopeName = MSMetaData::GetTelescopeName(ms);
}

MSStatReader::Result MSStatReader::readSamples(
    BaselineIntegration::Mode statType, size_t sequenceIndex, size_t bandIndex,
    bool includeAutos, bool includeFlags, bool freqDiff,
    ProgressListener& progress) {
  progress.OnStartTask("Read data & integrate baselines");
  casacore::MeasurementSet ms(_filename);
  casacore::ScalarColumn<int> antenna1Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA1));
  casacore::ScalarColumn<int> antenna2Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA2));
  casacore::ScalarColumn<double> timeCol(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::TIME));
  casacore::ScalarColumn<int> dataDescIdCol(
      ms, casacore::MeasurementSet::columnName(
              casacore::MeasurementSet::DATA_DESC_ID));

  casacore::ArrayColumn<std::complex<float>> dataColumn(ms, _dataColumn);
  casacore::ArrayColumn<bool> flagColumn(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::FLAG));

  casacore::IPosition dataShape = dataColumn.shape(0);
  size_t nPolarizations = dataShape[0], nChannels = dataShape[1],
         nTotal = freqDiff ? (nPolarizations * (nChannels - 1))
                           : nPolarizations * nChannels;
  casacore::Array<std::complex<float>> dataArray(dataShape);
  casacore::Array<bool> flagArray(dataShape);

  internal::MultiBandData bands(ms.spectralWindow(), ms.dataDescription());

  size_t startRow = _sequenceStart[sequenceIndex],
         endRow = _sequenceStart[sequenceIndex + 1];
  double time = timeCol(startRow) - 1;
  std::vector<Statistic> statsData;
  std::vector<double> times;
  size_t dataPos = 0;
  size_t totalProgress = (endRow - startRow) * 104 / 100;
  for (size_t row = startRow; row != endRow; ++row) {
    size_t antenna1 = antenna1Col(row), antenna2 = antenna2Col(row);
    bool baselineSelected = includeAutos || (antenna1 != antenna2);

    if (baselineSelected &&
        bands.GetBandIndex(dataDescIdCol(row)) == bandIndex) {
      if (timeCol(row) != time) {
        dataPos = statsData.size();
        statsData.resize(statsData.size() + nTotal);
        time = timeCol(row);
        times.emplace_back(time);
      }
      dataColumn.get(row, dataArray);
      flagColumn.get(row, flagArray);

      if (freqDiff) {
        for (size_t i = 0; i != nTotal; ++i) {
          bool flag1 = flagArray.cbegin()[i];
          bool flag2 = flagArray.cbegin()[i + nPolarizations];
          if ((!flag1 && !flag2) || includeFlags) {
            std::complex<float> val =
                dataArray.cbegin()[i] - dataArray.cbegin()[i + nPolarizations];
            statsData[dataPos + i].Add(statType, val);
          }
        }
      } else {
        for (size_t i = 0; i != nTotal; ++i) {
          bool flag = flagArray.cbegin()[i];
          if (!flag || includeFlags) {
            std::complex<float> val = dataArray.cbegin()[i];
            statsData[dataPos + i].Add(statType, val);
          }
        }
      }
    }
    progress.OnProgress(row - startRow, totalProgress);
  }

  progress.OnStartTask("Combining statistics");
  Result result =
      makeResult(statType, statsData.data(), nPolarizations,
                 freqDiff ? (nChannels - 1) : nChannels, times.size());
  result.second->SetObservationTimes(times);
  fillBand(result, freqDiff, bands, bandIndex);
  progress.OnFinish();
  return result;
}

MSStatReader::Result MSStatReader::readTimeDiff(
    BaselineIntegration::Mode statType, size_t sequenceIndex, size_t bandIndex,
    bool includeAutos, bool includeFlags, ProgressListener& progress) {
  progress.OnStartTask("Read data & integrate baselines");
  casacore::MeasurementSet ms(_filename);
  casacore::ScalarColumn<int> antenna1Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA1));
  casacore::ScalarColumn<int> antenna2Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA2));
  casacore::ScalarColumn<double> timeCol(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::TIME));
  casacore::ScalarColumn<int> dataDescIdCol(
      ms, casacore::MeasurementSet::columnName(
              casacore::MeasurementSet::DATA_DESC_ID));

  casacore::ArrayColumn<std::complex<float>> dataColumn(ms, _dataColumn);
  casacore::ArrayColumn<bool> flagColumn(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::FLAG));

  casacore::IPosition dataShape = dataColumn.shape(0);
  size_t nPolarizations = dataShape[0], nChannels = dataShape[1],
         nTotal = nPolarizations * nChannels;
  casacore::Array<std::complex<float>> dataArray(dataShape);
  casacore::Array<bool> flagArray(dataShape);

  internal::MultiBandData bands(ms.spectralWindow(), ms.dataDescription());

  size_t startRow = _sequenceStart[sequenceIndex],
         endRow = _sequenceStart[sequenceIndex + 1];
  double time = timeCol(startRow) - 1;
  std::vector<Statistic> statsData;
  std::vector<double> times;
  size_t dataPos = 0;
  size_t totalProgress = (endRow - startRow) * 104 / 100;
  using Baseline = std::pair<size_t, size_t>;
  using TimeData = std::vector<std::pair<std::complex<float>, bool>>;
  std::map<Baseline, TimeData> previousTimeData, currentTimeData;
  for (size_t row = startRow; row != endRow; ++row) {
    size_t antenna1 = antenna1Col(row), antenna2 = antenna2Col(row);
    bool baselineSelected = includeAutos || (antenna1 != antenna2);

    if (baselineSelected &&
        bands.GetBandIndex(dataDescIdCol(row)) == bandIndex) {
      std::pair<size_t, size_t> baseline(antenna1, antenna2);
      if (timeCol(row) != time) {
        time = timeCol(row);
        times.emplace_back(time);

        if (times.size() > 1) {
          dataPos = statsData.size();
          statsData.resize(statsData.size() + nTotal);
          previousTimeData = std::move(currentTimeData);
          currentTimeData.clear();
        }
      }

      dataColumn.get(row, dataArray);
      flagColumn.get(row, flagArray);

      auto& curBaseline = currentTimeData[baseline];
      curBaseline.resize(nTotal);
      for (size_t i = 0; i != nTotal; ++i) {
        bool curFlag = flagArray.cbegin()[i];
        std::complex<float> curVal = dataArray.cbegin()[i];
        curBaseline[i].first = curVal;
        curBaseline[i].second = curFlag;
      }

      auto prevBaselinePtr = previousTimeData.find(baseline);
      if (times.size() > 1 && prevBaselinePtr != previousTimeData.end()) {
        for (size_t i = 0; i != nTotal; ++i) {
          bool curFlag = flagArray.cbegin()[i];
          bool prevFlag = prevBaselinePtr->second[i].second;
          if ((!curFlag && !prevFlag) || includeFlags) {
            std::complex<float> val =
                prevBaselinePtr->second[i].first - dataArray.cbegin()[i];
            statsData[dataPos + i].Add(statType, val);
          }
        }
      }
    }
    progress.OnProgress(row - startRow, totalProgress);
  }

  progress.OnStartTask("Combining statistics");
  Result result = makeResult(statType, statsData.data(), nPolarizations,
                             nChannels, times.size() - 1);
  fillBand(result, false, bands, bandIndex);
  for (size_t i = 0; i != times.size() - 1; ++i)
    times[i] = 0.5 * (times[i] + times[i + 1]);
  times.resize(times.size() - 1);
  result.second->SetObservationTimes(times);
  progress.OnFinish();
  return result;
}

MSStatReader::Result MSStatReader::makeResult(
    BaselineIntegration::Mode statType, const Statistic* statsData,
    size_t nPolarizations, size_t nChannels, size_t nTimes) {
  size_t nTotal = nPolarizations * nChannels;
  std::vector<Image2DPtr> images(nPolarizations);
  std::vector<Mask2DPtr> masks(nPolarizations);
  for (size_t p = 0; p != nPolarizations; ++p) {
    images[p] = Image2D::CreateUnsetImagePtr(nTimes, nChannels);
    masks[p] = Mask2D::CreateUnsetMaskPtr(nTimes, nChannels);
  }
  for (size_t y = 0; y != nChannels; ++y) {
    for (size_t x = 0; x != nTimes; ++x) {
      for (size_t p = 0; p != nPolarizations; ++p) {
        const Statistic& stat = statsData[x * nTotal + y * nPolarizations + p];
        if (stat.count == 0) {
          images[p]->SetValue(x, y, 0.0);
          masks[p]->SetValue(x, y, true);
        } else {
          double val = stat.Calculate(statType);
          images[p]->SetValue(x, y, val);
          masks[p]->SetValue(x, y, false);
        }
      }
    }
  }
  std::vector<PolarizationEnum> pols;
  if (nPolarizations == 4)
    pols = std::vector<PolarizationEnum>{Polarization::XX, Polarization::XY,
                                         Polarization ::YX, Polarization::YY};
  else if (nPolarizations == 2)
    pols = std::vector<PolarizationEnum>{Polarization::XX, Polarization::YY};
  else
    pols.emplace_back(Polarization::StokesI);

  std::vector<TimeFrequencyData> tfs;
  for (size_t p = 0; p != nPolarizations; ++p) {
    tfs.emplace_back(TimeFrequencyData::AmplitudePart, pols[p], images[p]);
    tfs.back().SetGlobalMask(masks[p]);
  }
  Result result;
  if (nPolarizations == 4)
    result.first = TimeFrequencyData::MakeFromPolarizationCombination(
        tfs[0], tfs[1], tfs[2], tfs[3]);
  else if (nPolarizations == 2)
    result.first =
        TimeFrequencyData::MakeFromPolarizationCombination(tfs[0], tfs[1]);
  else
    result.first = std::move(tfs[0]);

  result.second.reset(new TimeFrequencyMetaData());

  return result;
}

void MSStatReader::fillBand(MSStatReader::Result& result, bool freqDiff,
                            const internal::MultiBandData& bands,
                            size_t bandIndex) {
  BandInfo bandInfo;
  bandInfo.windowIndex = 0;
  if (freqDiff) {
    bandInfo.channels.resize(bands[bandIndex].ChannelCount() - 1);
    for (size_t i = 0; i != bands[bandIndex].ChannelCount() - 1; ++i) {
      bandInfo.channels[i].frequencyIndex = i;
      bandInfo.channels[i].frequencyHz =
          0.5 * (bands[bandIndex].Channel(i).Frequency() +
                 bands[bandIndex].Channel(i + 1).Frequency());
      bandInfo.channels[i].effectiveBandWidthHz =
          bands[bandIndex].Channel(i).Width() +
          bands[bandIndex].Channel(i + 1).Width();
    }
  } else {
    bandInfo.channels.resize(bands[bandIndex].ChannelCount());
    for (size_t i = 0; i != bands[bandIndex].ChannelCount(); ++i) {
      bandInfo.channels[i].frequencyIndex = i;
      bandInfo.channels[i].frequencyHz =
          bands[bandIndex].Channel(i).Frequency();
      bandInfo.channels[i].effectiveBandWidthHz =
          bands[bandIndex].Channel(i).Width();
    }
  }
  result.second->SetBand(bandInfo);
}

void MSStatReader::StoreFlags(const std::vector<Mask2DCPtr>& flags,
                              BaselineIntegration::Differencing diffType,
                              size_t sequenceIndex, size_t bandIndex,
                              bool includeAutos) {
  std::vector<Mask2DCPtr> appliedFlags;
  switch (diffType) {
    case BaselineIntegration::NoDifference:
      appliedFlags = flags;
      break;
    case BaselineIntegration::TimeDifference:
      for (size_t i = 0; i != flags.size(); ++i) {
        Mask2DCPtr input = flags[i];
        Mask2DPtr mask =
            Mask2D::CreateUnsetMaskPtr(input->Width() + 1, input->Height());
        for (size_t y = 0; y != input->Height(); ++y) {
          mask->SetValue(0, y, input->Value(0, y));
          for (size_t x = 1; x != input->Width(); ++x) {
            mask->SetValue(x, y, input->Value(x - 1, y) || input->Value(x, y));
          }
          mask->SetValue(input->Width(), y,
                         input->Value(input->Width() - 1, y));
        }
        appliedFlags.emplace_back(std::move(mask));
      }
      break;
    case BaselineIntegration::FrequencyDifference:
      for (size_t i = 0; i != flags.size(); ++i) {
        Mask2DCPtr input = flags[i];
        Mask2DPtr mask =
            Mask2D::CreateUnsetMaskPtr(input->Width(), input->Height() + 1);
        for (size_t x = 0; x != input->Width(); ++x)
          mask->SetValue(x, 0, input->Value(x, 0));
        for (size_t y = 1; y != input->Height(); ++y) {
          for (size_t x = 0; x != input->Width(); ++x) {
            mask->SetValue(x, y, input->Value(x, y - 1) || input->Value(x, y));
          }
        }
        for (size_t x = 0; x != input->Width(); ++x)
          mask->SetValue(x, input->Height(),
                         input->Value(x, input->Height() - 1));
        appliedFlags.emplace_back(std::move(mask));
      }
      break;
  }
  storeFlags(appliedFlags, sequenceIndex, bandIndex, includeAutos);
}

void MSStatReader::storeFlags(const std::vector<Mask2DCPtr>& flags,
                              size_t sequenceIndex, size_t bandIndex,
                              bool includeAutos) {
  casacore::MeasurementSet ms(_filename,
                              casacore::MeasurementSet::TableOption::Update);
  casacore::ScalarColumn<int> antenna1Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA1));
  casacore::ScalarColumn<int> antenna2Col(
      ms,
      casacore::MeasurementSet::columnName(casacore::MeasurementSet::ANTENNA2));
  casacore::ScalarColumn<double> timeCol(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::TIME));
  casacore::ScalarColumn<int> dataDescIdCol(
      ms, casacore::MeasurementSet::columnName(
              casacore::MeasurementSet::DATA_DESC_ID));

  casacore::ArrayColumn<bool> flagColumn(
      ms, casacore::MeasurementSet::columnName(casacore::MeasurementSet::FLAG));

  casacore::IPosition dataShape = flagColumn.shape(0);
  size_t nPolarizations = dataShape[0], nChannels = dataShape[1];
  if (nPolarizations != flags.size())
    throw std::runtime_error(
        "Invalid nr of masks specified in call to MSStatReader::storeFlags()");
  casacore::Array<bool> flagArray(dataShape);

  internal::MultiBandData bands(ms.spectralWindow(), ms.dataDescription());

  size_t startRow = _sequenceStart[sequenceIndex],
         endRow = _sequenceStart[sequenceIndex + 1];
  double time = timeCol(startRow) - 1;
  size_t timeIndex = 0;
  for (size_t row = startRow; row != endRow; ++row) {
    size_t antenna1 = antenna1Col(row), antenna2 = antenna2Col(row);
    bool baselineSelected = includeAutos || (antenna1 != antenna2);

    if (baselineSelected &&
        bands.GetBandIndex(dataDescIdCol(row)) == bandIndex) {
      std::pair<size_t, size_t> baseline(antenna1, antenna2);
      if (timeCol(row) != time) {
        time = timeCol(row);
        ++timeIndex;
      }

      flagColumn.get(row, flagArray);
      auto iter = flagArray.cbegin();
      for (size_t ch = 0; ch != nChannels; ++ch) {
        for (size_t p = 0; p != nPolarizations; ++p) {
          *iter = *iter || flags[p]->Value(timeIndex - 1, ch);
          ++iter;
        }
      }
      flagColumn.put(row, flagArray);
    }
  }
}
