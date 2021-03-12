#include <boost/test/unit_test.hpp>

#include "../../algorithms/highpassfilter.h"
#include "../../algorithms/localfitmethod.h"

#include "../../util/rng.h"
#include "../../util/stopwatch.h"

#include <iostream>

using namespace aocommon;

BOOST_AUTO_TEST_SUITE(high_pass_filter_experiment,
                      *boost::unit_test::label("experiment") *
                          boost::unit_test::disabled())

static void Initialize(Image2DPtr &image, Mask2DPtr &mask) {
  const size_t width = 10000, height = 256;
  image = Image2D::CreateUnsetImagePtr(width, height);
  mask = Mask2D::CreateUnsetMaskPtr(width, height);
  size_t i = 0;
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      image->SetValue(x, y, i);
      mask->SetValue(x, y, (i % 25 == 0) || (y == 5));
      ++i;
    }
  }
}

static void InitializeFlagged(Image2DPtr &image, Mask2DPtr &mask) {
  const size_t width = 10000 / 3, height = 256 / 3;
  image = Image2D::CreateUnsetImagePtr(width, height);
  mask = Mask2D::CreateUnsetMaskPtr(width, height);
  size_t i = 0;
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      image->SetValue(x, y, i);
      mask->SetValue(x, y, (i % 2 == 0) || (y == 5));
      ++i;
    }
  }
}

BOOST_AUTO_TEST_CASE(time_fitting) {
  Image2DPtr image;
  Mask2DPtr mask;
  Initialize(image, mask);

  LocalFitMethod fitMethod;
  TimeFrequencyData data(TimeFrequencyData::AmplitudePart,
                         Polarization::StokesI, image);
  fitMethod.SetToWeightedAverage(10, 20, 2.5, 5.0);
  fitMethod.Initialize(data);
  Stopwatch watch(true);
  for (size_t i = 0; i < fitMethod.TaskCount(); ++i) fitMethod.PerformFit(i);
  std::cout << " time token: " << watch.ToString() << ' ';
}

BOOST_AUTO_TEST_CASE(time_high_pass_filter) {
  Image2DPtr image;
  Mask2DPtr mask;
  Initialize(image, mask);

  HighPassFilter filter;
  filter.SetHWindowSize(21);
  filter.SetVWindowSize(41);
  filter.SetHKernelSigmaSq(2.5);
  filter.SetVKernelSigmaSq(5.0);
  Stopwatch watch(true);
  filter.ApplyHighPass(image, mask);
  std::cout << " time token: " << watch.ToString() << ' ';
}

BOOST_AUTO_TEST_CASE(time_flagged_fitting) {
  Image2DPtr image;
  Mask2DPtr mask;
  InitializeFlagged(image, mask);

  LocalFitMethod fitMethod;
  TimeFrequencyData data(TimeFrequencyData::AmplitudePart,
                         Polarization::StokesI, image);
  fitMethod.SetToWeightedAverage(10, 20, 2.5, 5.0);
  fitMethod.Initialize(data);
  Stopwatch watch(true);
  for (size_t i = 0; i < fitMethod.TaskCount(); ++i) fitMethod.PerformFit(i);
  std::cout << " time token: " << watch.ToString() << ' ';
}

BOOST_AUTO_TEST_CASE(time_flagged_high_pass_filter) {
  Image2DPtr image;
  Mask2DPtr mask;
  InitializeFlagged(image, mask);

  HighPassFilter filter;
  filter.SetHWindowSize(21);
  filter.SetVWindowSize(41);
  filter.SetHKernelSigmaSq(2.5);
  filter.SetVKernelSigmaSq(5.0);
  Stopwatch watch(true);
  filter.ApplyHighPass(image, mask);
  std::cout << " time token: " << watch.ToString() << ' ';
}

BOOST_AUTO_TEST_SUITE_END()
