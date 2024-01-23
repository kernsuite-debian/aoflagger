#ifndef AOFLAGGER_QUALITY_COMBINE_H_
#define AOFLAGGER_QUALITY_COMBINE_H_

#include <string>
#include <vector>

#include "histogramcollection.h"
#include "statisticscollection.h"

namespace quality {

struct FileContents {
  StatisticsCollection statistics_collection;
  HistogramCollection histogram_collection;
};

/**
 * Reads and combines a number of quality statistics tables.
 */
FileContents ReadAndCombine(const std::vector<std::string>& files,
                            bool verbose);

}  // namespace quality

#endif
