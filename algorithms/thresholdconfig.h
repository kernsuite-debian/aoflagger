#ifndef THRESHOLDCONFIG_H
#define THRESHOLDCONFIG_H

#include <vector>

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

namespace algorithms {

class ThresholdConfig {
 public:
  enum Distribution { Gaussian, Rayleigh };
  ThresholdConfig();

  void InitializeLengthsDefault(unsigned count = 0);

  void InitializeLengthsSingleSample();

  void InitializeThresholdsFromFirstThreshold(num_t firstThreshold,
                                              Distribution noiseDistribution);

  void Execute(const Image2D* image, Mask2D* mask, bool additive,
               num_t timeSensitivity, num_t frequencySensitivity) const;
  void ExecuteWithMissing(const Image2D* image, Mask2D* mask,
                          const Mask2D* missing, bool additive,
                          num_t timeSensitivity,
                          num_t frequencySensitivity) const;

  num_t GetHorizontalThreshold(unsigned index) const {
    return _horizontalOperations[index].threshold;
  }

  num_t GetVerticalThreshold(unsigned index) const {
    return _verticalOperations[index].threshold;
  }

  void SetHorizontalThreshold(unsigned index, num_t threshold) {
    _horizontalOperations[index].threshold = threshold;
  }

  void SetVerticalThreshold(unsigned index, num_t threshold) {
    _verticalOperations[index].threshold = threshold;
  }

  size_t GetHorizontalLength(unsigned index) const {
    return _horizontalOperations[index].length;
  }

  size_t GetVerticalLength(unsigned index) const {
    return _verticalOperations[index].length;
  }

  size_t GetHorizontalOperations() const {
    return _horizontalOperations.size();
  }
  size_t GetVerticalOperations() const { return _horizontalOperations.size(); }
  void RemoveHorizontalOperations() { _horizontalOperations.clear(); }
  void RemoveVerticalOperations() { _verticalOperations.clear(); }

 private:
  struct ThresholdOperation {
    explicit ThresholdOperation(size_t length_)
        : length(length_), threshold(0) {}
    size_t length;
    num_t threshold;
  };
  std::vector<ThresholdOperation> _horizontalOperations;
  std::vector<ThresholdOperation> _verticalOperations;
  Distribution _distribution;
};

}  // namespace algorithms

#endif
