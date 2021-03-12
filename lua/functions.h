#ifndef LUA_FUNCTIONS_H
#define LUA_FUNCTIONS_H

#include "data.h"

#include <cstring>

class ScriptData;

namespace aoflagger_lua {
void apply_bandpass(Data& data, const std::string& filename,
                    ScriptData& scriptData);

void collect_statistics(const Data& dataAfter, const Data& dataBefore,
                        ScriptData& scriptData);

void copy_to_channel(Data& destination, const Data& source, size_t channel);

void copy_to_frequency(Data& destination, const Data& source,
                       double frequencyHz);

Data downsample(const Data& data, size_t horizontalFactor,
                size_t verticalFactor);

Data downsample_masked(const Data& data, size_t horizontalFactor,
                       size_t verticalFactor);

void upsample(const Data& input, Data& destination, size_t horizontalFactor,
              size_t verticalFactor);

void high_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight,
                      double horizontalSigmaSquared,
                      double verticalSigmaSquared);

void low_pass_filter(Data& data, size_t kernelWidth, size_t kernelHeight,
                     double horizontalSigmaSquared,
                     double verticalSigmaSquared);

// TODO this function should collect the statistics and print
// them later on (and be renamed).
void print_polarization_statistics(const Data& data);

void save_heat_map(const char* filename, const Data& data);

void scale_invariant_rank_operator(Data& data, double level_horizontal,
                                   double level_vertical);

void scale_invariant_rank_operator_masked(Data& data, const Data& missing,
                                          double level_horizontal,
                                          double level_vertical);

void sumthreshold(Data& data, double hThresholdFactor, double vThresholdFactor,
                  bool horizontal, bool vertical);

void sumthreshold_masked(Data& data, const Data& missing,
                         double hThresholdFactor, double vThresholdFactor,
                         bool horizontal, bool vertical);

void threshold_channel_rms(Data& data, double threshold,
                           bool thresholdLowValues);

void threshold_timestep_rms(Data& data, double threshold);

Data trim_channels(const Data& data, size_t start_channel, size_t end_channel);

Data trim_frequencies(const Data& data, double start_frequency,
                      double end_frequency);

void visualize(Data& data, const std::string& label, size_t sortingIndex,
               ScriptData& scriptData);
}  // namespace aoflagger_lua

#endif
