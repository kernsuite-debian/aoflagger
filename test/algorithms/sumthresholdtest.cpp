#include "testtools.h"

#include "../../structures/image2d.h"
#include "../../structures/mask2d.h"
#include "../../structures/timefrequencydata.h"

#include "../../algorithms/thresholdconfig.h"
#include "../../algorithms/sumthreshold.h"
#include "../../algorithms/sumthresholdmissing.h"

#include <boost/test/unit_test.hpp>

using algorithms::SumThreshold;
using algorithms::SumThresholdMissing;
using algorithms::ThresholdConfig;

using test_tools::CompareHorizontalSumThreshold;
using test_tools::CompareVerticalSumThreshold;

BOOST_AUTO_TEST_SUITE(sumthreshold, *boost::unit_test::label("algorithms"))

BOOST_AUTO_TEST_CASE(masked_vertical_reference) {
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        Mask2D missing =
            Mask2D::MakeSetMask<false>(input->Width(), input->Height());
        SumThresholdMissing::VerticalReference(*input, *mask, missing, *scratch,
                                               length, threshold);
      },
      {});
}

BOOST_AUTO_TEST_CASE(masked_vertical_consecutive) {
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        Mask2D missing =
            Mask2D::MakeSetMask<false>(input->Width(), input->Height());
        SumThresholdMissing::VerticalConsecutive(*input, *mask, missing,
                                                 *scratch, length, threshold);
      },
      {});
}

BOOST_AUTO_TEST_CASE(masked_vertical_stacked) {
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        Mask2D missing =
            Mask2D::MakeSetMask<false>(input->Width(), input->Height());
        SumThresholdMissing::VerticalCache cache;
        SumThresholdMissing::InitializeVertical(cache, *input, missing);
        SumThresholdMissing::VerticalStacked(cache, *input, *mask, missing,
                                             *scratch, length, threshold);
      },
      {});
}

BOOST_AUTO_TEST_CASE(masked_horizontal_reference) {
  CompareHorizontalSumThreshold([](const Image2D* input, Mask2D* mask,
                                   Mask2D* scratch, size_t length,
                                   num_t threshold) {
    Mask2D missing =
        Mask2D::MakeSetMask<false>(input->Width(), input->Height());
    SumThresholdMissing::Horizontal(*input, *mask, missing, *scratch, length,
                                    threshold);
  });
}

#if defined(__SSE__) || defined(__x86_64__)
BOOST_AUTO_TEST_CASE(vertical_SSE) {
  if (!__builtin_cpu_supports("sse")) return;
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
         num_t threshold) {
        SumThreshold::VerticalLargeSSE(input, mask, scratch, length, threshold);
      },
      {32});
}

BOOST_AUTO_TEST_CASE(horizontal_SSE) {
  if (!__builtin_cpu_supports("sse")) return;
  CompareHorizontalSumThreshold([](const Image2D* input, Mask2D* mask,
                                   Mask2D* scratch, size_t length,
                                   num_t threshold) {
    SumThreshold::HorizontalLargeSSE(input, mask, scratch, length, threshold);
  });
}

BOOST_AUTO_TEST_CASE(stability_SSE) {
  if (!__builtin_cpu_supports("sse")) return;
  Mask2D maskA = Mask2D::MakeSetMask<false>(1, 1),
         maskB = Mask2D::MakeSetMask<false>(2, 2),
         maskC = Mask2D::MakeSetMask<false>(3, 3),
         maskD = Mask2D::MakeSetMask<false>(4, 4),
         scratch = Mask2D::MakeUnsetMask(4, 4);
  Image2D realA = Image2D::MakeZeroImage(1, 1),
          realB = Image2D::MakeZeroImage(2, 2),
          realC = Image2D::MakeZeroImage(3, 3),
          realD = Image2D::MakeZeroImage(4, 4);

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  config.InitializeThresholdsFromFirstThreshold(6.0, ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    SumThreshold::HorizontalLargeSSE(&realA, &maskA, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskA, &scratch, length, 1.0);
    SumThreshold::HorizontalLargeSSE(&realA, &maskB, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskB, &scratch, length, 1.0);
    SumThreshold::HorizontalLargeSSE(&realA, &maskC, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskC, &scratch, length, 1.0);
    SumThreshold::HorizontalLargeSSE(&realA, &maskD, &scratch, length, 1.0);
    SumThreshold::VerticalLargeSSE(&realA, &maskD, &scratch, length, 1.0);
  }
  BOOST_CHECK(true);
}
#endif  // defined(__SSE__) || defined(__x86_64__)

#if defined(__AVX2__) || defined(__x86_64__)
BOOST_AUTO_TEST_CASE(horizontal_sumthreshold_AVX_dumas) {
  if (!__builtin_cpu_supports("avx2")) return;
  CompareHorizontalSumThreshold([](const Image2D* input, Mask2D* mask, Mask2D*,
                                   size_t length, num_t threshold) {
    SumThreshold::HorizontalAVXDumas(input, mask, length, threshold);
  });
}

