#include "collector.h"

#include "../structures/msmetadata.h"

#include "../util/progress/progresslistener.h"

#include "histogramcollection.h"
#include "statisticscollection.h"

#include <aocommon/system.h>

#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <memory>

void Collector::Collect(const std::string& filename,
                        StatisticsCollection& statisticsCollection,
                        HistogramCollection& histogramCollection,
                        ProgressListener& progressListener) {
  progressListener.OnStartTask("Collecting statistics");
  std::unique_ptr<MSMetaData> ms(new MSMetaData(filename));
  const size_t polarizationCount = ms->PolarizationCount(),
               bandCount = ms->BandCount();
  const bool ignoreChannelZero =
      ms->IsChannelZeroRubish() && _mode != CollectTimeFrequency;
  const std::string stationName = ms->GetStationName();
  std::vector<BandInfo> bands(bandCount);
  std::vector<std::vector<double>> frequencies(bandCount);
  unsigned totalChannels = 0;
  for (unsigned b = 0; b < bandCount; ++b) {
    bands[b] = ms->GetBandInfo(b);
    frequencies[b].resize(bands[b].channels.size());
    totalChannels += bands[b].channels.size();
    for (unsigned c = 0; c < bands[b].channels.size(); ++c) {
      frequencies[b][c] = bands[b].channels[c].frequencyHz;
    }
  }
  ms.reset();

  // Initialize statisticscollection
  _statisticsCollection = &statisticsCollection;
  _histogramCollection = &histogramCollection;
  statisticsCollection.SetPolarizationCount(polarizationCount);
  size_t nCPU = aocommon::system::ProcessorCount();
  _threadStats.clear();
  for (size_t i = 0; i != nCPU; ++i)
    _threadStats.emplace_back(polarizationCount);
  if (_mode != CollectHistograms) {
    for (unsigned b = 0; b < bandCount; ++b) {
      if (ignoreChannelZero)
        statisticsCollection.InitializeBand(b, (frequencies[b].data() + 1),
                                            bands[b].channels.size() - 1);
      else
        statisticsCollection.InitializeBand(b, frequencies[b].data(),
                                            bands[b].channels.size());
      for (size_t i = 0; i != nCPU; ++i) {
        if (ignoreChannelZero)
          _threadStats[i].InitializeBand(b, (frequencies[b].data() + 1),
                                         bands[b].channels.size() - 1);
        else
          _threadStats[i].InitializeBand(b, frequencies[b].data(),
                                         bands[b].channels.size());
      }
    }
  }
  // Initialize Histograms collection
  histogramCollection.SetPolarizationCount(polarizationCount);

  // get columns
  casacore::Table table(filename);
  casacore::ArrayColumn<casacore::Complex> dataColumn(table, _dataColumnName);
  casacore::ArrayColumn<bool> flagColumn(table, "FLAG");
  casacore::ScalarColumn<double> timeColumn(table, "TIME");
  casacore::ScalarColumn<int> antenna1Column(table, "ANTENNA1"),
      antenna2Column(table, "ANTENNA2"), windowColumn(table, "DATA_DESC_ID");

  size_t channelCount = bands[0].channels.size();
  _correlatorFlags.assign(channelCount, false);
  _correlatorFlagsForBadAntenna.assign(channelCount, true);

  aocommon::Lane<Work> workLane(nCPU * 4);
  std::vector<std::thread> threads;
  for (size_t i = 0; i != nCPU; ++i)
    threads.emplace_back(
        [&, i]() { process(&workLane, &_threadStats[i], polarizationCount); });

  bool hasInterval = !(_intervalStart == 0 && _intervalEnd == 0);

  const size_t nrow = table.nrow();
  size_t timestepIndex = (size_t)-1;
  double prevtime = -1.0;
  for (size_t row = 0; row != nrow; ++row) {
    Work work;
    work.time = timeColumn(row);

    if (work.time != prevtime) {
      ++timestepIndex;
      prevtime = work.time;
    }

    if (!hasInterval ||
        (timestepIndex >= _intervalStart && timestepIndex < _intervalEnd)) {
      work.antenna1Index = antenna1Column(row),
      work.antenna2Index = antenna2Column(row),
      work.bandIndex = windowColumn(row);
      work.timestepIndex = timestepIndex;

      const BandInfo& band = bands[work.bandIndex];

      const casacore::Array<casacore::Complex> dataArray = dataColumn(row);
      const casacore::Array<bool> flagArray = flagColumn(row);
      work.hasFlaggedAntenna =
          _flaggedAntennae.find(work.antenna1Index) != _flaggedAntennae.end() ||
          _flaggedAntennae.find(work.antenna2Index) != _flaggedAntennae.end();

      casacore::Array<casacore::Complex>::const_contiter dataIter =
          dataArray.cbegin();
      casacore::Array<bool>::const_contiter flagIter = flagArray.cbegin();
      const unsigned startChannel = ignoreChannelZero ? 1 : 0;
      if (ignoreChannelZero) {
        dataIter += polarizationCount;
        flagIter += polarizationCount;
      }

      work.nChan = band.channels.size() - startChannel;
      work.isRFI.resize(polarizationCount);
      work.samples.resize(polarizationCount);
      for (unsigned p = 0; p < polarizationCount; ++p) {
        work.isRFI[p].resize(band.channels.size());
        work.samples[p].resize(band.channels.size());
      }
      for (unsigned channel = startChannel; channel < band.channels.size();
           ++channel) {
        for (unsigned p = 0; p < polarizationCount; ++p) {
          work.samples[p][channel - startChannel] = *dataIter;
          work.isRFI[p][channel - startChannel] = *flagIter;

          ++dataIter;
          ++flagIter;
        }
      }

      workLane.write(std::move(work));
    }
    progressListener.OnProgress(row, nrow);
  }

  workLane.write_end();

  for (size_t i = 0; i != nCPU; ++i) threads[i].join();

  progressListener.OnFinish();
}

