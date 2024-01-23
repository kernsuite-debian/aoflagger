#ifndef AOFLAGGER_QUALITY_OPERATIONS_H_
#define AOFLAGGER_QUALITY_OPERATIONS_H_

#include <optional>
#include <set>
#include <string>

#include "collector.h"

namespace quality {

void ListStatistics();

void CollectStatistics(const std::string& filename,
                       Collector::CollectingMode mode, size_t flaggedTimesteps,
                       std::set<size_t>&& flaggedAntennae,
                       const char* dataColumnName, size_t intervalStart,
                       size_t intervalEnd);

void CollectHistograms(const std::string& filename,
                       HistogramCollection& histogramCollection,
                       size_t flaggedTimesteps,
                       std::set<size_t>&& flaggedAntennae,
                       const char* dataColumnName);

void CombineStatistics(const std::string& result_filename,
                       const std::vector<std::string>& input_filenames);

void RemoveStatistics(const std::string& filename);

void RemoveHistogram(const std::string& filename);

void PrintSummary(const std::vector<std::string>& filenames);

void PrintRfiSummary(const std::vector<std::string>& filenames);

void PrintGlobalStatistic(const std::string& kindName,
                          const std::vector<std::string>& filenames);

void PrintPerAntennaStatistics(const std::string& kindName,
                               const std::vector<std::string>& filenames);

void PrintPerBaselineStatistics(const std::string& kindName,
                                const std::vector<std::string>& filenames);

void PrintPerTimeStatistics(const std::string& kindName,
                            const std::vector<std::string>& filenames);

void PrintPerFrequencyStatistics(const std::string& kindName,
                                 const std::vector<std::string>& filenames,
                                 std::optional<size_t> downsample);

void PrintFrequencyRangeStatistic(const std::string& kindName,
                                  const std::vector<std::string>& filenames,
                                  double startFreqMHz, double endFreqMHz);

void PrintRfiSlope(const std::string& filename);

void PrintRfiSlopePerBaseline(const std::string& filename,
                              const char* dataColumnName);
}  // namespace quality

#endif
