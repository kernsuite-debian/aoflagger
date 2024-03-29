#include "../algorithms/antennaselector.h"
#include "../algorithms/baselineselector.h"

#include "../structures/msmetadata.h"

#include "../quality/defaultstatistics.h"
#include "../quality/histogramcollection.h"
#include "../quality/qualitytablesformatter.h"
#include "../quality/statisticscollection.h"
#include "../quality/statisticsderivator.h"

#include "../quality/histogramtablesformatter.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <casacore/tables/Tables/ArrayColumn.h>
#include <casacore/tables/Tables/ScalarColumn.h>

#include <iostream>

using algorithms::AntennaSelector;
using algorithms::BaselineSelector;

StatisticsCollection load(const std::string& filename,
                          std::vector<AntennaInfo>& antennae) {
  StatisticsCollection statisticsCollection;
  const HistogramCollection histogramCollection;
  const MSMetaData ms(filename);
  const unsigned polarizationCount = ms.PolarizationCount();

  statisticsCollection.SetPolarizationCount(polarizationCount);
  QualityTablesFormatter qualityData(filename);
  statisticsCollection.Load(qualityData);
  const unsigned antennaCount = ms.AntennaCount();
  for (unsigned a = 0; a < antennaCount; ++a)
    antennae.push_back(ms.GetAntennaInfo(a));
  return statisticsCollection;
}

std::set<size_t> detectRFIPercentage(const char* filename) {
  std::vector<AntennaInfo> antennae;
  StatisticsCollection statisticsCollection = load(filename, antennae);

  BaselineSelector selector;
  selector.SetUseLog(true);

  statisticsCollection.IntegrateBaselinesToOneChannel();
  const BaselineStatisticsMap& baselineMap =
      statisticsCollection.BaselineStatistics();
  const std::vector<std::pair<unsigned, unsigned>> list =
      baselineMap.BaselineList();
  for (std::vector<std::pair<unsigned, unsigned>>::const_iterator i =
           list.begin();
       i != list.end(); ++i) {
    const unsigned a1 = i->first, a2 = i->second;

    DefaultStatistics statistic = baselineMap.GetStatistics(a1, a2);
    selector.Add(statistic, antennae[a1], antennae[a2]);
  }
  std::vector<BaselineSelector::SingleBaselineInfo> markedBaselines;
  std::set<unsigned> badStations;

  selector.Search(markedBaselines);
  selector.ImplyStations(markedBaselines, 0.3, badStations);

  std::cout << "List of " << badStations.size() << " bad stations:\n";
  for (const size_t ant : badStations) {
    std::cout << antennae[ant].name << " (" << ant << ")\n";
  }
  return std::set<size_t>(badStations.begin(), badStations.end());
}

std::set<size_t> detectStddev(const char* filename) {
  std::vector<AntennaInfo> antennae;
  const StatisticsCollection statisticsCollection = load(filename, antennae);
  AntennaSelector selector;
  std::vector<size_t> badStations = selector.Run(statisticsCollection);

  std::cout << "List of " << badStations.size() << " bad stations:\n";
  for (const size_t ant : badStations) {
    std::cout << antennae[ant].name << " (" << ant << ")\n";
  }
  return std::set<size_t>(badStations.begin(), badStations.end());
}

void flagAntennas(const char* filename, const std::set<size_t>& antennae) {
  casacore::MeasurementSet ms(filename, casacore::Table::Update);

  /**
   * Read some meta data from the measurement set
   */
  const casacore::MSSpectralWindow spwTable = ms.spectralWindow();
  const size_t spwCount = spwTable.nrow();
  if (spwCount != 1)
    throw std::runtime_error("Set should have exactly one spectral window");

  const casacore::ScalarColumn<int> numChanCol(
      spwTable, casacore::MSSpectralWindow::columnName(
                    casacore::MSSpectralWindowEnums::NUM_CHAN));
  const size_t channelCount = numChanCol.get(0);
  if (channelCount == 0) throw std::runtime_error("No channels in set");

  const casacore::ScalarColumn<int> ant1Column(
      ms, ms.columnName(casacore::MSMainEnums::ANTENNA1));
  const casacore::ScalarColumn<int> ant2Column(
      ms, ms.columnName(casacore::MSMainEnums::ANTENNA2));
  casacore::ArrayColumn<bool> flagsColumn(
      ms, ms.columnName(casacore::MSMainEnums::FLAG));

  if (ms.nrow() == 0) throw std::runtime_error("Table has no rows (no data)");
  const casacore::IPosition flagsShape = flagsColumn.shape(0);

  const casacore::Array<bool> flags(flagsShape, true);

  std::cout << "Flagging... " << std::flush;

  /**
   * Flag
   */
  size_t crossCount = 0, autoCount = 0;
  for (size_t rowIndex = 0; rowIndex != ms.nrow(); ++rowIndex) {
    // Selected?
    if (antennae.find(ant1Column.get(rowIndex)) != antennae.end() ||
        antennae.find(ant2Column.get(rowIndex)) != antennae.end()) {
      if (ant1Column.get(rowIndex) == ant2Column.get(rowIndex))
        ++autoCount;
      else
        ++crossCount;
      flagsColumn.put(rowIndex, flags);
    }
  }

  std::cout << "DONE (selected " << crossCount << " cross- and " << autoCount
            << " auto-correlated timesteps)\n";
}

void printSyntax(std::ostream& stream, char* argv[]) {
  stream << "The executable 'badstations' will give a list of stations that "
            "are outliers\n"
            "according to the RFI statistics.\n"
            "\n"
            "Syntax: badstations [options] <filename>\n"
            "\n"
            "Options:\n"
            "-flag\n"
            "  Will not only show statistics, but also flag the antennas that "
            "are found\n"
            "  to be bad in the measurement set.\n"
            "  (this only works if the given filename is a measurement set, "
            "not a .ref).\n"
            "-method <stddev / percentage>\n"
            "  Select detection method. Method 'stddev' is the default, and "
            "simply detects\n"
            "  stations with an outlyer standard deviation. Method "
            "'percentage' detects\n"
            "  outliers based on the percentage RFI statistic, taking into "
            "account that\n"
            "  short baselines often see fewer RFI, by fitting a curve to the "
            "statistic\n"
            "  as a function of baseline.\n";
}

int main(int argc, char* argv[]) {
#ifdef HAS_LOFARSTMAN
  register_lofarstman();
#endif  // HAS_LOFARSTMAN

  int argi = 1;
  bool doFlag = false;
  enum Method { StddevMethod, RFIPercentangeMethod } method = StddevMethod;
  while (argi < argc && argv[argi][0] == '-') {
    const std::string p(argv[argi] + 1);
    if (p == "flag") {
      doFlag = true;
    } else if (p == "method") {
      ++argi;
      const std::string m = argv[argi];
      if (m == "stddev")
        method = StddevMethod;
      else if (m == "percentage")
        method = RFIPercentangeMethod;
      else
        throw std::runtime_error("Unknown method given");
    } else {
      throw std::runtime_error("Unknown parameter");
    }
    ++argi;
  }
  if (argi >= argc) {
    printSyntax(std::cerr, argv);
    return -1;
  } else {
    const char* filename = argv[argi];
    std::set<size_t> badAntennas;
    switch (method) {
      case StddevMethod:
        badAntennas = detectStddev(filename);
        break;
      case RFIPercentangeMethod:
        badAntennas = detectRFIPercentage(filename);
        break;
    }
    if (doFlag) flagAntennas(filename, badAntennas);
    return 0;
  }
}
