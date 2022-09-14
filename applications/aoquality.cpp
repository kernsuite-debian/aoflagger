#include <iostream>

#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/SetupNewTab.h>
#include <casacore/tables/Tables/TableCopy.h>

#include "../structures/msmetadata.h"

#include "../quality/collector.h"
#include "../quality/defaultstatistics.h"
#include "../quality/histogramcollection.h"
#include "../quality/qualitytablesformatter.h"
#include "../quality/statisticscollection.h"
#include "../quality/statisticsderivator.h"

#include "../util/plot.h"
#include "../util/progress/stdoutreporter.h"

void actionCollect(const std::string& filename, Collector::CollectingMode mode,
                   size_t flaggedTimesteps, std::set<size_t>&& flaggedAntennae,
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

void actionCollectHistogram(const std::string& filename,
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

void printStatistics(std::complex<long double>* complexStat, unsigned count) {
  if (count != 1) std::cout << '[';
  if (count > 0)
    std::cout << complexStat[0].real() << " + " << complexStat[0].imag() << 'i';
  for (unsigned p = 1; p < count; ++p) {
    std::cout << ", " << complexStat[p].real() << " + " << complexStat[p].imag()
              << 'i';
  }
  if (count != 1) std::cout << ']';
}

void printStatistics(unsigned long* stat, unsigned count) {
  if (count != 1) std::cout << '[';
  if (count > 0) std::cout << stat[0];
  for (unsigned p = 1; p < count; ++p) {
    std::cout << ", " << stat[p];
  }
  if (count != 1) std::cout << ']';
}

void printStatistics(const DefaultStatistics& statistics) {
  std::cout << "Count=";
  printStatistics(statistics.count, statistics.PolarizationCount());
  std::cout << "\nSum=";
  printStatistics(statistics.sum, statistics.PolarizationCount());
  std::cout << "\nSumP2=";
  printStatistics(statistics.sumP2, statistics.PolarizationCount());
  std::cout << "\nDCount=";
  printStatistics(statistics.dCount, statistics.PolarizationCount());
  std::cout << "\nDSum=";
  printStatistics(statistics.dSum, statistics.PolarizationCount());
  std::cout << "\nDSumP2=";
  printStatistics(statistics.dSumP2, statistics.PolarizationCount());
  std::cout << "\nRFICount=";
  printStatistics(statistics.rfiCount, statistics.PolarizationCount());
  std::cout << '\n';
}

void actionQueryGlobalStat(const std::string& kindName,
                           const std::string& filename) {
  MSMetaData ms(filename);
  const unsigned polarizationCount = ms.PolarizationCount();
  BandInfo band;
  if (ms.BandCount() != 0) band = ms.GetBandInfo(0);

  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  QualityTablesFormatter formatter(filename);
  StatisticsCollection collection(polarizationCount);
  collection.Load(formatter);
  DefaultStatistics statistics(polarizationCount);
  collection.GetGlobalCrossBaselineStatistics(statistics);
  StatisticsDerivator derivator(collection);
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
  for (unsigned p = 0; p < polarizationCount; ++p) {
    long double val = derivator.GetStatisticAmplitude(kind, statistics, p);
    std::cout << '\t' << val;
  }
  std::cout << '\n';
}

void actionQueryFrequencyRange(const std::string& kindName,
                               const std::string& filename, double startFreqMHz,
                               double endFreqMHz) {
  MSMetaData ms(filename);
  const unsigned polarizationCount = ms.PolarizationCount();

  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  QualityTablesFormatter formatter(filename);
  StatisticsCollection collection(polarizationCount);
  collection.Load(formatter);
  DefaultStatistics statistics(polarizationCount);
  collection.GetFrequencyRangeStatistics(statistics, startFreqMHz * 1e6,
                                         endFreqMHz * 1e6);
  StatisticsDerivator derivator(collection);
  const DefaultStatistics singlePol(statistics.ToSinglePolarization());

  std::cout << startFreqMHz << '\t' << endFreqMHz << '\t'
            << derivator.GetStatisticAmplitude(kind, singlePol, 0);
  for (unsigned p = 0; p < polarizationCount; ++p) {
    long double val = derivator.GetStatisticAmplitude(kind, statistics, p);
    std::cout << '\t' << val;
  }
  std::cout << '\n';
}

void actionQueryBaselines(const std::string& kindName,
                          const std::string& filename) {
  MSMetaData ms(filename);
  const unsigned polarizationCount = ms.PolarizationCount();

  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  QualityTablesFormatter formatter(filename);
  StatisticsCollection collection(polarizationCount);
  collection.Load(formatter);
  const std::vector<std::pair<unsigned, unsigned>>& baselines =
      collection.BaselineStatistics().BaselineList();
  StatisticsDerivator derivator(collection);

  std::cout << "ANTENNA1\tANTENNA2";
  for (unsigned p = 0; p < polarizationCount; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (std::vector<std::pair<unsigned, unsigned>>::const_iterator i =
           baselines.begin();
       i != baselines.end(); ++i) {
    const unsigned antenna1 = i->first, antenna2 = i->second;
    std::cout << antenna1 << '\t' << antenna2;
    for (unsigned p = 0; p < polarizationCount; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexBaselineStatistic(kind, antenna1, antenna2, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void actionQueryFrequency(const std::string& kindName,
                          const std::string& filename,
                          std::optional<size_t> downsample) {
  const size_t polarizationCount = MSMetaData::PolarizationCount(filename);
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  QualityTablesFormatter formatter(filename);
  StatisticsCollection collection(polarizationCount);
  collection.Load(formatter);
  if (downsample) collection.LowerFrequencyResolution(*downsample);
  const std::map<double, DefaultStatistics>& freqStats =
      collection.FrequencyStatistics();
  StatisticsDerivator derivator(collection);

  std::cout << "FREQUENCY";
  for (size_t p = 0; p < polarizationCount; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (const std::pair<const double, DefaultStatistics>& freqStat : freqStats) {
    const double frequency = freqStat.first;
    std::cout << frequency * 1e-6;
    for (unsigned p = 0; p < polarizationCount; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexStatistic(kind, freqStat.second, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void actionQueryTime(const std::string& kindName, const std::string& filename) {
  const unsigned polarizationCount = MSMetaData::PolarizationCount(filename);
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  QualityTablesFormatter formatter(filename);
  StatisticsCollection collection(polarizationCount);
  collection.Load(formatter);
  const std::map<double, DefaultStatistics>& timeStats =
      collection.TimeStatistics();
  StatisticsDerivator derivator(collection);

  std::cout << "TIME";
  for (unsigned p = 0; p < polarizationCount; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (std::map<double, DefaultStatistics>::const_iterator i =
           timeStats.begin();
       i != timeStats.end(); ++i) {
    const double time = i->first;
    std::cout << time;
    for (unsigned p = 0; p < polarizationCount; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexStatistic(kind, i->second, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void actionQueryAntenna(const std::string& kindName,
                        const std::string& filename) {
  const unsigned polarizationCount = MSMetaData::PolarizationCount(filename);
  const QualityTablesFormatter::StatisticKind kind =
      QualityTablesFormatter::NameToKind(kindName);

  QualityTablesFormatter formatter(filename);
  StatisticsCollection collection(polarizationCount);
  collection.Load(formatter);
  const std::map<size_t, DefaultStatistics> stats =
      collection.GetAntennaStatistics();
  StatisticsDerivator derivator(collection);

  std::cout << "ANTENNA";
  for (unsigned p = 0; p < polarizationCount; ++p)
    std::cout << '\t' << kindName << "_POL" << p << "_R\t" << kindName << "_POL"
              << p << "_I";
  std::cout << '\n';
  for (const std::pair<const size_t, DefaultStatistics>& s : stats) {
    const size_t antenna = s.first;
    std::cout << antenna;
    for (unsigned p = 0; p < polarizationCount; ++p) {
      const std::complex<long double> val =
          derivator.GetComplexStatistic(kind, s.second, p);
      std::cout << '\t' << val.real() << '\t' << val.imag();
    }
    std::cout << '\n';
  }
}

void actionSummarize(const std::string& filename) {
  StatisticsCollection statisticsCollection;
  HistogramCollection histogramCollection;
  MSMetaData ms(filename);
  const unsigned polarizationCount = ms.PolarizationCount();

  statisticsCollection.SetPolarizationCount(polarizationCount);
  QualityTablesFormatter qualityData(filename);
  statisticsCollection.Load(qualityData);

  DefaultStatistics statistics(statisticsCollection.PolarizationCount());

  statisticsCollection.GetGlobalTimeStatistics(statistics);
  std::cout << "Time statistics: \n";
  printStatistics(statistics);

  statisticsCollection.GetGlobalFrequencyStatistics(statistics);
  std::cout << "\nFrequency statistics: \n";
  printStatistics(statistics);

  statisticsCollection.GetGlobalCrossBaselineStatistics(statistics);
  std::cout << "\nCross-correlated baseline statistics: \n";
  printStatistics(statistics);

  DefaultStatistics singlePolStat = statistics.ToSinglePolarization();
  std::cout << "RFIPercentange: "
            << StatisticsDerivator::GetStatisticAmplitude(
                   QualityTablesFormatter::RFIPercentageStatistic,
                   singlePolStat, 0)
            << '\n';

  statisticsCollection.GetGlobalAutoBaselineStatistics(statistics);
  std::cout << "\nAuto-correlated baseline: \n";
  printStatistics(statistics);
}

void actionSummarizeRFI(const std::string& filename) {
  MSMetaData ms(filename);
  const unsigned polarizationCount = ms.PolarizationCount();
  const BandInfo band = ms.GetBandInfo(0);

  StatisticsCollection statisticsCollection;
  statisticsCollection.SetPolarizationCount(polarizationCount);
  QualityTablesFormatter qualityData(filename);
  statisticsCollection.Load(qualityData);
  DefaultStatistics statistics(statisticsCollection.PolarizationCount());
  statisticsCollection.GetGlobalCrossBaselineStatistics(statistics);
  DefaultStatistics singlePolStat = statistics.ToSinglePolarization();

  double startTime = statisticsCollection.TimeStatistics().begin()->first,
         endTime = statisticsCollection.TimeStatistics().rbegin()->first,
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

  size_t rowIndex = polTable.nrow();
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

void actionCombine(const std::string& outFilename,
                   const std::vector<std::string>& inFilenames) {
  if (!inFilenames.empty()) {
    const std::string& firstInFilename = *inFilenames.begin();
    std::cout << "Combining " << inFilenames.size() << " sets into "
              << outFilename << '\n';

    std::vector<AntennaInfo> antennae;
    StatisticsCollection statisticsCollection;
    HistogramCollection histogramCollection;
    std::cout << "Reading antenna table...\n";
    MSMetaData msMeta(firstInFilename);
    antennae.resize(msMeta.AntennaCount());
    for (size_t i = 0; i != msMeta.AntennaCount(); ++i)
      antennae[i] = msMeta.GetAntennaInfo(i);

    for (std::vector<std::string>::const_iterator i = inFilenames.begin();
         i != inFilenames.end(); ++i) {
      std::cout << "Reading " << *i << "...\n";
      // TODO read quality tables from all inFilenames
      QualityTablesFormatter formatter(*i);
      StatisticsCollection collectionPart;
      collectionPart.Load(formatter);
      if (i == inFilenames.begin())
        statisticsCollection.SetPolarizationCount(
            collectionPart.PolarizationCount());
      statisticsCollection.Add(collectionPart);
    }
    // Create main table
    casacore::TableDesc tableDesc = casacore::MS::requiredTableDesc();
    casacore::ArrayColumnDesc<std::complex<float>> dataColumnDesc =
        casacore::ArrayColumnDesc<std::complex<float>>(
            casacore::MS::columnName(casacore::MSMainEnums::DATA));
    tableDesc.addColumn(dataColumnDesc);
    casacore::SetupNewTable newTab(outFilename, tableDesc,
                                   casacore::Table::New);
    casacore::MeasurementSet ms(newTab);
    ms.createDefaultSubtables(casacore::Table::New);

    std::cout << "Writing antenna table...\n";
    WriteAntennae(ms, antennae);

    std::cout << "Writing polarization table ("
              << statisticsCollection.PolarizationCount() << " pols)...\n";
    WritePolarizationForLinearPols(ms);

    std::cout << "Writing quality table...\n";
    QualityTablesFormatter formatter(outFilename);
    statisticsCollection.Save(formatter);
  }
}

void actionRemove(const std::string& filename) {
  QualityTablesFormatter formatter(filename);
  formatter.RemoveAllQualityTables();
}

void printRFISlopeForHistogram(const std::map<HistogramCollection::AntennaPair,
                                              LogHistogram*>& histogramMap,
                               char polarizationSymbol,
                               const AntennaInfo* antennae) {
  for (std::map<HistogramCollection::AntennaPair, LogHistogram*>::const_iterator
           i = histogramMap.begin();
       i != histogramMap.end(); ++i) {
    const unsigned a1 = i->first.first, a2 = i->first.second;
    Baseline baseline(antennae[a1], antennae[a2]);
    double length = baseline.Distance();
    const LogHistogram& histogram = *i->second;
    double start, end;
    histogram.GetRFIRegion(start, end);
    double slope = histogram.NormalizedSlope(start, end);
    double stddev = histogram.NormalizedSlopeStdError(start, end, slope);
    std::cout << polarizationSymbol << '\t' << a1 << '\t' << a2 << '\t'
              << length << '\t' << slope << '\t' << stddev << '\n';
  }
}

void actionHistogram(const std::string& filename, const std::string& query,
                     const char* dataColumnName) {
  HistogramTablesFormatter histogramFormatter(filename);
  const unsigned polarizationCount = MSMetaData::PolarizationCount(filename);
  if (query == "rfislope") {
    HistogramCollection collection(polarizationCount);
    collection.Load(histogramFormatter);
    MSMetaData set(filename);
    std::cout << set.GetBandInfo(0).CenterFrequencyHz();
    for (unsigned p = 0; p < polarizationCount; ++p) {
      LogHistogram histogram;
      collection.GetRFIHistogramForCrossCorrelations(p, histogram);
      std::cout << '\t' << histogram.NormalizedSlopeInRFIRegion();
    }
    std::cout << '\n';
  } else if (query == "rfislope-per-baseline") {
    HistogramCollection collection;
    actionCollectHistogram(filename, collection, 0, std::set<size_t>(),
                           dataColumnName);
    MSMetaData set(filename);
    size_t antennaCount = set.AntennaCount();
    std::vector<AntennaInfo> antennae(antennaCount);
    for (size_t a = 0; a < antennaCount; ++a)
      antennae[a] = set.GetAntennaInfo(a);

    HistogramCollection* summedCollection =
        collection.CreateSummedPolarizationCollection();
    const std::map<HistogramCollection::AntennaPair, LogHistogram*>&
        histogramMap = summedCollection->GetRFIHistogram(0);
    printRFISlopeForHistogram(histogramMap, '*', &antennae[0]);
    delete summedCollection;
    for (unsigned p = 0; p < polarizationCount; ++p) {
      const std::map<HistogramCollection::AntennaPair, LogHistogram*>&
          histogramMap = collection.GetRFIHistogram(p);
      printRFISlopeForHistogram(histogramMap, '0' + p, &antennae[0]);
    }
  } else if (query == "remove") {
    histogramFormatter.RemoveAll();
  } else {
    std::cerr << "Unknown histogram command: " << query << "\n";
  }
}

void printSyntax(std::ostream& stream, char* argv[]) {
  stream << "Syntax: " << argv[0]
         << " <action> [options]\n\n"
            "Possible actions:\n"
            "\thelp        - Get more info about an action (usage: '"
         << argv[0]
         << " help <action>')\n"
            "\tcollect     - Processes the entire measurement set, collects "
            "the statistics\n"
            "\t              and writes them in the quality tables.\n"
            "\tcombine     - Combine several tables.\n"
            "\thistogram   - Various histogram actions.\n"
            "\tliststats   - Display a list of possible statistic kinds.\n"
            "\tquery_b     - Query per baseline.\n"
            "\tquery_t     - Query per time step.\n"
            "\tquery_f     - Query per frequency.\n"
            "\tquery_fr    - Query a frequency range\n"
            "\tquery_g     - Query single global statistic.\n"
            "\tremove      - Remove all quality tables.\n"
            "\tsummarize   - Give a summary of the statistics currently in the "
            "quality tables.\n"
            "\tsummarizerfi- Give a summary of the rfi statistics.\n"
            "\n\n"
            "A few actions take a statistic kind. Some common statistic kinds "
            "are: StandardDeviation,\n"
            "Variance, Mean, RFIPercentage, RFIRatio, Count. These are case "
            "sensitive. Run 'aoquality liststats' for a full list.\n";
}

int main(int argc, char* argv[]) {
#ifdef HAS_LOFARSTMAN
  register_lofarstman();
#endif  // HAS_LOFARSTMAN

  if (argc < 2) {
    printSyntax(std::cerr, argv);
    return -1;
  } else {
    const std::string action = argv[1];

    if (action == "help") {
      if (argc != 3) {
        printSyntax(std::cout, argv);
      } else {
        std::string helpAction = argv[2];
        if (helpAction == "help") {
          printSyntax(std::cout, argv);
        } else if (helpAction == "collect") {
          std::cout
              << "Syntax: " << argv[0]
              << " collect [-d [column]/-tf/-h] <ms> [quack timesteps] [list "
                 "of antennae]\n\n"
                 "The collect action will go over a whole measurement set and "
                 "\n"
                 "collect the default statistics. It will write the results in "
                 "the \n"
                 "quality subtables of the main measurement set.\n\n"
                 "Currently, the default statistics are:\n"
                 "\tRFIRatio, Count, Mean, SumP2, DCount, DMean, DSumP2.\n"
                 "The subtables that will be updated are:\n"
                 "\tQUALITY_KIND_NAME, QUALITY_TIME_STATISTIC,\n"
                 "\tQUALITY_FREQUENCY_STATISTIC and "
                 "QUALITY_BASELINE_STATISTIC.\n\n";
        } else if (helpAction == "summarize") {
          std::cout
              << "Syntax: " << argv[0]
              << " summarize <ms>\n\n"
                 "Gives a summary of the statistics in the measurement set.\n";
        } else if (helpAction == "query_a") {
          std::cout << "Syntax: " << argv[0]
                    << " query_a <kind> <ms>\n\n"
                       "Prints the given statistic for each antenna.\n";
        } else if (helpAction == "query_b") {
          std::cout << "Syntax: " << argv[0]
                    << " query_b <kind> <ms>\n\n"
                       "Prints the given statistic for each baseline.\n";
        } else if (helpAction == "query_t") {
          std::cout << "Syntax: " << argv[0]
                    << " query_t <kind> <ms>\n\n"
                       "Print the given statistic for each time step.\n";
        } else if (helpAction == "query_f") {
          std::cout << "Syntax: " << argv[0]
                    << " query_f <kind> <ms>\n\n"
                       "Print the given statistic for each frequency.\n";
        } else if (helpAction == "query_g") {
          std::cout << "Syntax " << argv[0]
                    << " query_g <kind> <ms>\n\n"
                       "Print the given statistic for this measurement set.\n";
        } else if (helpAction == "combine") {
          std::cout << "Syntax: " << argv[0]
                    << " combine <target_ms> [<in_ms> [<in_ms> ..]]\n\n"
                       "This will read all given input measurement sets, "
                       "combine the statistics and \n"
                       "write the results to a target measurement set. The "
                       "target measurement set should\n"
                       "not exist beforehand.\n";
        } else if (helpAction == "histogram") {
          std::cout << "Syntax: " << argv[0]
                    << " histogram <query> <ms>]\n\n"
                       "Query can be:\n"
                       "\trfislope - performs linear regression on the part of "
                       "the histogram that should contain the RFI.\n"
                       "\t           Reports one value per polarisation.\n";
        } else if (helpAction == "remove") {
          std::cout << "Syntax: " << argv[0]
                    << " remove [ms]\n\n"
                       "This will completely remove all quality tables from "
                       "the measurement set.\n";
        } else {
          std::cerr << "Unknown action specified in help.\n";
          return -1;
        }
      }
    } else if (action == "liststats") {
      for (int i = 0; i != QualityTablesFormatter::EndPlaceHolderStatistic;
           ++i) {
        QualityTablesFormatter::StatisticKind kind =
            (QualityTablesFormatter::StatisticKind)i;
        std::cout << QualityTablesFormatter::KindToName(kind) << '\n';
      }
    } else if (action == "collect") {
      if (argc < 3) {
        std::cerr << "collect actions needs one or two parameters (the "
                     "measurement set)\n";
        return -1;
      } else {
        int argi = 2;
        bool histograms = false, timeFrequency = false;
        const char* dataColumnName = "DATA";
        size_t intervalStart = 0, intervalEnd = 0;
        while (argi < argc && argv[argi][0] == '-') {
          std::string p = &argv[argi][1];
          if (p == "h")
            histograms = true;
          else if (p == "d") {
            ++argi;
            dataColumnName = argv[argi];
          } else if (p == "tf")
            timeFrequency = true;
          else if (p == "interval") {
            intervalStart = atoi(argv[argi + 1]);
            intervalEnd = atoi(argv[argi + 2]);
            argi += 2;
          } else
            throw std::runtime_error(
                "Bad parameter given to aoquality collect");
          ++argi;
        }
        std::string filename = argv[argi];
        size_t flaggedTimesteps = 0;
        ++argi;
        std::set<size_t> flaggedAntennae;
        if (argi != argc) {
          flaggedTimesteps = atoi(argv[argi]);
          ++argi;
          while (argi != argc) {
            flaggedAntennae.insert(atoi(argv[argi]));
            ++argi;
          }
        }
        Collector::CollectingMode mode;
        if (histograms)
          mode = Collector::CollectHistograms;
        else if (timeFrequency)
          mode = Collector::CollectTimeFrequency;
        else
          mode = Collector::CollectDefault;
        actionCollect(filename, mode, flaggedTimesteps,
                      std::move(flaggedAntennae), dataColumnName, intervalStart,
                      intervalEnd);
      }
    } else if (action == "combine") {
      if (argc < 3) {
        std::cerr << "combine actions needs at least one parameter: aoquality "
                     "combine <output> <input1> [<input2> ...]\n";
        return -1;
      } else {
        std::string outFilename = argv[2];
        std::vector<std::string> inFilenames;
        for (int i = 3; i < argc; ++i) inFilenames.push_back(argv[i]);
        actionCombine(outFilename, inFilenames);
      }
    } else if (action == "histogram") {
      if (argc != 4) {
        std::cerr << "histogram actions needs two parameters (the query and "
                     "the measurement set)\n";
        return -1;
      } else {
        actionHistogram(argv[3], argv[2], "DATA");
      }
    } else if (action == "summarize") {
      if (argc != 3) {
        std::cerr
            << "summarize actions needs one parameter (the measurement set)\n";
        return -1;
      } else {
        actionSummarize(argv[2]);
      }
    } else if (action == "summarizerfi") {
      if (argc != 3) {
        std::cerr << "summarizerfi actions needs one parameter (the "
                     "measurement set)\n";
        return -1;
      } else {
        actionSummarizeRFI(argv[2]);
      }
    } else if (action == "query_g") {
      if (argc != 4) {
        std::cerr << "Syntax for query global stat: 'aoquality query_g <KIND> "
                     "<MS>'\n";
        return -1;
      } else {
        actionQueryGlobalStat(argv[2], argv[3]);
      }
    } else if (action == "query_a") {
      if (argc != 4) {
        std::cerr
            << "Syntax for query antennas: 'aoquality query_a <KIND> <MS>'\n";
        return -1;
      } else {
        actionQueryAntenna(argv[2], argv[3]);
        return 0;
      }
    } else if (action == "query_b") {
      if (argc != 4) {
        std::cerr
            << "Syntax for query baselines: 'aoquality query_b <KIND> <MS>'\n";
        return -1;
      } else {
        actionQueryBaselines(argv[2], argv[3]);
      }
    } else if (action == "query_f") {
      if (argc < 4) {
        std::cerr << "Syntax for query times: 'aoquality query_t [options] "
                     "<KIND> <MS>'\n"
                     "Options:\n"
                     "  -downsample <n_bins>\n"
                     "    Average down the statistics in frequency to the "
                     "given nr of bins.\n";
        return -1;
      } else {
        size_t argi = 2;
        std::optional<size_t> downsample;
        while (argv[argi][0] == '-') {
          std::string p(&argv[argi][1]);
          if (p == "downsample") {
            ++argi;
            downsample = std::atoi(argv[argi]);
          } else
            throw std::runtime_error("Invalid parameter: " + p);
          ++argi;
        }
        actionQueryFrequency(argv[argi], argv[argi + 1], downsample);
        return 0;
      }
    } else if (action == "query_fr") {
      if (argc == 5) {
        std::string range = argv[4];
        if (range == "DVB4")
          actionQueryFrequencyRange(argv[2], argv[3], 167, 174);
        else if (range == "DVB5")
          actionQueryFrequencyRange(argv[2], argv[3], 174, 181);
        else if (range == "DVB6")
          actionQueryFrequencyRange(argv[2], argv[3], 181, 188);
        else if (range == "DVB7")
          actionQueryFrequencyRange(argv[2], argv[3], 188, 195);
        else {
          std::cerr << "Syntax for query times: 'aoquality query_fr <KIND> "
                       "<MS> <START MHZ> <END MHZ>'\n";
          return -1;
        }
        return 0;
      } else if (argc == 6) {
        actionQueryFrequencyRange(argv[2], argv[3], atof(argv[4]),
                                  atof(argv[5]));
        return 0;
      } else {
        std::cerr << "Syntax for query frequency range: 'aoquality query_fr "
                     "<KIND> <MS> "
                     "<START MHZ> <END MHZ>'\n";
        return -1;
      }
    } else if (action == "query_t") {
      if (argc != 4) {
        std::cerr
            << "Syntax for query times: 'aoquality query_t <KIND> <MS>'\n";
        return -1;
      } else {
        actionQueryTime(argv[2], argv[3]);
        return 0;
      }
    } else if (action == "remove") {
      if (argc != 3) {
        std::cerr
            << "Syntax for removing quality tables: 'aoquality remove <MS>'\n";
        return -1;
      } else {
        actionRemove(argv[2]);
        return 0;
      }
    } else {
      std::cerr << "Unknown action '" << action << "'.\n\n";
      printSyntax(std::cerr, argv);
      return -1;
    }

    return 0;
  }
}
