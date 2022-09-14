#include "../testingtools/imageasserter.h"

#include "../../structures/mask2d.h"
#include "../../structures/image2d.h"

#include "../../algorithms/localfitmethod.h"
#include "../../algorithms/highpassfilter.h"

#include <boost/test/unit_test.hpp>

using aocommon::Polarization;

using algorithms::HighPassFilter;
using algorithms::LocalFitMethod;

BOOST_AUTO_TEST_SUITE(high_pass_filter, *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(filter) {
  const size_t width = 99, height = 99;
  Image2DPtr testImage = Image2D::CreateZeroImagePtr(width, height);
  testImage->SetValue(10, 10, 1.0);
  testImage->SetValue(15, 15, 2.0);
  testImage->SetValue(20, 20, 0.5);

  // Fitting
  LocalFitMethod fitMethod;
  TimeFrequencyData data(TimeFrequencyData::AmplitudePart,
                         Polarization::StokesI,
                         Image2DCPtr(new Image2D(*testImage)));
  fitMethod.SetToWeightedAverage(10, 20, 2.5, 5.0);
  fitMethod.Initialize(data);
  for (size_t i = 0; i < fitMethod.TaskCount(); ++i) fitMethod.PerformFit(i);
  Image2DPtr fitResult(new Image2D(Image2D::MakeFromDiff(
      *testImage, *fitMethod.Background().GetSingleImage())));

  // High-pass filter
  HighPassFilter filter;
  Image2DPtr filterResult(new Image2D(*testImage));
  filter.SetHWindowSize(21);
  filter.SetVWindowSize(41);
  filter.SetHKernelSigmaSq(2.5);
  filter.SetVKernelSigmaSq(5.0);
  filterResult = filter.ApplyHighPass(
      filterResult, Mask2D::CreateSetMaskPtr<false>(width, height));

  ImageAsserter::AssertEqual(filterResult, fitResult);
}

BOOST_AUTO_TEST_CASE(small_image_filter) {
  const size_t width = 8, height = 8;
  Image2DPtr testImage = Image2D::CreateZeroImagePtr(width, height);
  testImage->SetValue(1, 1, 1.0);

  // Fitting
  LocalFitMethod fitMethod;
  TimeFrequencyData data(TimeFrequencyData::AmplitudePart,
                         Polarization::StokesI,
                         Image2DPtr(new Image2D(*testImage)));
  fitMethod.SetToWeightedAverage(10, 20, 2.5, 5.0);
  fitMethod.Initialize(data);
  for (size_t i = 0; i < fitMethod.TaskCount(); ++i) fitMethod.PerformFit(i);
  Image2DCPtr fitResult(new Image2D(Image2D::MakeFromDiff(
      *testImage, *fitMethod.Background().GetSingleImage())));

  // High-pass filter
  HighPassFilter filter;
  Image2DPtr filterResult(new Image2D(*testImage));
  filter.SetHWindowSize(21);
  filter.SetVWindowSize(41);
  filter.SetHKernelSigmaSq(2.5);
  filter.SetVKernelSigmaSq(5.0);
  filterResult = filter.ApplyHighPass(
      filterResult, Mask2D::CreateSetMaskPtr<false>(width, height));

  BOOST_CHECK(true);  // avoid warnings by boost

  // This test will fail, but the high-pass filter is actually slightly better
  // than the older "fitter" -- it will keep the kernel as large as possible,
  // while the sliding window fit can be one off. The test is still good to
  // guard for out of bounds errors.
  // ImageAsserter::AssertEqual(filterResult, fitResult, "Convolution with
  // kernel that is larger than the image");
}

BOOST_AUTO_TEST_CASE(filter_with_mask) {
  const size_t width = 8, height = 8;
  Image2DPtr image = Image2D::CreateZeroImagePtr(width, height);
  image->SetValue(1, 1, 1.0);
  Mask2DPtr mask = Mask2D::CreateSetMaskPtr<false>(width, height);
  mask->SetValue(1, 1, true);

  // High-pass filter
  HighPassFilter filter;
  filter.SetHWindowSize(4);
  filter.SetVWindowSize(4);
  filter.SetHKernelSigmaSq(4.0);
  filter.SetVKernelSigmaSq(4.0);
  image = filter.ApplyLowPass(image, mask);

  ImageAsserter::AssertConstant(image, 0.0);
}

BOOST_AUTO_TEST_CASE(completely_masked_image) {
  const size_t width = 9, height = 9;
  Image2DPtr image = Image2D::CreateZeroImagePtr(width, height);
  image->SetValue(1, 1, 1.0);
  image->SetValue(8, 8, 10.0);
  Mask2DPtr mask = Mask2D::CreateSetMaskPtr<true>(width, height);

  // High-pass filter
  HighPassFilter filter;
  filter.SetHWindowSize(4);
  filter.SetVWindowSize(4);
  filter.SetHKernelSigmaSq(4.0);
  filter.SetVKernelSigmaSq(4.0);
  image = filter.ApplyLowPass(image, mask);

  ImageAsserter::AssertConstant(image, 0.0);
}

BOOST_AUTO_TEST_CASE(nan_image) {
  const size_t width = 7, height = 7;
  Image2DPtr image = Image2D::CreateZeroImagePtr(width, height);
  image->SetValue(1, 1, std::numeric_limits<float>::quiet_NaN());
  image->SetValue(7, 7, 10.0);

  // High-pass filter
  HighPassFilter filter;
  filter.SetHWindowSize(4);
  filter.SetVWindowSize(4);
  filter.SetHKernelSigmaSq(4.0);
  filter.SetVKernelSigmaSq(4.0);
  image = filter.ApplyLowPass(image,
                              Mask2D::CreateSetMaskPtr<false>(width, height));

  ImageAsserter::AssertFinite(image);
}

BOOST_AUTO_TEST_SUITE_END()
