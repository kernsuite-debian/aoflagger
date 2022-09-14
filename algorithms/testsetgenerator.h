#ifndef MITIGATIONTESTER_H
#define MITIGATIONTESTER_H

#include <cstddef>
#include <fstream>
#include <random>

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

#include "../util/rng.h"

#include "enums.h"

namespace algorithms {

class TestSetGenerator {
 public:
  enum NoiseType {
    Gaussian,
    GaussianProduct,
    GaussianPartialProduct,
    Rayleigh
  };
  enum BroadbandShape {
    UniformShape,
    GaussianShape,
    SinusoidalShape,
    BurstShape
  };

  static double shapeLevel(enum BroadbandShape shape, double x) {
    switch (shape) {
      default:
      case UniformShape:
        return 1.0;
      case GaussianShape:
        return exp(-x * x * 3.0 * 3.0);
      case SinusoidalShape:
        return (1.0 + cos(x * M_PI * 2.0 * 1.5)) * 0.5;
      case BurstShape:
        return RNG::Gaussian() * 0.6;
    }
  }

  static Image2D MakeRayleighData(unsigned width, unsigned height);
  static Image2D MakeGaussianData(unsigned width, unsigned height);
  static Image2D MakeNoise(unsigned width, unsigned height, int gaussian) {
    if (gaussian == 1)
      return MakeGaussianData(width, height);
    else if (gaussian == 0)
      return MakeRayleighData(width, height);
    else
      return Image2D::MakeZeroImage(width, height);
  }

  static void AddBroadbandLine(Image2D& data, Mask2D& rfi, double lineStrength,
                               size_t startTime, size_t duration) {
    AddBroadbandLine(data, rfi, lineStrength, startTime, duration, 1.0);
  }
  static void AddBroadbandLine(Image2D& data, Mask2D& rfi, double lineStrength,
                               size_t startTime, size_t duration,
                               double frequencyRatio) {
    AddBroadbandLine(data, rfi, lineStrength, startTime, duration,
                     frequencyRatio, 0.5L - frequencyRatio / 2.0L);
  }
  static void AddSpectralLine(Image2D& data, Mask2D& rfi, double lineStrength,
                              size_t startChannel, size_t nChannels,
                              double timeRatio, double timeOffsetRatio,
                              enum BroadbandShape shape);
  static void AddIntermittentSpectralLine(Image2D& data, Mask2D& rfi,
                                          double lineStrength, size_t channel,
                                          double probability, std::mt19937& mt);
  static void AddBroadbandLine(Image2D& data, Mask2D& rfi, double lineStrength,
                               size_t startTime, size_t duration,
                               double frequencyRatio,
                               double frequencyOffsetRatio);
  static void AddBroadbandLinePos(Image2D& data, Mask2D& rfi,
                                  double lineStrength, size_t startTime,
                                  size_t duration, unsigned frequencyStart,
                                  double frequencyEnd,
                                  enum BroadbandShape shape);
  static void AddSlewedBroadbandLinePos(Image2D& data, Mask2D& rfi,
                                        double lineStrength, double slewrate,
                                        size_t startTime, size_t duration,
                                        unsigned frequencyStart,
                                        double frequencyEnd,
                                        enum BroadbandShape shape);
  static void AddRfiPos(Image2D& data, Mask2D& rfi, double lineStrength,
                        size_t startTime, size_t duration,
                        unsigned frequencyPos);

  static std::string GetDescription(BackgroundTestSet backgroundSet);
  static std::string GetDescription(RFITestSet rfiSet);
  static TimeFrequencyData MakeTestSet(RFITestSet rfiSet,
                                       BackgroundTestSet backgroundSet,
                                       size_t width, size_t height);
  static void MakeBackground(BackgroundTestSet backgroundSet,
                             TimeFrequencyData& image);
  static void MakeTestSet(RFITestSet testSet, TimeFrequencyData& data);

 private:
  static void AddSpectralLinesToTestSet(Image2D& image, Mask2D& rfi,
                                        double strength,
                                        enum BroadbandShape shape);
  static void AddIntermittentSpectralLinesToTestSet(Image2D& image, Mask2D& rfi,
                                                    double strength);
  static void AddGaussianBroadbandToTestSet(Image2D& image, Mask2D& rfi) {
    AddBroadbandToTestSet(image, rfi, 1.0, 1.0, GaussianShape);
  }
  static void AddSinusoidalBroadbandToTestSet(Image2D& image, Mask2D& rfi) {
    AddBroadbandToTestSet(image, rfi, 1.0, 1.0, SinusoidalShape);
  }
  static void AddBurstBroadbandToTestSet(Image2D& image, Mask2D& rfi) {
    AddBroadbandToTestSet(image, rfi, 1.0, 1.0, BurstShape);
  }
  static void AddSlewedGaussianBroadbandToTestSet(Image2D& image, Mask2D& rfi) {
    AddSlewedBroadbandToTestSet(image, rfi, 1.0);
  }

  static void AddBroadbandToTestSet(Image2D& image, Mask2D& rfi, double length,
                                    double strength = 1.0,
                                    enum BroadbandShape shape = UniformShape);
  static void AddSlewedBroadbandToTestSet(
      Image2D& image, Mask2D& rfi, double length, double strength = 1.0,
      double slewrate = 0.02, enum BroadbandShape shape = GaussianShape);
  static void AddVarBroadbandToTestSet(Image2D& image, Mask2D& rfi);
  static void SetModelData(Image2D& image, unsigned sources, size_t width,
                           size_t height);
  static void SubtractBackground(Image2D& image);
  static Image2D sampleRFIDistribution(unsigned width, unsigned height,
                                       double ig_over_rsq);

  static double Rand(enum NoiseType type) {
    switch (type) {
      case Gaussian:
        return RNG::Gaussian();
      case GaussianProduct:
        return RNG::GaussianProduct();
      case GaussianPartialProduct:
        return RNG::GaussianPartialProduct();
      case Rayleigh:
        return RNG::Rayleigh();
    }
    throw std::exception();
  }
};

}  // namespace algorithms

#endif
