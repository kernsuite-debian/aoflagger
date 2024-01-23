#include "testsetgenerator.h"

#include <cmath>
#include <vector>
#include <utility>

#include "combinatorialthresholder.h"
#include "highpassfilter.h"

#include "../structures/image2d.h"
#include "../msio/pngfile.h"

#include "../util/logger.h"
#include "../util/ffttools.h"
#include "../util/stopwatch.h"

#include "../imaging/defaultmodels.h"
#include "../imaging/model.h"
#include "../imaging/observatorium.h"

namespace algorithms {

void TestSetGenerator::AddSpectralLine(Image2D& data, Mask2D& rfi,
                                       double lineStrength, size_t startChannel,
                                       size_t nChannels, double timeRatio,
                                       double timeOffsetRatio,
                                       enum BroadbandShape shape) {
  const size_t width = data.Width();
  const size_t tStart = size_t(timeOffsetRatio * width);
  const size_t tEnd = size_t((timeOffsetRatio + timeRatio) * width);
  const double tDuration = tEnd - tStart;
  for (size_t t = tStart; t != tEnd; ++t) {
    // x will run from -1 to 1
    const double x = (double)((t - tStart) * 2) / tDuration - 1.0;
    const double factor = shapeLevel(shape, x);
    for (size_t ch = startChannel; ch < startChannel + nChannels; ++ch) {
      const double value = lineStrength * factor;
      data.AddValue(t, ch, value);
      if (value != 0.0) rfi.SetValue(t, ch, true);
    }
  }
}

void TestSetGenerator::AddIntermittentSpectralLine(Image2D& data, Mask2D& rfi,
                                                   double strength,
                                                   size_t channel,
                                                   double probability,
                                                   std::mt19937& mt) {
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  const size_t width = data.Width();
  for (size_t t = 0; t != width; ++t) {
    if (distribution(mt) < probability) {
      data.AddValue(t, channel, strength);
      rfi.SetValue(t, channel, true);
    }
  }
}

void TestSetGenerator::AddBroadbandLine(Image2D& data, Mask2D& rfi,
                                        double lineStrength, size_t startTime,
                                        size_t duration, double frequencyRatio,
                                        double frequencyOffsetRatio) {
  const size_t frequencyCount = data.Height();
  const unsigned fStart = (size_t)(frequencyOffsetRatio * frequencyCount);
  const unsigned fEnd =
      (size_t)((frequencyOffsetRatio + frequencyRatio) * frequencyCount);
  AddBroadbandLinePos(data, rfi, lineStrength, startTime, duration, fStart,
                      fEnd, UniformShape);
}

void TestSetGenerator::AddBroadbandLinePos(Image2D& data, Mask2D& rfi,
                                           double lineStrength,
                                           size_t startTime, size_t duration,
                                           unsigned frequencyStart,
                                           double frequencyEnd,
                                           enum BroadbandShape shape) {
  const double s = frequencyEnd - frequencyStart;
  for (size_t f = frequencyStart; f < frequencyEnd; ++f) {
    // x will run from -1 to 1
    const double x = (double)((f - frequencyStart) * 2) / s - 1.0;
    const double factor = shapeLevel(shape, x);
    for (size_t t = startTime; t < startTime + duration; ++t) {
      data.AddValue(t, f, lineStrength * factor);
      if (lineStrength > 0.0) rfi.SetValue(t, f, true);
    }
  }
}

void TestSetGenerator::AddSlewedBroadbandLinePos(
    Image2D& data, Mask2D& rfi, double lineStrength, double slewrate,
    size_t startTime, size_t duration, unsigned frequencyStart,
    double frequencyEnd, enum BroadbandShape shape) {
  const double s = (frequencyEnd - frequencyStart);
  for (size_t f = frequencyStart; f < frequencyEnd; ++f) {
    // x will run from -1 to 1
    const double x = (double)((f - frequencyStart) * 2) / s - 1.0;
    const double factor = shapeLevel(shape, x);
    const double slew = slewrate * (double)f;
    const size_t slewInt = (size_t)slew;
    const double slewRest = slew - slewInt;

    data.AddValue(startTime + slewInt, f,
                  lineStrength * factor * (1.0 - slewRest));
    if (lineStrength > 0.0) rfi.SetValue(startTime + slewInt, f, true);
    for (size_t t = startTime + 1; t < startTime + duration; ++t) {
      data.AddValue(t + slewInt, f, lineStrength * factor);
      if (lineStrength > 0.0) rfi.SetValue(t + slewInt, f, true);
    }
    data.AddValue(startTime + duration + slewInt, f,
                  lineStrength * factor * slewRest);
    if (lineStrength > 0.0)
      rfi.SetValue(startTime + duration + slewInt, f, true);
  }
}

void TestSetGenerator::AddRfiPos(Image2D& data, Mask2D& rfi,
                                 double lineStrength, size_t startTime,
                                 size_t duration, unsigned frequencyPos) {
  for (size_t t = startTime; t < startTime + duration; ++t) {
    data.AddValue(t, frequencyPos, lineStrength);
    if (lineStrength > 0) rfi.SetValue(t, frequencyPos, true);
  }
}

Image2D TestSetGenerator::MakeRayleighData(unsigned width, unsigned height) {
  Image2D image = Image2D::MakeUnsetImage(width, height);
  for (unsigned y = 0; y < height; ++y) {
    for (unsigned x = 0; x < width; ++x) {
      image.SetValue(x, y, RNG::Rayleigh());
    }
  }
  return image;
}

Image2D TestSetGenerator::MakeGaussianData(unsigned width, unsigned height) {
  Image2D image = Image2D::MakeUnsetImage(width, height);
  for (unsigned y = 0; y < height; ++y) {
    for (unsigned x = 0; x < width; ++x) {
      image.SetValue(x, y, RNG::Gaussian());
    }
  }
  return image;
}

std::string TestSetGenerator::GetDescription(BackgroundTestSet backgroundSet) {
  switch (backgroundSet) {
    case BackgroundTestSet::Empty:
      return "Empty";
    case BackgroundTestSet::LowFrequency:
      return "Low frequency sinusoid";
    case BackgroundTestSet::HighFrequency:
      return "High frequency sinusoids";
    case BackgroundTestSet::ThreeSources:
      return "Three sources";
    case BackgroundTestSet::FiveSources:
      return "Five sources";
    case BackgroundTestSet::FiveFilteredSources:
      return "Five filtered sources";
    case BackgroundTestSet::StaticSidelobeSource:
      return "Static sidelobe source";
    case BackgroundTestSet::StrongVariableSidelobeSource:
      return "Strong sidelobe source";
    case BackgroundTestSet::FaintVariableSidelobeSource:
      return "Faint sidelobe source";
    case BackgroundTestSet::Checker:
      return "Checker grid";
  }
  return "";
}

std::string TestSetGenerator::GetDescription(RFITestSet rfiSet) {
  switch (rfiSet) {
    case RFITestSet::Empty:
      return "Empty";
    case RFITestSet::SpectralLines:
      return "Spectral lines";
    case RFITestSet::GaussianSpectralLines:
      return "Gaussian spectral lines";
    case RFITestSet::IntermittentSpectralLines:
      return "Intermittent spectral lines";
    case RFITestSet::FullBandBursts:
      return "Full-band bursts / A";
    case RFITestSet::HalfBandBursts:
      return "Half-band bursts / B";
    case RFITestSet::VaryingBursts:
      return "Varying bursts / C";
    case RFITestSet::GaussianBursts:
      return "Gaussian bursts";
    case RFITestSet::SinusoidalBursts:
      return "Sinusodial bursts";
    case RFITestSet::SlewedGaussians:
      return "Slewed Gaussian bursts";
    case RFITestSet::FluctuatingBursts:
      return "Fluctuating bursts";
    case RFITestSet::StrongPowerLaw:
      return "Strong power law RFI";
    case RFITestSet::MediumPowerLaw:
      return "Medium power law RFI";
    case RFITestSet::WeakPowerLaw:
      return "Weak power law RFI";
    case RFITestSet::PolarizedSpike:
      return "Polarized spike";
  }
  return std::string();
}

TimeFrequencyData TestSetGenerator::MakeTestSet(RFITestSet rfiSet,
                                                BackgroundTestSet backgroundSet,
                                                size_t width, size_t height) {
  TimeFrequencyData data;
  if (rfiSet == algorithms::RFITestSet::PolarizedSpike) {
    std::vector<Image2DPtr> images(8);
    images[0] = Image2D::CreateZeroImagePtr(width, height);
    for (size_t p = 1; p != 8; ++p) {
      images[p] = images[0];
    }
    data = TimeFrequencyData::FromLinear(images[0], images[1], images[2],
                                         images[3], images[4], images[5],
                                         images[6], images[7]);
  } else {
    const Image2DPtr real =
        Image2D::MakePtr(TestSetGenerator::MakeNoise(width, height, 1.0));
    const Image2DPtr imag =
        Image2D::MakePtr(TestSetGenerator::MakeNoise(width, height, 1.0));
    data = TimeFrequencyData(aocommon::Polarization::StokesI, real, imag);
  }
  TestSetGenerator::MakeTestSet(rfiSet, data);
  TestSetGenerator::MakeBackground(backgroundSet, data);
  return data;
}

void TestSetGenerator::MakeBackground(BackgroundTestSet testSet,
                                      TimeFrequencyData& data) {
  if (testSet == BackgroundTestSet::StaticSidelobeSource ||
      testSet == BackgroundTestSet::StrongVariableSidelobeSource ||
      testSet == BackgroundTestSet::FaintVariableSidelobeSource) {
    SourceSet source;
    switch (testSet) {
      default:
      case BackgroundTestSet::StaticSidelobeSource:
        source = SourceSet::ConstantDistortion;
        break;
      case BackgroundTestSet::StrongVariableSidelobeSource:
        source = SourceSet::StrongVariableDistortion;
        break;
      case BackgroundTestSet::FaintVariableSidelobeSource:
        source = SourceSet::FaintVariableDistortion;
        break;
    }
    const size_t channelCount = data.ImageHeight();
    double bandwidth;
    bandwidth = (double)channelCount / 16.0 * 2500000.0;
    const std::pair<TimeFrequencyData, TimeFrequencyMetaDataPtr> pair =
        DefaultModels::LoadSet(DefaultModels::NCPSet, source, 0.0, channelCount,
                               bandwidth);
    data = pair.first;
  } else {
    for (size_t polIndex = 0; polIndex != data.PolarizationCount();
         ++polIndex) {
      TimeFrequencyData polData = data.MakeFromPolarizationIndex(polIndex);
      for (size_t imageIndex = 0; imageIndex != polData.ImageCount();
           ++imageIndex) {
        const bool isImaginary = imageIndex == 1;
        if (isImaginary) {
          const Image2DPtr image =
              Image2D::MakePtr(*polData.GetImage(imageIndex));
          const size_t width = image->Width();
          const size_t height = image->Height();
          switch (testSet) {
            default:
            case BackgroundTestSet::Empty:
              break;
            case BackgroundTestSet::LowFrequency:
              for (unsigned y = 0; y < image->Height(); ++y) {
                for (unsigned x = 0; x < image->Width(); ++x) {
                  image->AddValue(
                      x, y,
                      sinn(num_t(x) * M_PIn * 5.0 / image->Width()) + 0.1);
                }
              }
              break;
            case BackgroundTestSet::HighFrequency:
              for (unsigned y = 0; y < image->Height(); ++y) {
                for (unsigned x = 0; x < image->Width(); ++x) {
                  image->AddValue(x, y,
                                  sinn((long double)(x + y * 0.1) * M_PIn *
                                           5.0L / image->Width() +
                                       0.1));
                  image->AddValue(x, y,
                                  sinn((long double)(x + pown(y, 1.1)) * M_PIn *
                                           50.0L / image->Width() +
                                       0.1));
                }
              }
              for (unsigned y = 0; y < image->Height(); ++y) {
                for (unsigned x = 0; x < image->Width(); ++x) {
                  image->AddValue(x, y, 1.0);
                }
              }
              break;
            case BackgroundTestSet::ThreeSources:
              SetModelData(*image, 3, width, height);
              break;
            case BackgroundTestSet::FiveSources:
              SetModelData(*image, 5, width, height);
              break;
            case BackgroundTestSet::FiveFilteredSources:
              SetModelData(*image, 5, width, height);
              SubtractBackground(*image);
              break;
            case BackgroundTestSet::Checker:
              for (size_t y = 0; y != height; ++y) {
                for (size_t x = 0; x != width; ++x) {
                  image->SetValue(x, y, ((x + y) % 2 == 0) ? -1 : 2);
                }
              }
              break;
          }
          polData.SetImage(imageIndex, image);
        }  // if is imaginary
      }    // for over image index
      data.SetPolarizationData(polIndex, polData);
    }  // for over polarizations
  }
}

void TestSetGenerator::MakeTestSet(RFITestSet testSet,
                                   TimeFrequencyData& data) {
  for (size_t polIndex = 0; polIndex != data.PolarizationCount(); ++polIndex) {
    TimeFrequencyData polData = data.MakeFromPolarizationIndex(polIndex);
    const Mask2DPtr rfi(Mask2D::MakePtr(*polData.GetSingleMask()));
    for (size_t imageIndex = 0; imageIndex != polData.ImageCount();
         ++imageIndex) {
      const bool isImaginary = imageIndex == 1;
      const Image2DPtr image = Image2D::MakePtr(*polData.GetImage(imageIndex));
      const size_t width = image->Width();
      const size_t height = image->Height();
      if (polIndex == 2 && isImaginary &&
          testSet == RFITestSet::PolarizedSpike) {
        // Set one absurdly high value which all strategies should detect
        size_t rfiX = width / 2, rfiY = height / 2;
        image->SetValue(rfiX, rfiY, 1000.0);
      } else if (!isImaginary) {
        switch (testSet) {
          case RFITestSet::Empty:
            break;
          case RFITestSet::SpectralLines:
            AddSpectralLinesToTestSet(*image, *rfi, 1.0, UniformShape);
            break;
          case RFITestSet::GaussianSpectralLines:
            AddSpectralLinesToTestSet(*image, *rfi, 1.0, GaussianShape);
            break;
          case RFITestSet::IntermittentSpectralLines:
            AddIntermittentSpectralLinesToTestSet(*image, *rfi, 10.0);
            break;
          case RFITestSet::FullBandBursts:
            AddBroadbandToTestSet(*image, *rfi, 1.0);
            break;
          case RFITestSet::HalfBandBursts:
            AddBroadbandToTestSet(*image, *rfi, 0.5);
            break;
          case RFITestSet::VaryingBursts:
            AddVarBroadbandToTestSet(*image, *rfi);
            break;
          case RFITestSet::GaussianBursts:
            AddBroadbandToTestSet(*image, *rfi, 1.0, 1.0, GaussianShape);
            break;
          case RFITestSet::SinusoidalBursts:
            AddBroadbandToTestSet(*image, *rfi, 1.0, 1.0, SinusoidalShape);
            break;
          case RFITestSet::SlewedGaussians:
            AddSlewedBroadbandToTestSet(*image, *rfi, 1.0);
            break;
          case RFITestSet::FluctuatingBursts:
            AddBurstBroadbandToTestSet(*image, *rfi);
            break;
          case RFITestSet::StrongPowerLaw:
            *image += sampleRFIDistribution(width, height, 1.0);
            rfi->SetAll<true>();
            break;
          case RFITestSet::MediumPowerLaw:
            *image += sampleRFIDistribution(width, height, 0.1);
            rfi->SetAll<true>();
            break;
          case RFITestSet::WeakPowerLaw:
            *image += sampleRFIDistribution(width, height, 0.01);
            rfi->SetAll<true>();
            break;
          case RFITestSet::PolarizedSpike:
            break;
        }
      }
      polData.SetImage(imageIndex, image);
    }
    polData.SetGlobalMask(rfi);
    data.SetPolarizationData(polIndex, polData);
  }
}

void TestSetGenerator::AddSpectralLinesToTestSet(Image2D& image, Mask2D& rfi,
                                                 double baseStrength,
                                                 enum BroadbandShape shape) {
  for (size_t i = 0; i != 10; ++i) {
    const size_t channel = ((i * 2 + 1) * image.Height()) / 20;
    const double strength = baseStrength * (1.0 + (i * 2.0 / 10.0));
    AddSpectralLine(image, rfi, strength, channel, 1, 1.0, 0.0, shape);
  }
}

void TestSetGenerator::AddIntermittentSpectralLinesToTestSet(Image2D& image,
                                                             Mask2D& rfi,
                                                             double strength) {
  std::mt19937 mt;
  for (size_t i = 0; i != 20; ++i) {
    const size_t channel = ((i * 2 + 1) * image.Height()) / 40;
    const double probability = (i + 5) * 1.0 / 28.0;
    AddIntermittentSpectralLine(image, rfi, strength, channel, probability, mt);
  }
}

void TestSetGenerator::AddBroadbandToTestSet(Image2D& image, Mask2D& rfi,
                                             double length, double strength,
                                             enum BroadbandShape shape) {
  const size_t frequencyCount = image.Height();
  const unsigned step = image.Width() / 11;
  const unsigned fStart = (unsigned)((0.5 - length / 2.0) * frequencyCount);
  const unsigned fEnd = (unsigned)((0.5 + length / 2.0) * frequencyCount);
  AddBroadbandLinePos(image, rfi, 3.0 * strength, step * 1, 3, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 2.5 * strength, step * 2, 3, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 2.0 * strength, step * 3, 3, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 1.8 * strength, step * 4, 3, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 1.6 * strength, step * 5, 3, fStart, fEnd,
                      shape);

