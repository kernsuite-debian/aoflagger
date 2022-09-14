#include "testtools.h"

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"

#include "../../algorithms/sumthresholdmissing.h"
#include "../../algorithms/testsetgenerator.h"
#include "../../algorithms/thresholdconfig.h"

#include <boost/test/unit_test.hpp>

using algorithms::SumThresholdMissing;

using test_tools::CompareHorizontalSumThreshold;
using test_tools::CompareVerticalSumThreshold;

BOOST_AUTO_TEST_SUITE(masked_sumthreshold,
                      *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(horizontal) {
  const unsigned width = 8, height = 8;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height),
         missing = Mask2D::MakeSetMask<false>(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  Image2D image = Image2D::MakeSetImage(width, height, 0.0);

  for (size_t y = 0; y != height; ++y) {
    image.SetValue(3, y, 1.0);
    image.SetValue(4, y, 1.0);
  }

  SumThresholdMissing::Horizontal(image, mask, missing, scratch, 2, 0.8);

  for (size_t y = 0; y != height; ++y) {
    BOOST_CHECK(mask.Value(3, y));
    BOOST_CHECK(mask.Value(4, y));
    BOOST_CHECK(!mask.Value(0, y));
    BOOST_CHECK(!mask.Value(2, y));
    BOOST_CHECK(!mask.Value(5, y));
  }
}

BOOST_AUTO_TEST_CASE(vertical) {
  const unsigned width = 8, height = 8;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height);
  Mask2D missing = Mask2D::MakeSetMask<false>(width, height);
  Mask2D scratch = Mask2D::MakeUnsetMask(width, height);
  Image2D image = Image2D::MakeSetImage(width, height, 0.0);

  for (size_t x = 0; x != width; ++x) {
    image.SetValue(x, 3, 1.0);
    image.SetValue(x, 4, 1.0);
  }

  SumThresholdMissing::Horizontal(image, mask, missing, scratch, 2, 0.8);

  for (size_t x = 0; x != width; ++x) {
    BOOST_CHECK(mask.Value(x, 3));
    BOOST_CHECK(mask.Value(x, 4));
    BOOST_CHECK(!mask.Value(x, 0));
    BOOST_CHECK(!mask.Value(x, 2));
    BOOST_CHECK(!mask.Value(x, 5));
  }
}

BOOST_AUTO_TEST_CASE(compare_vertical_masked_reference) {
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        Image2D mInput;
        Mask2D mMask;
        Mask2D missing;
        test_tools::IntroduceGap(*input, *mask, mInput, mMask, missing);
        SumThresholdMissing::VerticalReference(mInput, mMask, missing, *scratch,
                                               length, threshold);
        test_tools::RemoveGap(*mask, mMask);
      },
      {});
}

BOOST_AUTO_TEST_CASE(compare_vertical_masked_consecutive) {
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        Image2D mInput;
        Mask2D mMask;
        Mask2D missing;
        test_tools::IntroduceGap(*input, *mask, mInput, mMask, missing);
        SumThresholdMissing::VerticalConsecutive(mInput, mMask, missing,
                                                 *scratch, length, threshold);
        test_tools::RemoveGap(*mask, mMask);
      },
      {});
}

BOOST_AUTO_TEST_CASE(compare_vertical_masked_stacked) {
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        Image2D mInput;
        Mask2D mMask;
        Mask2D missing;
        test_tools::IntroduceGap(*input, *mask, mInput, mMask, missing);
        SumThresholdMissing::VerticalCache cache;
        SumThresholdMissing::InitializeVertical(cache, mInput, missing);
        SumThresholdMissing::VerticalStacked(cache, mInput, mMask, missing,
                                             *scratch, length, threshold);
        test_tools::RemoveGap(*mask, mMask);
      },
      {});
}

BOOST_AUTO_TEST_SUITE_END()
