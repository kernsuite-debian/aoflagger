#include "operations.h"

#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/TableCopy.h>

#include "collector.h"
#include "combine.h"
#include "defaultstatistics.h"
#include "histogramcollection.h"
#include "qualitytablesformatter.h"
#include "statisticscollection.h"
#include "statisticsderivator.h"

#include "../util/progress/stdoutreporter.h"

#include "../structures/msmetadata.h"

namespace quality {

namespace {

void PrintStatistics(std::complex<long double>* complexStat, unsigned count) {
  if (count != 1) std::cout << '[';
  if (count > 0)
    std::cout << complexStat[0].real() << " + " << complexStat[0].imag() << 'i';
  for (unsigned p = 1; p < count; ++p) {
    std::cout << ", " << complexStat[p].real() << " + " << complexStat[p].imag()
              << 'i';
  }
  if (count != 1) std::cout << ']';
}

void PrintStatistics(unsigned long* stat, unsigned count) {
  if (count != 1) std::cout << '[';
  if (count > 0) std::cout << stat[0];
  for (unsigned p = 1; p < count; ++p) {
    std::cout << ", " << stat[p];
  }
  if (count != 1) std::cout << ']';
}

void PrintStatistics(const DefaultStatistics& statistics) {
  std::cout << "Count=";
  PrintStatistics(statistics.count, statistics.PolarizationCount());
  std::cout << "\nSum=";
  PrintStatistics(statistics.sum, statistics.PolarizationCount());
  std::cout << "\nSumP2=";
  PrintStatistics(statistics.sumP2, statistics.PolarizationCount());
  std::cout << "\nDCount=";
  PrintStatistics(statistics.dCount, statistics.PolarizationCount());
  std::cout << "\nDSum=";
  PrintStatistics(statistics.dSum, statistics.PolarizationCount());
  std::cout << "\nDSumP2=";
  PrintStatistics(statistics.dSumP2, statistics.PolarizationCount());
  std::cout << "\nRFICount=";
  PrintStatistics(statistics.rfiCount, statistics.PolarizationCount());
  std::cout << '\n';
}

void PrintRFISlopeForHistogram(const std::map<HistogramCollection::AntennaPair,
                                              LogHistogram*>& histogramMap,
                               char polarizationSymbol,
                               const AntennaInfo* antennae) {
  for (std::map<HistogramCollection::AntennaPair, LogHistogram*>::const_iterator
           i = histogramMap.begin();
       i != histogramMap.end(); ++i) {
    const unsigned a1 = i->first.first, a2 = i->first.second;
    const Baseline baseline(antennae[a1], antennae[a2]);
    const double length = baseline.Distance();
    const LogHistogram& histogram = *i->second;
    double start, end;
    histogram.GetRFIRegion(start, end);
    const double slope = histogram.NormalizedSlope(start, end);
    const double stddev = histogram.NormalizedSlopeStdError(start, end, slope);
    std::cout << polarizationSymbol << '\t' << a1 << '\t' << a2 << '\t'
              << length << '\t' << slope << '\t' << stddev << '\n';
  }
}

}  // namespace

void ListStatistics() {
  for (int i = 0; i != QualityTablesFormatter::EndPlaceHolderStatistic; ++i) {
    const QualityTablesFormatter::StatisticKind kind =
        (QualityTablesFormatter::StatisticKind)i;
    std::cout << QualityTablesFormatter::KindToName(kind) << '\n';
  }
}

void CollectStatistics(const std::string& filename,
                       Collector::CollectingMode mode, size_t flaggedTimesteps,
                       std::set<size_t>&& flaggedAntennae,
                       const char* dataColumnName, size_t intervalStart,
                       size_t intervalEnd) {
  StatisticsCollection statisticsCollection;
  HistogramCollection histogramCollection;

  Collector collector;
  collector.SetDataColumn(dataColumnName);
  collector.SetInterval(intervalStart, intervalEnd);
  collector.SetMode(mode);
  collector.SetFlaggedAntennae(std::move(flaggedAntennae));
  collector.SetFlaggedTimesteps(flaggedTimesteps);
  StdOutReporter reporter;
  collector.Collect(filename, statisticsCollection, histogramCollection,
                    reporter);

  switch (mode) {
    case Collector::CollectDefault:
    case Collector::CollectTimeFrequency: {
      std::cout << "Writing quality tables..." << std::endl;

      QualityTablesFormatter qualityData(filename);
      statisticsCollection.Save(qualityData);
    } break;
    case Collector::CollectHistograms: {
      std::cout << "Writing histogram tables..." << std::endl;

      HistogramTablesFormatter histograms(filename);
      histogramCollection.Save(histograms);
    } break;
  }

  std::cout << "Done.\n";
}

void CollectHistograms(const std::string& filename,
                       HistogramCollection& histogramCollection,
                       size_t flaggedTimesteps,
                       std::set<size_t>&& flaggedAntennae,
                       const char* dataColumnName) {
  std::cout << "Collecting statistics...\n";

  StatisticsCollection tempCollection;
  Collector collector;
  collector.SetDataColumn(dataColumnName);
  collector.SetMode(Collector::CollectHistograms);
  collector.SetFlaggedAntennae(std::move(flaggedAntennae));
  collector.SetFlaggedTimesteps(flaggedTimesteps);
  StdOutReporter reporter;
  collector.Collect(filename, tempCollection, histogramCollection, reporter);
}

void PrintGlobalStatistic(const std::string& kindName,
                          const std::vector<std::string>& filenames) {
  const MSMetaData ms(filenames.front());
  BandInfo band;
  if (ms.BandCount() != 0) band = ms.GetBandInfo(0);

  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  const quality::FileContents contents =
      quality::ReadAndCombine(filenames, false);
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  DefaultStatistics statistics(n_polarizations);
  contents.statistics_collection.GetGlobalCrossBaselineStatistics(statistics);
  const StatisticsDerivator derivator(contents.statistics_collection);
  const DefaultStatistics singlePol(statistics.ToSinglePolarization());

  double start, end;
  if (band.channels.empty()) {
    start = 0.0;
    end = 0.0;
  } else {
    start = band.channels.begin()->frequencyHz;
    end = band.channels.rbegin()->frequencyHz;
  }
  std::cout << round(start / 10000.0) / 100.0 << '\t'
            << round(end / 10000.0) / 100.0 << '\t'
            << derivator.GetStatisticAmplitude(kind, singlePol, 0);
  for (unsigned p = 0; p < n_polarizations; ++p) {
    const long double val =
        derivator.GetStatisticAmplitude(kind, statistics, p);
    std::cout << '\t' << val;
  }
  std::cout << '\n';
}

void PrintFrequencyRangeStatistic(const std::string& kindName,
                                  const std::vector<std::string>& filenames,
                                  double startFreqMHz, double endFreqMHz) {
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  const quality::FileContents contents =
      quality::ReadAndCombine(filenames, false);
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  DefaultStatistics statistics(n_polarizations);
  contents.statistics_collection.GetFrequencyRangeStatistics(
      statistics, startFreqMHz * 1e6, endFreqMHz * 1e6);
  const StatisticsDerivator derivator(contents.statistics_collection);
  const DefaultStatistics singlePol(statistics.ToSinglePolarization());

  std::cout << startFreqMHz << '\t' << endFreqMHz << '\t'
            << derivator.GetStatisticAmplitude(kind, singlePol, 0);
  for (unsigned p = 0; p < n_polarizations; ++p) {
    const long double val =
        derivator.GetStatisticAmplitude(kind, statistics, p);
    std::cout << '\t' << val;
  }
  std::cout << '\n';
}

void PrintPerBaselineStatistics(const std::string& kindName,
                                const std::vector<std::string>& filenames) {
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  quality::FileContents contents = quality::ReadAndCombine(filenames, false);
  contents.statistics_collection.IntegrateBaselinesToOneChannel();
  const std::vector<std::pair<unsigned, unsigned>>& baselines =
      contents.statistics_collection.BaselineStatistics().BaselineList();
  const StatisticsDerivator derivator(contents.statistics_collection);

  std::cout << "ANTENNA1\tANTENNA2";
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  for (unsigned p = 0; p < n_polarizations; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (std::vector<std::pair<unsigned, unsigned>>::const_iterator i =
           baselines.begin();
       i != baselines.end(); ++i) {
    const unsigned antenna1 = i->first, antenna2 = i->second;
    std::cout << antenna1 << '\t' << antenna2;
    for (unsigned p = 0; p < n_polarizations; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexBaselineStatistic(kind, antenna1, antenna2, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void PrintPerFrequencyStatistics(const std::string& kindName,
                                 const std::vector<std::string>& filenames,
                                 std::optional<size_t> downsample) {
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  quality::FileContents contents = quality::ReadAndCombine(filenames, false);
  if (downsample)
    contents.statistics_collection.LowerFrequencyResolution(*downsample);
  const std::map<double, DefaultStatistics>& freqStats =
      contents.statistics_collection.FrequencyStatistics();
  const StatisticsDerivator derivator(contents.statistics_collection);

  std::cout << "FREQUENCY";
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  for (size_t p = 0; p < n_polarizations; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (const std::pair<const double, DefaultStatistics>& freqStat : freqStats) {
    const double frequency = freqStat.first;
    std::cout << frequency * 1e-6;
    for (unsigned p = 0; p < n_polarizations; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexStatistic(kind, freqStat.second, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void PrintPerTimeStatistics(const std::string& kindName,
                            const std::vector<std::string>& filenames) {
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  quality::FileContents contents = quality::ReadAndCombine(filenames, false);
  contents.statistics_collection.IntegrateTimeToOneChannel();
  const std::map<double, DefaultStatistics>& timeStats =
      contents.statistics_collection.TimeStatistics();
  const StatisticsDerivator derivator(contents.statistics_collection);

  std::cout << "TIME";
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  for (unsigned p = 0; p < n_polarizations; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (std::map<double, DefaultStatistics>::const_iterator i =
           timeStats.begin();
       i != timeStats.end(); ++i) {
    const double time = i->first;
    std::cout << time;
    for (unsigned p = 0; p < n_polarizations; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexStatistic(kind, i->second, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void PrintPerAntennaStatistics(const std::string& kindName,
                               const std::vector<std::string>& filenames) {
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  quality::FileContents contents = quality::ReadAndCombine(filenames, false);
  contents.statistics_collection.IntegrateBaselinesToOneChannel();
  const std::map<size_t, DefaultStatistics> stats =
      contents.statistics_collection.GetAntennaStatistics();
  const StatisticsDerivator derivator(contents.statistics_collection);

  std::cout << "ANTENNA";
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  for (unsigned p = 0; p < n_polarizations; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (const std::pair<const size_t, DefaultStatistics>& s : stats) {
    const size_t antenna = s.first;
    std::cout << antenna;
    for (unsigned p = 0; p < n_polarizations; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexStatistic(kind, s.second, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void PrintSummary(const std::vector<std::string>& filenames) {
  const quality::FileContents contents =
      quality::ReadAndCombine(filenames, false);
  const unsigned n_polarizations =
      contents.statistics_collection.PolarizationCount();
  DefaultStatistics statistics(n_polarizations);

  contents.statistics_collection.GetGlobalTimeStatistics(statistics);
  std::cout << "Time statistics: \n";
  PrintStatistics(statistics);

  contents.statistics_collection.GetGlobalFrequencyStatistics(statistics);
  std::cout << "\nFrequency statistics: \n";
  PrintStatistics(statistics);

  contents.statistics_collection.GetGlobalCrossBaselineStatistics(statistics);
  std::cout << "\nCross-correlated baseline statistics: \n";
  PrintStatistics(statistics);

  const DefaultStatistics singlePolStat = statistics.ToSinglePolarization();
  std::cout << "RFIPercentange: "
            << StatisticsDerivator::GetStatisticAmplitude(
                   QualityTablesFormatter::RFIPercentageStatistic,
                   singlePolStat, 0)
            << '\n';

  contents.statistics_collection.GetGlobalAutoBaselineStatistics(statistics);
  std::cout << "\nAuto-correlated baseline: \n";
  PrintStatistics(statistics);
}

void PrintRfiSummary(const std::vector<std::string>& filenames) {
  const MSMetaData ms(filenames.front());
  const BandInfo band = ms.GetBandInfo(0);

  const quality::FileContents contents =
      quality::ReadAndCombine(filenames, false);
  DefaultStatistics statistics(
      contents.statistics_collection.PolarizationCount());
  contents.statistics_collection.GetGlobalCrossBaselineStatistics(statistics);
  const DefaultStatistics singlePolStat = statistics.ToSinglePolarization();

  double startTime =
             contents.statistics_collection.TimeStatistics().begin()->first,
         endTime =
             contents.statistics_collection.TimeStatistics().rbegin()->first,
         startFreq = band.channels.begin()->frequencyHz,
         endFreq = band.channels.rbegin()->frequencyHz;
  std::cout.precision(16);
  std::cout << startTime << '\t' << endTime << '\t'
            << round(startFreq / 10000.0) / 100.0 << '\t'
            << round(endFreq / 10000.0) / 100.0 << '\t'
            << StatisticsDerivator::GetStatisticAmplitude(
                   QualityTablesFormatter::RFIPercentageStatistic,
                   singlePolStat, 0)
            << '\n';
}

void WriteAntennae(casacore::MeasurementSet& ms,
                   const std::vector<AntennaInfo>& antennae) {
  casacore::MSAntenna antTable = ms.antenna();
  casacore::ScalarColumn<casacore::String> nameCol =
      casacore::ScalarColumn<casacore::String>(
          antTable, antTable.columnName(casacore::MSAntennaEnums::NAME));
  casacore::ScalarColumn<casacore::String> stationCol =
      casacore::ScalarColumn<casacore::String>(
          antTable, antTable.columnName(casacore::MSAntennaEnums::STATION));
  casacore::ScalarColumn<casacore::String> typeCol =
      casacore::ScalarColumn<casacore::String>(
          antTable, antTable.columnName(casacore::MSAntennaEnums::TYPE));
  casacore::ScalarColumn<casacore::String> mountCol =
      casacore::ScalarColumn<casacore::String>(
          antTable, antTable.columnName(casacore::MSAntennaEnums::MOUNT));
  casacore::ArrayColumn<double> positionCol = casacore::ArrayColumn<double>(
      antTable, antTable.columnName(casacore::MSAntennaEnums::POSITION));
  casacore::ScalarColumn<double> dishDiameterCol =
      casacore::ScalarColumn<double>(
          antTable,
          antTable.columnName(casacore::MSAntennaEnums::DISH_DIAMETER));

  size_t rowIndex = antTable.nrow();
  antTable.addRow(antennae.size());
  for (std::vector<AntennaInfo>::const_iterator antPtr = antennae.begin();
       antPtr != antennae.end(); ++antPtr) {
    const AntennaInfo& ant = *antPtr;
    nameCol.put(rowIndex, ant.name);
    stationCol.put(rowIndex, ant.station);
    typeCol.put(rowIndex, "");
    mountCol.put(rowIndex, ant.mount);
    casacore::Vector<double> posArr(3);
    posArr[0] = ant.position.x;
    posArr[1] = ant.position.y;
    posArr[2] = ant.position.z;
    positionCol.put(rowIndex, posArr);
    dishDiameterCol.put(rowIndex, ant.diameter);
    ++rowIndex;
  }
}

void WritePolarizationForLinearPols(casacore::MeasurementSet& ms,
                                    bool flagRow = false) {
  casacore::MSPolarization polTable = ms.polarization();
  casacore::ScalarColumn<int> numCorrCol = casacore::ScalarColumn<int>(
      polTable, polTable.columnName(casacore::MSPolarizationEnums::NUM_CORR));
  casacore::ArrayColumn<int> corrTypeCol = casacore::ArrayColumn<int>(
      polTable, polTable.columnName(casacore::MSPolarizationEnums::CORR_TYPE));
  casacore::ArrayColumn<int> corrProductCol = casacore::ArrayColumn<int>(
      polTable,
      polTable.columnName(casacore::MSPolarizationEnums::CORR_PRODUCT));
  casacore::ScalarColumn<bool> flagRowCol = casacore::ScalarColumn<bool>(
      polTable, polTable.columnName(casacore::MSPolarizationEnums::FLAG_ROW));

  const size_t rowIndex = polTable.nrow();
  polTable.addRow(1);
  numCorrCol.put(rowIndex, 4);

  casacore::Vector<int> cTypeVec(4);
  cTypeVec[0] = 9;
  cTypeVec[1] = 10;
  cTypeVec[2] = 11;
  cTypeVec[3] = 12;
  corrTypeCol.put(rowIndex, cTypeVec);

  casacore::Array<int> cProdArr(casacore::IPosition(2, 2, 4));
  casacore::Array<int>::iterator i = cProdArr.begin();
  *i = 0;
  ++i;
  *i = 0;
  ++i;
  *i = 0;
  ++i;
  *i = 1;
  ++i;
  *i = 1;
  ++i;
  *i = 0;
  ++i;
  *i = 1;
  ++i;
  *i = 1;
  corrProductCol.put(rowIndex, cProdArr);

  flagRowCol.put(rowIndex, flagRow);
}

void CombineStatistics(const std::string& result_filename,
                       const std::vector<std::string>& input_filenames) {
  if (!input_filenames.empty()) {
    const std::string& firstInFilename = *input_filenames.begin();
    std::cout << "Combining " << input_filenames.size() << " sets into "
              << result_filename << '\n';

    std::vector<AntennaInfo> antennae;
    StatisticsCollection statisticsCollection;
    const HistogramCollection histogramCollection;
    std::cout << "Reading antenna table...\n";
    const MSMetaData msMeta(firstInFilename);
    antennae.resize(msMeta.AntennaCount());
    for (size_t i = 0; i != msMeta.AntennaCount(); ++i)
      antennae[i] = msMeta.GetAntennaInfo(i);

    for (std::vector<std::string>::const_iterator i = input_filenames.begin();
         i != input_filenames.end(); ++i) {
      std::cout << "Reading " << *i << "...\n";
      // TODO read quality tables from all inFilenames
      QualityTablesFormatter formatter(*i);
      StatisticsCollection collectionPart;
      collectionPart.Load(formatter);
      if (i == input_filenames.begin())
        statisticsCollection.SetPolarizationCount(
            collectionPart.PolarizationCount());
      statisticsCollection.Add(collectionPart);
    }
    // Create main table
    casacore::TableDesc tableDesc = casacore::MS::requiredTableDesc();
    const casacore::ArrayColumnDesc<std::complex<float>> dataColumnDesc =
        casacore::ArrayColumnDesc<std::complex<float>>(
            casacore::MS::columnName(casacore::MSMainEnums::DATA));
    tableDesc.addColumn(dataColumnDesc);
    casacore::SetupNewTable newTab(result_filename, tableDesc,
                                   casacore::Table::New);
    casacore::MeasurementSet ms(newTab);
    ms.createDefaultSubtables(casacore::Table::New);

    std::cout << "Writing antenna table...\n";
    WriteAntennae(ms, antennae);

    std::cout << "Writing polarization table ("
              << statisticsCollection.PolarizationCount() << " pols)...\n";
    WritePolarizationForLinearPols(ms);

    std::cout << "Writing quality table...\n";
    QualityTablesFormatter formatter(result_filename);
    statisticsCollection.Save(formatter);
  }
}

void RemoveStatistics(const std::string& filename) {
  QualityTablesFormatter formatter(filename);
  formatter.RemoveAllQualityTables();
}

void PrintRfiSlope(const std::string& filename) {
  HistogramTablesFormatter histogramFormatter(filename);
  const unsigned polarizationCount = MSMetaData::PolarizationCount(filename);
  HistogramCollection collection(polarizationCount);
  collection.Load(histogramFormatter);
  const MSMetaData set(filename);
  std::cout << set.GetBandInfo(0).CenterFrequencyHz();
  for (unsigned p = 0; p < polarizationCount; ++p) {
    LogHistogram histogram;
    collection.GetRFIHistogramForCrossCorrelations(p, histogram);
    std::cout << '\t' << histogram.NormalizedSlopeInRFIRegion();
  }
  std::cout << '\n';
}

void PrintRfiSlopePerBaseline(const std::string& filename,
                              const char* dataColumnName) {
  HistogramTablesFormatter histogramFormatter(filename);
  const unsigned polarizationCount = MSMetaData::PolarizationCount(filename);
  HistogramCollection collection;
  CollectHistograms(filename, collection, 0, std::set<size_t>(),
                    dataColumnName);
  const MSMetaData set(filename);
  const size_t antennaCount = set.AntennaCount();
  std::vector<AntennaInfo> antennae(antennaCount);
  for (size_t a = 0; a < antennaCount; ++a) antennae[a] = set.GetAntennaInfo(a);

  HistogramCollection* summedCollection =
      collection.CreateSummedPolarizationCollection();
  const std::map<HistogramCollection::AntennaPair, LogHistogram*>&
      histogramMap = summedCollection->GetRFIHistogram(0);
  PrintRFISlopeForHistogram(histogramMap, '*', &antennae[0]);
  delete summedCollection;
  for (unsigned p = 0; p < polarizationCount; ++p) {
    const std::map<HistogramCollection::AntennaPair, LogHistogram*>&
        histogramMap = collection.GetRFIHistogram(p);
    PrintRFISlopeForHistogram(histogramMap, '0' + p, &antennae[0]);
  }
}

void RemoveHistogram(const std::string& filename) {
  HistogramTablesFormatter histogramFormatter(filename);
  histogramFormatter.RemoveAll();
}

}  // namespace quality