  AddBroadbandLinePos(image, rfi, 3.0 * strength, step * 6, 1, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 2.5 * strength, step * 7, 1, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 2.0 * strength, step * 8, 1, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 1.8 * strength, step * 9, 1, fStart, fEnd,
                      shape);
  AddBroadbandLinePos(image, rfi, 1.6 * strength, step * 10, 1, fStart, fEnd,
                      shape);
}

void TestSetGenerator::AddSlewedBroadbandToTestSet(Image2D& image, Mask2D& rfi,
                                                   double length,
                                                   double strength,
                                                   double slewrate,
                                                   enum BroadbandShape shape) {
  const size_t frequencyCount = image.Height();
  const unsigned step = image.Width() / 11;
  const unsigned fStart = (unsigned)((0.5 - length / 2.0) * frequencyCount);
  const unsigned fEnd = (unsigned)((0.5 + length / 2.0) * frequencyCount);
  AddSlewedBroadbandLinePos(image, rfi, 3.0 * strength, slewrate, step * 1, 3,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 2.5 * strength, slewrate, step * 2, 3,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 2.0 * strength, slewrate, step * 3, 3,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 1.8 * strength, slewrate, step * 4, 3,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 1.6 * strength, slewrate, step * 5, 3,
                            fStart, fEnd, shape);