BOOST_AUTO_TEST_CASE(vertical_sumthreshold_AVX) {
  if (!__builtin_cpu_supports("avx2")) return;
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D*, size_t length,
         num_t threshold) {
        Mask2D scratch(*mask);
        SumThreshold::VerticalLargeAVX(input, mask, &scratch, length,
                                       threshold);
      },
      {32});
}

BOOST_AUTO_TEST_CASE(vertical_sumthreshold_AVX_dumas) {
  if (!__builtin_cpu_supports("avx2")) return;
  CompareVerticalSumThreshold(
      [](const Image2D* input, Mask2D* mask, Mask2D*, size_t length,
         num_t threshold) {
        SumThreshold::VerticalScratch vScratch(input->Width(), input->Height());
        SumThreshold::VerticalAVXDumas(input, mask, &vScratch, length,
                                       threshold);
      },
      {32});
}

static void SimpleVerticalSumThresholdAVX(bool dumas) {
  if (!__builtin_cpu_supports("avx2")) return;
  const unsigned width = 8, height = 8;
  Mask2D mask = Mask2D::MakeSetMask<false>(width, height),
         scratch = Mask2D::MakeUnsetMask(width, height);
  SumThreshold::VerticalScratch vScratch(width, height);
  Image2D image = Image2D::MakeSetImage(width, height, 0.0);

  for (size_t x = 0; x != width; ++x) {
    image.SetValue(x, 3, 1.0);
    image.SetValue(x, 4, 1.0);
  }

  if (dumas)
    SumThreshold::VerticalAVXDumas(&image, &mask, &vScratch, 2, 0.8);
  else
    SumThreshold::VerticalLargeAVX(&image, &mask, &scratch, 2, 0.8);

  for (size_t x = 0; x != width; ++x) {
    BOOST_CHECK(mask.Value(x, 3));
    BOOST_CHECK(mask.Value(x, 4));
    BOOST_CHECK(!mask.Value(x, 0));
    BOOST_CHECK(!mask.Value(x, 2));
    BOOST_CHECK(!mask.Value(x, 5));
  }
}

BOOST_AUTO_TEST_CASE(simple_vertical_sumthreshold_AVX) {
  if (!__builtin_cpu_supports("avx2")) return;
  SimpleVerticalSumThresholdAVX(false);
}

BOOST_AUTO_TEST_CASE(simple_vertical_sumthreshold_AVX_dumas) {
  if (!__builtin_cpu_supports("avx2")) return;
  SimpleVerticalSumThresholdAVX(true);
}

static void StabilityAVX(bool dumas) {
  if (!__builtin_cpu_supports("avx2")) return;
  Mask2D maskA = Mask2D::MakeSetMask<false>(1, 1),
         maskB = Mask2D::MakeSetMask<false>(2, 2),
         maskC = Mask2D::MakeSetMask<false>(3, 3),
         maskD = Mask2D::MakeSetMask<false>(4, 4),
         scratch = Mask2D::MakeUnsetMask(4, 4);
  SumThreshold::VerticalScratch vScratch(4, 4);
  Image2D realA = Image2D::MakeZeroImage(1, 1),
          realB = Image2D::MakeZeroImage(2, 2),
          realC = Image2D::MakeZeroImage(3, 3),
          realD = Image2D::MakeZeroImage(4, 4);

  ThresholdConfig config;
  config.InitializeLengthsDefault(9);
  config.InitializeThresholdsFromFirstThreshold(6.0, ThresholdConfig::Rayleigh);
  for (unsigned i = 0; i < 9; ++i) {
    const unsigned length = config.GetHorizontalLength(i);
    if (dumas) {
      SumThreshold::HorizontalAVXDumas(&realA, &maskA, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realA, &maskA, &vScratch, length, 1.0);
      SumThreshold::HorizontalAVXDumas(&realB, &maskB, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realB, &maskB, &vScratch, length, 1.0);
      SumThreshold::HorizontalAVXDumas(&realC, &maskC, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realC, &maskC, &vScratch, length, 1.0);
      SumThreshold::HorizontalAVXDumas(&realD, &maskD, length, 1.0);
      SumThreshold::VerticalAVXDumas(&realD, &maskD, &vScratch, length, 1.0);
    } else {
      SumThreshold::VerticalLargeAVX(&realA, &maskA, &scratch, length, 1.0);
      SumThreshold::VerticalLargeAVX(&realB, &maskB, &scratch, length, 1.0);
      SumThreshold::VerticalLargeAVX(&realC, &maskC, &scratch, length, 1.0);
      SumThreshold::VerticalLargeAVX(&realD, &maskD, &scratch, length, 1.0);
    }
  }
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(stability_AVX) { StabilityAVX(false); }

BOOST_AUTO_TEST_CASE(stability_AVX_dumas) { StabilityAVX(true); }

#endif  // defined(__AVX2__) || defined(__x86_64__)

BOOST_AUTO_TEST_SUITE_END()
