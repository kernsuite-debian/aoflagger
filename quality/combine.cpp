#include "combine.h"

#include "../structures/msmetadata.h"

namespace quality {

FileContents ReadAndCombine(const std::vector<std::string>& files,
                            bool verbose) {
  FileContents result;
  size_t n_polarizations = 0;
  for (size_t i = 0; i != files.size(); ++i) {
    const std::string& filename = files[i];
    if (n_polarizations == 0) {
      n_polarizations = MSMetaData::PolarizationCount(filename);
      result.statistics_collection.SetPolarizationCount(n_polarizations);
      result.histogram_collection.SetPolarizationCount(n_polarizations);
    } else {
      if (n_polarizations != MSMetaData::PolarizationCount(filename)) {
        throw std::runtime_error(
            "Can't combine measurement set quality statistics with different "
            "number of polarizations");
      }
    }

    if (verbose)
      std::cout << " (" << (i + 1) << "/" << files.size() << ") Adding "
                << filename << " to statistics...\n";
    QualityTablesFormatter qualityTables(filename);
    StatisticsCollection statCollection(n_polarizations);
    statCollection.Load(qualityTables);
    result.statistics_collection.Add(statCollection);

    HistogramTablesFormatter histogramTables(filename);
    if (histogramTables.HistogramsExist()) {
      HistogramCollection histCollection(n_polarizations);
      histCollection.Load(histogramTables);
      result.histogram_collection.Add(histCollection);
    }
  }
  return result;
}

}  // namespace quality
