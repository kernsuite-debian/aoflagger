#include "thresholdconfig.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "../structures/image2d.h"

#include "../util/rng.h"

#include "thresholdtools.h"
#include "sumthreshold.h"
#include "sumthresholdmissing.h"

namespace algorithms {

ThresholdConfig::ThresholdConfig() : _distribution(Gaussian) {}

void ThresholdConfig::InitializeLengthsDefault(unsigned count) {
  if (count > 9 || count == 0) count = 9;
  constexpr size_t lengths[9] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
  _horizontalOperations.clear();
  _horizontalOperations.reserve(count);
  _verticalOperations.clear();
  _verticalOperations.reserve(count);
  for (unsigned i = 0; i < count; ++i) {
    _horizontalOperations.emplace_back(lengths[i]);
    _verticalOperations.emplace_back(lengths[i]);
  }
}

void ThresholdConfig::InitializeLengthsSingleSample() {
  _horizontalOperations.emplace_back(1);
  _verticalOperations.emplace_back(1);
}

void ThresholdConfig::InitializeThresholdsFromFirstThreshold(
    num_t firstThreshold, Distribution noiseDistribution) {
  constexpr num_t exp_base = 1.5;
  for (ThresholdConfig::ThresholdOperation& op : _horizontalOperations) {
    op.threshold =
        firstThreshold * std::pow(exp_base, std::log2(op.length)) / op.length;
  }
  for (ThresholdConfig::ThresholdOperation& op : _verticalOperations) {
    op.threshold =
        firstThreshold * std::pow(exp_base, std::log2(op.length)) / op.length;
  }
  _distribution = noiseDistribution;
}

void ThresholdConfig::Execute(const Image2D* image, Mask2D* mask, bool additive,
                              num_t timeSensitivity,
                              num_t frequencySensitivity) const {
  ExecuteWithMissing(image, mask, nullptr, additive, timeSensitivity,
                     frequencySensitivity);
}

void ThresholdConfig::ExecuteWithMissing(const Image2D* image, Mask2D* mask,
                                         const Mask2D* missing, bool additive,
                                         num_t timeSensitivity,
                                         num_t frequencySensitivity) const {
  num_t timeFactor = timeSensitivity;
  num_t frequencyFactor = frequencySensitivity;

  Image2D modified_image;
  if (!image->AllFinite()) {
    modified_image = image->MakeFiniteCopy();
    image = &modified_image;
  }

  switch (_distribution) {
    case Gaussian: {
      num_t mean, stddev;
      if (missing == nullptr)
        ThresholdTools::WinsorizedMeanAndStdDev(image, mask, mean, stddev);
      else
        ThresholdTools::WinsorizedMeanAndStdDev(image, mask, missing, mean,
                                                stddev);
      if (stddev != 0.0L) {
        timeFactor = stddev * timeSensitivity;
        frequencyFactor = stddev * frequencySensitivity;
      }
    } break;
    case Rayleigh: {
      num_t mode;
      if (missing == nullptr)
        mode = ThresholdTools::WinsorizedMode(image, mask);
      else
        mode = ThresholdTools::WinsorizedMode(image, mask, missing);
      if (mode != 0.0L) {
        timeFactor = timeSensitivity * mode;
        frequencyFactor = frequencySensitivity * mode;
      }
    } break;
  }

  if (!additive) mask->SetAll<false>();
  Mask2D scratch(*mask);

  const size_t operationCount =
      std::max(_horizontalOperations.size(), _verticalOperations.size());
  SumThreshold::VerticalScratch normalScratch(mask->Width(), mask->Height());
  SumThresholdMissing::VerticalCache vMissingCache;
  if (missing)
    SumThresholdMissing::InitializeVertical(vMissingCache, *image, *missing);
  for (size_t i = 0; i < operationCount; ++i) {
    if (i < _horizontalOperations.size()) {
      if (missing == nullptr) {
        SumThreshold::HorizontalLarge(
            image, mask, &scratch, _horizontalOperations[i].length,
            _horizontalOperations[i].threshold * timeFactor);
      } else {
        SumThresholdMissing::Horizontal(
            *image, *mask, *missing, scratch, _horizontalOperations[i].length,
            _horizontalOperations[i].threshold * timeFactor);
      }
    }

    if (i < _verticalOperations.size()) {
      if (missing == nullptr)
        SumThreshold::VerticalLarge(
            image, mask, &scratch, &normalScratch,
            _verticalOperations[i].length,
            _verticalOperations[i].threshold * frequencyFactor);
      else
        SumThresholdMissing::VerticalStacked(
            vMissingCache, *image, *mask, *missing, scratch,
            _verticalOperations[i].length,
            _verticalOperations[i].threshold * frequencyFactor);
    }
  }
}

}  // namespace algorithms
