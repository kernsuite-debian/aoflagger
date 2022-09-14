#ifndef TIMEFREQUENCYSTATISTICS_H
#define TIMEFREQUENCYSTATISTICS_H

#include <string>

#include "../structures/timefrequencydata.h"

namespace algorithms {

class TimeFrequencyStatistics {
 public:
  explicit TimeFrequencyStatistics(const TimeFrequencyData& data);

  num_t GetFlaggedRatio();

  static std::string FormatRatio(num_t ratio);

 private:
  TimeFrequencyData _data;
};

}  // namespace algorithms

#endif  // TIMEFREQUENCYSTATISTICS_H
