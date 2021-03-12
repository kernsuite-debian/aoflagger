#ifndef MS_STAT_READER_H
#define MS_STAT_READER_H

#include <complex>
#include <string>
#include <vector>
#include <utility>

#include "../structures/timefrequencydata.h"
#include "../structures/timefrequencymetadata.h"

#include "../imagesets/msoptions.h"

namespace internal {
class MultiBandData;
}

class MSStatReader {
 public:
  MSStatReader(const std::string& filename, const std::string& dataColumn);

  size_t NSequences() const { return _sequenceStart.size() - 1; }
  size_t NBands() const { return _nBands; }

  typedef std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> Result;
  Result Read(BaselineIntegration::Mode statType,
              BaselineIntegration::Differencing diffType, size_t sequenceIndex,
              size_t bandIndex, bool includeAutos, bool includeFlags,
              class ProgressListener& progress) {
    switch (diffType) {
      default:
      case BaselineIntegration::NoDifference:
        return readSamples(statType, sequenceIndex, bandIndex, includeAutos,
                           includeFlags, false, progress);
      case BaselineIntegration::TimeDifference:
        return readTimeDiff(statType, sequenceIndex, bandIndex, includeAutos,
                            includeFlags, progress);
      case BaselineIntegration::FrequencyDifference:
        return readSamples(statType, sequenceIndex, bandIndex, includeAutos,
                           includeFlags, true, progress);
    }
  }
  std::string TelescopeName() const { return _telescopeName; }

  void StoreFlags(const std::vector<Mask2DCPtr>& flags,
                  BaselineIntegration::Differencing diffType,
                  size_t sequenceIndex, size_t bandIndex, bool includeAutos);

 private:
  Result readSamples(BaselineIntegration::Mode statType, size_t sequenceIndex,
                     size_t bandIndex, bool includeAutos, bool includeFlags,
                     bool freqDiff, ProgressListener& progress);
  Result readTimeDiff(BaselineIntegration::Mode statType, size_t sequenceIndex,
                      size_t bandIndex, bool includeAutos, bool includeFlags,
                      ProgressListener& progress);
  void fillBand(MSStatReader::Result& result, bool freqDiff,
                const internal::MultiBandData& bands, size_t bandIndex);

  struct Statistic {
    double data1 = 0.0, data2 = 0.0, data3 = 0.0;
    size_t count = 0;

    void Add(BaselineIntegration::Mode stat, std::complex<float> sample) {
      ++count;
      switch (stat) {
        case BaselineIntegration::Count:
          break;
        case BaselineIntegration::Average:
          data1 += sample.real();
          break;
        case BaselineIntegration::AverageAbs:
          data1 += std::abs(sample);
          break;
        case BaselineIntegration::Squared:
          data1 += std::norm(sample);
          break;
        case BaselineIntegration::Stddev:
          data1 += sample.real();
          data2 += sample.imag();
          data3 += std::norm(sample);
          break;
      }
    }

    double Calculate(BaselineIntegration::Mode stat) const {
      switch (stat) {
        case BaselineIntegration::Count:
          return count;
        case BaselineIntegration::Average:
        case BaselineIntegration::AverageAbs:
        case BaselineIntegration::Squared:
          return data1 / count;
        case BaselineIntegration::Stddev: {
          double normMeanSq =
              std::norm(std::complex<double>(data1, data2) / double(count));
          return std::sqrt(data3 / count - normMeanSq);
        }
      }
      return 0.0;
    }
  };

  Result makeResult(BaselineIntegration::Mode statType,
                    const Statistic* statsData, size_t nPolarizations,
                    size_t nChannels, size_t nTimes);

  void storeFlags(const std::vector<Mask2DCPtr>& flags, size_t sequenceIndex,
                  size_t bandIndex, bool includeAutos);

  const std::string _filename;
  const std::string _dataColumn;
  size_t _nBands;
  std::vector<size_t> _sequenceStart;
  std::string _telescopeName;
};

#endif
