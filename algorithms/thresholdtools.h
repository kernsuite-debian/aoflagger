#ifndef THRESHOLDTOOLS_H
#define THRESHOLDTOOLS_H

#include <vector>
#include <cmath>
#include <complex>

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

namespace algorithms {

class ThresholdTools {
 public:
  static void MeanAndStdDev(const Image2D* image, const Mask2D* mask,
                            num_t& mean, num_t& stddev);
  static numl_t Sum(const Image2D* image, const Mask2D* mask);
  static numl_t RMS(const Image2D* image, const Mask2D* mask);
  static num_t Mode(const Image2D* input, const Mask2D* mask);
  static num_t WinsorizedMode(const Image2D* image, const Mask2D* mask);
  static num_t WinsorizedMode(const Image2D* image, const Mask2D* maskA,
                              const Mask2D* maskB);
  static num_t WinsorizedMode(const Image2D* image);
  template <typename T>
  static void TrimmedMeanAndStdDev(const std::vector<T>& input, T& mean,
                                   T& stddev);
  template <typename T>
  static void WinsorizedMeanAndStdDev(const std::vector<T>& input, T& mean,
                                      T& stddev);
  static void WinsorizedMeanAndStdDev(const Image2D* image, const Mask2D* mask,
                                      num_t& mean, num_t& stddev);
  static void WinsorizedMeanAndStdDev(const Image2D* image, const Mask2D* maskA,
                                      const Mask2D* maskB, num_t& mean,
                                      num_t& stddev);
  static void WinsorizedMeanAndStdDev(const Image2D* image, num_t& mean,
                                      num_t& stddev);

  template <typename T>
  static double WinsorizedRMS(const std::vector<std::complex<T>>& input);

  static num_t MinValue(const Image2D* image, const Mask2D* mask);
  static num_t MaxValue(const Image2D* image, const Mask2D* mask);
  static void SetFlaggedValuesToZero(Image2D* dest, const Mask2D* mask);
  static void CountMaskLengths(const Mask2D* mask, int* lengths,
                               size_t lengthsSize);

  static void FilterConnectedSamples(Mask2D* mask,
                                     size_t minConnectedSampleArea,
                                     bool eightConnected = true);
  static void FilterConnectedSample(Mask2D* mask, size_t x, size_t y,
                                    size_t minConnectedSampleArea,
                                    bool eightConnected = true);
  static void UnrollPhase(Image2D* image);
  static Image2DPtr ShrinkHorizontally(size_t factor, const Image2D* input,
                                       const Mask2D* mask);
  static Image2DPtr ShrinkVertically(size_t factor, const Image2D* input,
                                     const Mask2D* mask);

  static Image2DPtr FrequencyRectangularConvolution(const Image2D* source,
                                                    size_t convolutionSize) {
    Image2DPtr image(new Image2D(*source));
    const size_t upperWindowHalf = (convolutionSize + 1) / 2;
    for (size_t x = 0; x < image->Width(); ++x) {
      num_t sum = 0.0;
      for (size_t y = 0; y < upperWindowHalf; ++y) sum += image->Value(x, y);
      for (size_t y = upperWindowHalf; y < convolutionSize; ++y) {
        image->SetValue(x, y - upperWindowHalf, sum / (num_t)y);
        sum += image->Value(x, y);
      }
      size_t count = convolutionSize;
      for (size_t y = convolutionSize; y != image->Height(); ++y) {
        image->SetValue(x, y - upperWindowHalf, sum / (num_t)count);
        sum += image->Value(x, y) - image->Value(x, y - convolutionSize);
      }
      for (size_t y = image->Height(); y != image->Height() + upperWindowHalf;
           ++y) {
        image->SetValue(x, y - upperWindowHalf, sum / (num_t)count);
        sum -= image->Value(x, y - convolutionSize);
        --count;
      }
    }
    return image;
  }

  static Mask2DPtr Threshold(const Image2D* image, num_t threshold) {
    Mask2DPtr mask =
        Mask2D::CreateUnsetMaskPtr(image->Width(), image->Height());
    for (size_t y = 0; y < image->Height(); ++y) {
      for (size_t x = 0; x < image->Width(); ++x) {
        mask->SetValue(x, y, image->Value(x, y) >= threshold);
      }
    }
    return mask;
  }

 private:
  ThresholdTools() {}

  // We need this less than operator, because the normal operator
  // does not enforce a strictly ordered set, because a<b != !(b<a) in the case
  // of nans/infs.
  static bool numLessThanOperator(const num_t& a, const num_t& b) {
    if (std::isfinite(a)) {
      if (std::isfinite(b))
        return a < b;
      else
        return true;
    }
    return false;
  }

  template <typename T>
  static bool complexLessThanOperator(const std::complex<T>& a,
                                      const std::complex<T>& b) {
    if (std::isfinite(a.real()) && std::isfinite(a.imag())) {
      if (std::isfinite(b.real()) && std::isfinite(b.imag()))
        return (a * std::conj(a)).real() < (b * std::conj(b)).real();
      else
        return true;
    }
    return false;
  }
};

}  // namespace algorithms

#endif