void Collector::process(aocommon::Lane<Work>* workLane,
                        StatisticsCollection* stats, size_t polarizationCount) {
  Work work;
  HistogramCollection histos(polarizationCount);
  while (workLane->read(work)) {
    for (unsigned p = 0; p < polarizationCount; ++p) {
      switch (_mode) {
        case CollectDefault:
          if (work.hasFlaggedAntenna || work.timestepIndex < _flaggedTimesteps)
            stats->Add(work.antenna1Index, work.antenna2Index, work.time,
                       work.bandIndex, p,
                       &reinterpret_cast<float*>(work.samples[p].data())[0],
                       &reinterpret_cast<float*>(work.samples[p].data())[1],
                       work.isRFI[p].data(),
                       _correlatorFlagsForBadAntenna.data(), work.nChan, 2, 1,
                       1);
          else
            stats->Add(work.antenna1Index, work.antenna2Index, work.time,
                       work.bandIndex, p,
                       &reinterpret_cast<float*>(work.samples[p].data())[0],
                       &reinterpret_cast<float*>(work.samples[p].data())[1],
                       work.isRFI[p].data(), _correlatorFlags.data(),
                       work.nChan, 2, 1, 1);
          break;
        case CollectHistograms:
          histos.Add(work.antenna1Index, work.antenna2Index, p,
                     work.samples[p].data(), work.isRFI[p].data(), work.nChan);
          break;
        case CollectTimeFrequency:
          if (work.hasFlaggedAntenna || work.timestepIndex < _flaggedTimesteps)
            stats->Add(work.antenna1Index, work.antenna2Index, work.time,
                       work.bandIndex, p,
                       &reinterpret_cast<float*>(work.samples[p].data())[0],
                       &reinterpret_cast<float*>(work.samples[p].data())[1],
                       work.isRFI[p].data(),
                       _correlatorFlagsForBadAntenna.data(), work.nChan, 2, 1,
                       1);
          else
            stats->AddToTimeFrequency(
                work.antenna1Index, work.antenna2Index, work.time,
                work.bandIndex, p,
                &reinterpret_cast<float*>(work.samples[p].data())[0],
                &reinterpret_cast<float*>(work.samples[p].data())[1],
                work.isRFI[p].data(), _correlatorFlags.data(), work.nChan, 2, 1,
                1);
          break;
      }
    }
  }
  std::unique_lock<std::mutex> lock(_mutex);
  _statisticsCollection->Add(*stats);
  _histogramCollection->Add(histos);
}