  AddSlewedBroadbandLinePos(image, rfi, 3.0 * strength, slewrate, step * 6, 1,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 2.5 * strength, slewrate, step * 7, 1,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 2.0 * strength, slewrate, step * 8, 1,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 1.8 * strength, slewrate, step * 9, 1,
                            fStart, fEnd, shape);
  AddSlewedBroadbandLinePos(image, rfi, 1.6 * strength, slewrate, step * 10, 1,
                            fStart, fEnd, shape);
}

void TestSetGenerator::AddVarBroadbandToTestSet(Image2D& image, Mask2D& rfi) {
  // The "randomness" should be reproducable randomness, so calling
  // the random number generator to generate the numbers is not a good
  // idea.
  const unsigned step = image.Width() / 11;
  AddBroadbandLine(image, rfi, 3.0, step * 1, 3, 0.937071, 0.0185952);
  AddBroadbandLine(image, rfi, 2.5, step * 2, 3, 0.638442, 0.327689);
  AddBroadbandLine(image, rfi, 2.0, step * 3, 3, 0.859308, 0.0211675);
  AddBroadbandLine(image, rfi, 1.8, step * 4, 3, 0.418327, 0.324842);
  AddBroadbandLine(image, rfi, 1.6, step * 5, 3, 0.842374, 0.105613);

  AddBroadbandLine(image, rfi, 3.0, step * 6, 1, 0.704607, 0.163653);
  AddBroadbandLine(image, rfi, 2.5, step * 7, 1, 0.777955, 0.0925143);
  AddBroadbandLine(image, rfi, 2.0, step * 8, 1, 0.288418, 0.222322);
  AddBroadbandLine(image, rfi, 1.8, step * 9, 1, 0.892462, 0.0381083);
  AddBroadbandLine(image, rfi, 1.6, step * 10, 1, 0.444377, 0.240526);
}

void TestSetGenerator::SetModelData(Image2D& image, unsigned sources,
                                    size_t width, size_t height) {
  class Model model;
  if (sources >= 5) {
    model.AddSource(0.1, 0.1, 0.5);
    model.AddSource(0.1, 0.0, 0.35);
    model.AddSource(.101, 0.001, 0.45);
    model.AddSource(1.0, 0.0, 1.0);
    model.AddSource(4.0, 3.0, 0.9);
  } else {
    if (sources >= 1) model.AddSource(0.1, 0.1, 0.7);
    if (sources >= 2) model.AddSource(0.1, 0.0, 0.5);
    if (sources >= 3) model.AddSource(1.0, 0.0, 1.0);
  }
  WSRTObservatorium wsrt(size_t(0), size_t(1), height);
  const std::pair<TimeFrequencyData, TimeFrequencyMetaDataCPtr> data =
      model.SimulateObservation(width, wsrt, 0.05, 0.05, 0, 1);
  image = *data.first.GetRealPart();
}

void TestSetGenerator::SubtractBackground(Image2D& image) {
  const Mask2DPtr zero =
      Mask2D::CreateSetMaskPtr<false>(image.Width(), image.Height());
  HighPassFilter filter;
  filter.SetHKernelSigmaSq(7.5 * 7.5);
  filter.SetVKernelSigmaSq(15.0 * 15.0);
  filter.SetHWindowSize(20);
  filter.SetVWindowSize(40);
  const Image2DPtr imagePtr = Image2D::MakePtr(image);
  image = *filter.ApplyHighPass(imagePtr, zero);
}

Image2D TestSetGenerator::sampleRFIDistribution(unsigned width, unsigned height,
                                                double ig_over_rsq) {
  Image2D image = Image2D::MakeUnsetImage(width, height);
  const double sigma = 1.0;

  for (size_t f = 0; f < height; ++f) {
    for (size_t t = 0; t < width; ++t) {
      image.SetValue(t, f,
                     Rand(Gaussian) * sigma + ig_over_rsq / RNG::Uniform());
    }
  }
  return image;
}

}  // namespace algorithms
