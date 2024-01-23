#include "../testingtools/imageasserter.h"

#include "../../structures/mask2d.h"
#include "../../structures/image2d.h"

#include "../../algorithms/highpassfilter.h"

#include <boost/test/unit_test.hpp>

using algorithms::HighPassFilter;

BOOST_AUTO_TEST_SUITE(high_pass_filter, *boost::unit_test::label("algorithms"))

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
